#include <jni.h>
#include <string>
#include "MeterReader.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_asnet_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject, /* this */
        jstring javaPath, /*传递路径，找到param和bin的路径*/
        jstring imagePath) {
    // 将jstring转换为C风格的字符串
    const char* cPath = env->GetStringUTFChars(javaPath, nullptr);
    std::string result(cPath);
    env->ReleaseStringUTFChars(javaPath, cPath);

    const char* c_image_path = env->GetStringUTFChars(imagePath, nullptr);
    std::string image_path(c_image_path);
    env->ReleaseStringUTFChars(imagePath, c_image_path);

    std::string param_path =  result + "/model.param";
    std::string bin_path =  result + "/model.bin";

    MeterReader reader;

    reader.loadModel(param_path, bin_path);

    cv::Mat image = cv::imread(image_path);
    // 处理图像
    float ratio = -1234.01234;
    ratio = reader.process(image);
    std::cout << "Reading ratio: " << ratio << std::endl;
    std::string hello = std::to_string(ratio);

    return env->NewStringUTF(hello.c_str());
}