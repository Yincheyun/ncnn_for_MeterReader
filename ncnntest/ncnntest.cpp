#include "MeterReader.h"
#include <iostream>

int main() {
    MeterReader reader;

    // 加载模型
    if (!reader.loadModel("model.param", "model.bin")) {
        std::cerr << "Failed to load model" << std::endl;
        return -1;
    }

    // 读取图像
    cv::Mat image = cv::imread("110.jpg");
    if (image.empty()) {
        std::cerr << "Failed to load image" << std::endl;
        return -1;
    }

    // 处理图像
    float ratio = reader.process(image);

    std::string hello = std::to_string(ratio);

    std::cout << "Reading ratio: " << hello << std::endl;

    return 0;
}