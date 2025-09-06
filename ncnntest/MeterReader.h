#pragma once
#ifndef METERREADER_H
#define METERREADER_H

#include <opencv2/opencv.hpp>
#include "ncnn/net.h"

class MeterReader {
public:
    MeterReader();
    ~MeterReader();

    bool loadModel(const std::string& paramPath, const std::string& binPath);
    float process(const cv::Mat& image);

private:
    ncnn::Net net_;
    bool isModelLoaded_;

    // 超参数（需要根据你的表盘调整）
    const int line_width_ = 1600;
    const int line_height_ = 150;
    const int circle_radius_ = 200;
    const int circle_center_[2] = { 208, 208 };
    const float pi_ = 3.1415926535898f;
    const float threshold_ = 0.5f;

    // 预处理
    cv::Mat squarePicture(const cv::Mat& image, int image_size);
    ncnn::Mat toTensor(const cv::Mat& image);

    // 后处理
    cv::Mat binaryImage(const ncnn::Mat& output, int channel);
    cv::Mat corrosion(const cv::Mat& image);
    cv::Mat createLineImage(const cv::Mat& image_mask);
    std::vector<float> convert1DData(const cv::Mat& line_image);
    std::vector<float> meanFiltration(const std::vector<float>& data);

    struct ReadingResult {
        int scale_num;
        float num_scale;
        float ratio;
    };

    ReadingResult getRelativeValue(const cv::Mat& image_pointer, const cv::Mat& image_dail);
};

#endif // METERREADER_H#pragma once
