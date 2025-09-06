cmake_minimum_required(VERSION 3.22.1)

project("asnet")
#这俩是系统生成的留着

# 包含头文件
# CMAKE_SOURCE_DIR就是一直到cpp的路径
#其实就是包含了cpp/和cpp/include/这俩目录，参考VS2022那步，只保留cpp/include/也行
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)

# 引入 ncnn 动态库，就是导入libncnn.so的路径
add_library(ncnn SHARED IMPORTED)
set_target_properties(ncnn PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/../../../libs/${ANDROID_ABI}/libncnn.so)

# 添加 OpenCV 的包含目录
include_directories(
        ${CMAKE_SOURCE_DIR}/../../../../opencv/native/jni/include
)

# 查找 OpenCV 库文件
# 假设 OpenCV 库文件位于 OpenCV 项目的 libs 目录下，并且库文件是针对当前 ABI 编译的
set(OPENCV_LIBS "${CMAKE_SOURCE_DIR}/../../../../opencv/native/libs/${ANDROID_ABI}/libopencv_java4.so")



# 添加你的本地库
add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp  #系统生成的文件，就是测试是的Hello from C++字符串来源，先别动
        MeterReader.cpp #下一步用到的，这一步测试的话注释掉这句
)

# 链接库
target_link_libraries(
        ${CMAKE_PROJECT_NAME}
        android
        ncnn     #告诉编辑器要libncnn.so这玩意
        ${OPENCV_LIBS}
        log
)