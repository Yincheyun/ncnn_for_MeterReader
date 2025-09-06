#include "MeterReader.h"
#include <cmath>       // ��ѧ����
#include <iostream>    // �������
#include <vector>      // ��������
#include <string>      // �ַ������� 
#include <algorithm>   // �㷨����������max, min�ȣ�
//#include <opencv2/opencv.hpp>


MeterReader::MeterReader() : isModelLoaded_(false) {}

MeterReader::~MeterReader() {}

bool MeterReader::loadModel(const std::string& paramPath, const std::string& binPath) {
    int ret = net_.load_param(paramPath.c_str());
    if (ret != 0) return false;

    ret = net_.load_model(binPath.c_str());
    if (ret != 0) return false;

    isModelLoaded_ = true;
    return true;
}

float MeterReader::process(const cv::Mat& image) {
    if (!isModelLoaded_) return -1.0f;

    // Ԥ����
    cv::Mat processedImage = squarePicture(image, 416);
    ncnn::Mat input = toTensor(processedImage);

    // ����
    ncnn::Extractor ex = net_.create_extractor();
    ex.set_light_mode(true);
    ex.set_num_threads(4);

    ex.input("in0", input); // ����ʵ������ڵ������޸�

    ncnn::Mat output;
    ex.extract("out0", output); // ����ʵ������ڵ������޸�

    // ����
    cv::Mat pointer_mask = binaryImage(output, 0);
    cv::Mat dail_mask = binaryImage(output, 1);

    ReadingResult result = getRelativeValue(pointer_mask, dail_mask);
    return result.ratio;
}

// Ԥ������
cv::Mat MeterReader::squarePicture(const cv::Mat& image, int image_size) {
    int h1 = image.rows, w1 = image.cols;
    int max_len = (std::max)(h1, w1);//�޸�
    float fx = static_cast<float>(image_size) / max_len;
    float fy = static_cast<float>(image_size) / max_len;

    cv::Mat resized;
    cv::resize(image, resized, cv::Size(), fx, fy, cv::INTER_AREA);

    int h2 = resized.rows, w2 = resized.cols;
    cv::Mat background(image_size, image_size, CV_8UC3, cv::Scalar(127, 127, 127));

    int s_h = image_size / 2 - h2 / 2;
    int s_w = image_size / 2 - w2 / 2;

    cv::Mat roi = background(cv::Rect(s_w, s_h, w2, h2));
    resized.copyTo(roi);

    return background;
}

ncnn::Mat MeterReader::toTensor(const cv::Mat& image) {
    ncnn::Mat in = ncnn::Mat::from_pixels(image.data, ncnn::Mat::PIXEL_BGR, image.cols, image.rows);
    in.substract_mean_normalize(nullptr, nullptr); // ����ѵ��ʱ�Ĺ�һ����������

    // ���ѵ��ʱ��[0,1]��һ��
    const float norm_vals[3] = { 1 / 255.f, 1 / 255.f, 1 / 255.f };
    in.substract_mean_normalize(nullptr, norm_vals);

    return in;
}

// ������
cv::Mat MeterReader::binaryImage(const ncnn::Mat& output, int channel) {
    int h = output.h;
    int w = output.w;
    cv::Mat mask(h, w, CV_32FC1);

    // ��������
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            float value = output.channel(channel).row(i)[j];
            mask.at<float>(i, j) = value > threshold_ ? 1.0f : 0.0f;
        }
    }

    // ת��Ϊ8UC1���ڸ�ʴ����
    cv::Mat mask_8u;
    mask.convertTo(mask_8u, CV_8UC1, 255);
    mask_8u = corrosion(mask_8u);

    // ת����float
    cv::Mat result;
    mask_8u.convertTo(result, CV_32FC1, 1.0 / 255.0);

    return result;
}

cv::Mat MeterReader::corrosion(const cv::Mat& image) {
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::Mat eroded;
    cv::erode(image, eroded, kernel);
    return eroded;
}

cv::Mat MeterReader::createLineImage(const cv::Mat& image_mask) {
    cv::Mat line_image(line_height_, line_width_, CV_32FC1, cv::Scalar(0));

    for (int row = 0; row < line_height_; row++) {
        for (int col = 0; col < line_width_; col++) {
            float theta = (2 * pi_ / line_width_) * (col + 1);
            int radius = circle_radius_ - row - 1;

            int y = static_cast<int>(circle_center_[0] + radius * std::cos(theta) + 0.5f);
            int x = static_cast<int>(circle_center_[1] - radius * std::sin(theta) + 0.5f);

            // �߽���
            if (x >= 0 && x < image_mask.cols && y >= 0 && y < image_mask.rows) {
                line_image.at<float>(row, col) = image_mask.at<float>(y, x);
            }
        }
    }

    return line_image;
}

std::vector<float> MeterReader::convert1DData(const cv::Mat& line_image) {
    std::vector<float> data_1d(line_width_, 0.0f);

    for (int col = 0; col < line_width_; col++) {
        for (int row = 0; row < line_height_; row++) {
            if (line_image.at<float>(row, col) > 0.5f) {
                data_1d[col] += 1.0f;
            }
        }
    }

    return data_1d;
}

std::vector<float> MeterReader::meanFiltration(const std::vector<float>& data) {
    float sum = 0.0f;
    for (float value : data) {
        sum += value;
    }
    float mean = sum / data.size();

    std::vector<float> filtered = data;
    for (float& value : filtered) {
        if (value < mean) {
            value = 0.0f;
        }
    }

    return filtered;
}

MeterReader::ReadingResult MeterReader::getRelativeValue(const cv::Mat& image_pointer, const cv::Mat& image_dail) {
    cv::Mat line_image_pointer = createLineImage(image_pointer);
    cv::Mat line_image_dail = createLineImage(image_dail);

    std::vector<float> data_1d_pointer = convert1DData(line_image_pointer);
    std::vector<float> data_1d_dail = convert1DData(line_image_dail);
    data_1d_dail = meanFiltration(data_1d_dail);

    ReadingResult result{ -1, -1.0f, -1.0f };

    bool dail_flag = false;
    bool pointer_flag = false;
    int one_dail_start = 0, one_dail_end = 0;
    int one_pointer_start = 0, one_pointer_end = 0;
    std::vector<float> dail_location;
    float pointer_location = 0.0f;

    for (int i = 0; i < line_width_ - 1; i++) {
        // ����̶�
        if (data_1d_dail[i] > 0 && data_1d_dail[i + 1] > 0) {
            if (!dail_flag) {
                one_dail_start = i;
                dail_flag = true;
            }
        }
        if (dail_flag) {
            if (data_1d_dail[i] == 0 && data_1d_dail[i + 1] == 0) {
                one_dail_end = i - 1;
                float one_dail_location = (one_dail_start + one_dail_end) / 2.0f;
                dail_location.push_back(one_dail_location);
                dail_flag = false;
            }
        }

        // ����ָ��
        if (data_1d_pointer[i] > 0 && data_1d_pointer[i + 1] > 0) {
            if (!pointer_flag) {
                one_pointer_start = i;
                pointer_flag = true;
            }
        }
        if (pointer_flag) {
            if (data_1d_pointer[i] == 0 && data_1d_pointer[i + 1] == 0) {
                one_pointer_end = i - 1;
                pointer_location = (one_pointer_start + one_pointer_end) / 2.0f;
                pointer_flag = false;
            }
        }
    }

    int scale_num = dail_location.size();
    if (scale_num > 0) {
        for (int i = 0; i < scale_num - 1; i++) {
            if (dail_location[i] <= pointer_location && pointer_location < dail_location[i + 1]) {
                result.num_scale = i + (pointer_location - dail_location[i]) /
                    (dail_location[i + 1] - dail_location[i] + 1e-5f) + 1;
                break;
            }
        }
        result.ratio = (pointer_location - dail_location[0]) /
            (dail_location.back() - dail_location[0] + 1e-5f);
        result.scale_num = scale_num;
    }

    return result;
}