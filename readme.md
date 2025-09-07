### 记录一下最近项目需要，做的一个ncnn框架安卓移植过程

****

**之前安卓开发一窍不通，这里记录下学习过程和心得**

**第一次上传项目，可能文档中比较啰嗦，用词也比较随意，见谅**

**ncnn框架：https://github.com/Tencent/ncnn**

**opencv：https://github.com/opencv/opencv**

**u2net：https://github.com/HibikiJie/ReadMeter**



****



### 1.环境准备

**这里主要是以下几个部分：一个训练好的网络；ncnn（可能会需要opencv，建议也搞了）；android stdio**

#### 一个训练好的网络

这里选的是：https://github.com/HibikiJie/ReadMeter给出的u2net。

这里使用的网络是pytorch的，参数保存在了net.pt文件里，做轻量化移植需要使用ncnn框架，要做模型的的转换

ncnn的wiki给了模型转换教程：https://opendeep.wiki/Tencent/ncnn/model-conversion。ncnn推荐使用pnnx进行转换，下面实现。

- 重新保存net.pt

这里net.pt放在这个目录下，由于这个pt只保存了参数，没保存模型，需要创建个python文件加载网络模型，然后按照教程转换。把ReadMeter里面的models目录也放在当前目录，并新建一个transpt.py用来加载模型和转换。

<font size = "5" color = 'red'>后续绝对路径可以和我不一样，但是相对路径要一致</font>

<font size = "5" color = 'red'>在写第三节opencv的Andrioid Stdio实现时，由于opencv导入时提示路径有非法字符，这里我说明一下，图片里面路径就不改了，重新截图比较麻烦，我的路径的一切u2net修改为ncnn_net</font>

参考pnnx项目给出的教程：https://github.com/pnnx/pnnx。在项目的transpt目录下是模型转换教程。这里我已经按照ReadMeter项目**提前**创建好了python环境。目录结构如下，在E:\Imgseg\ncnn_unet下创建transpt目录，然后进入transpt目录，在这个目录下进入python环境安装pnnx和查看。

```bash
E:.
├─pic
└─transpt
    ├─.idea
    │  └─inspectionProfiles
    └─models
        └─__pycache__
```

```bash
pip3 install pnnx
pip show pnnx
```

![image-20250905094125596](pic\install_pnnx.png)

目录结构和transpt.py代码如下：

![image-20250905095859727](.\pic\catalog.png)

```python
#转换代码
import torch
from models.net import U2NET

model = U2NET(3, 2)
state_dict = torch.load('net.pt', map_location='cpu')
model.load_state_dict(state_dict)  # 加载权重

model.eval()
dummy_input = torch.randn(1, 3, 416, 416)  # 调整为你模型的输入尺寸

# 方法1：trace（适用于静态模型）
traced_model = torch.jit.trace(model, dummy_input)

traced_model.save('model.pt')  # 保存
```

导入模型和参数，按照pnnx教程把模型重新保存为model.pt

- 把.pt转换为.param和.bin

```bash
#在当前目录，当前环境的终端使用如下命令转换，参考pnnx，教程有讲解
#主要就是inputshape，这玩意根据不同的网络模型修改下就行
pnnx ./model.pt inputshape=[1,3,416,416]
```

结果如下：

![image-20250905101118058](.\pic\trans_result.png)

- 重命名一下（可选）

个人不喜欢xx.ncnn.xx这种命名格式，我重命名成model.bin和model.param了，可以不做，后面加载时相应修改下代码就行。

#### ncnn

**ncnn框架：https://github.com/Tencent/ncnn**进入项目的relase下载，这里下两个版本，下面的opencv也是。一个x86的windows版本的，一个android-shared版本的。也可以直接下载源代码自己编译。需要x86的windows版本是因为u2net网络预处理和后向处理都是pyton调用opencv实现的，但是移植到手机上需要用java调用c++实现，所以下一步是windows下的c++实现。**(linux也行，应该差不多，后面有空补一下ncnn在x86-linux下交叉编译arm-linux的项目)**

下载这俩然后解压。

```C
ncnn-20250503-windows-vs2022-shared.zip
ncnn-20250503-android-vulkan-shared.zip
//vs2022是我用的版本，vscode配置链接库也有教程
//如果是其他vs的版本也可以下对应的
//安卓的vulkan和非vulkan版本有什么区别目前还不清楚
```

#### opencv（可选，但是强烈建议做）

这个可以去官网下。下这俩。

**opencv官网：https://opencv.org/releases/**

![image-20250905104328925](.\pic\download_cv.png)

下载后是这俩文件，一个解压一个安装，记下路径就行

![image-20250905110512854](.\pic\cv.png)

我的路径：

```bash
#windows #目录下是build、sources和几个txt就对了
E:\Imgseg\libs\opencv 
#安卓 #目录下有samples、sdk就对了
E:\Imgseg\libs\opencv-4.7.0-android-sdk\OpenCV-android-sdk
```

#### Android Stdio

这个直接官网下载最新的安装就行。

---



### 2.ncnn框架下u2net的windows实现

在windows下把pytorch框架的u2net，以及前向处理和后向处理改成windows下的C++代码下ncnn框架实现，这步逻辑封装好点，留好接口方便后面Android stdio通过jni调用。u2net的实现比较简单，接口也比较清晰。

创建vs2022工程，在上一步工程子目录下创建。在E:\Imgseg\u2net目录下创建ncnntest用于windows下的ncnn框架的逻辑实现。目录如下。

<font size = "5" color = 'red'>这里的u2net后面改成ncnn_net了</font>

```bash
E:.
├─ncnntest
├─pic
└─transpt
    ├─.idea
    │  └─inspectionProfiles
    └─models
        └─__pycache__
```

vs2022在ncnntest目录创建工程。然后添加上一步得到的ncnn-windows和安装的opencv-windows库。

选择视图-->其他窗口-->属性管理器

![image-20250905113206506](.\pic\vs2022_lib.png)

在属性管理器选择Release | x64，右键，添加新项目属性表。

![image-20250905113625649](.\pic\Property_Manager.png)

新建一个opencv属性表和ncnn属性表。（也可以变成一个，但是有的项目可以不需要opencv的，所以做环境拆分）**创建时下面有个路径选择，记住！！！方便以后复用（也有可能再也不用QAQ）**

![image-20250905114447875](.\pic\Add_Property.png)

双击opencv_windows属性表，或者右键-->属性进入这个页面。

![image-20250905123548382](.\pic\cv_include.png)

添加的路径如下：

```bash
#包含目录
E:\Imgseg\libs\opencv\build\include

#库目录
E:\Imgseg\libs\opencv\build\x64\vc16\lib
```

![image-20250905122909263](.\pic\cv_include_input.png)

```bash
#附加依赖项
opencv_world470.lib
```

- <font color = 'red'>包含路径就是在写C++时，#include<xxx>,这个xxx前面的路径，你不添加了包含路径就需要写#include<a/b/c/d.h>这种的绝对路径。</font>
- <font color = 'red'>库目录就是.lib文件位置，告诉编译器我用到的头文件在哪实现的。</font>
- <font color = 'red'>附加依赖项是这个库的名字，用到了哪个库，这里是opencv_world470.lib这个库。</font>
- <font color = 'red'>用vscode的话也有类似的配置，或者写个CMakeLists.txt添加配置，这个在Android stdio那步有。</font>

让copilot写一段opencv测试代码如下：

```c++
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // 创建一个黑色图像
    cv::Mat image = cv::Mat::zeros(400, 400, CV_8UC3);

    // 在图像上画一个蓝色圆
    cv::circle(image, cv::Point(200, 200), 100, cv::Scalar(255, 0, 0), -1);

    // 显示图像
    cv::imshow("OpenCV 测试", image);

    // 等待用户按键
    cv::waitKey(0);

    // 释放窗口
    cv::destroyAllWindows();

    return 0;
}
```

运行时提示会报错。

![image-20250905124200670](.\pic\err.png)

这个文件在E:\Imgseg\libs\opencv\build\x64\vc16\bin里面，**解决办法是给他添加到环境变量或者直接复制到项目里面就行**。修改环境变量后重启一下项目，运行，测试通过，opencv导入成功。

![image-20250905124459452](.\pic\cv_test.png)

同理导入ncnn。**并添加环境变量！！！，如果下载源代码，自己编译.lib好像就不需要添加bin文件到环境变量**

```bash
#包含目录
E:\Imgseg\libs\ncnn-20250503-windows-vs2022-shared\x64\include

#库目录
E:\Imgseg\libs\ncnn-20250503-windows-vs2022-shared\x64\lib

#附加依赖项
ncnn.lib
```

把u2net的python代码用C++实现，建议ai生成然后修下bug，比自己翻译快。把model.bin和model.param复制到这个目录，以及一张测试图片，测试。

![image-20250905130423071](.\pic\ncnn_result.png)

<font size = '5' color = 'red'>整个工程放在ncnntest目录下了。</font>

----



### 3.android stdio移植

再建个目录！放android stdio工程，个人认为这玩意不太好用。创建工程时选择Native C++；

<font size = '5' color = 'red'>这里工程的Name我改成了asnet，应该也不能有数字？</font>

![image-20250905132633591](.\pic\as_select.png)

修改下使用Java，SDK版本和构建工具。这里我项目命名为ncnn4u2net。然后next，finish，创建工程。

save location位置改了，这里没截到。
<font size = '5' color = 'red'>项目名改成了asnet，路径的u2net也改为了ncnn_unet。save location真实路径：E:\Imgseg\ncnn_net\asforncnn</font>

![image-20250905133740879](.\pic\as_select_1.png)

用模拟器测试一下。出现下面界面说明项目创建成功，可以开始下一步了。

![image-20250905140151364](.\pic\mumu_test.png)

#### ncnn的安卓移植

![image-20250905150130779](.\pic\to_project.png)

生成的项目默认安卓视图，修改成project视图。

在app目录下新建libs，在app/src/main/cpp下新建include。参考上一步的include和lib，这一步实际上就是把ncnn的lib和include放进来。

把ncnn-20250503-android-vulkan-shared.zip解压后打开，有5个目录。

```bash
arm64-v8a
armeabi-v7a
riscv64
x86
x86_64
#这几个是安卓芯片架构的区别，不知道就全建立
#我的手机是arm64-v8a
#电脑模拟器与电脑是保持一致的，是x86_64
#所以我只建了arm64-v8a和x86_64
```

把对应目录下的lib目录下的libncnn.so复制过去。任意一个目录下的include目录下的ncnn目录全部复制到app/src/main/cpp/include里面。接着修改app/src/main/cpp/下的CMakeLists.txt。

```bash
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

# 添加你的本地库
add_library(${CMAKE_PROJECT_NAME} SHARED
        native-lib.cpp  #系统生成的文件，就是测试时的Hello from C++字符串来源，先别动
)

# 链接库
target_link_libraries(
        ${CMAKE_PROJECT_NAME}
        android
        ncnn     #告诉编辑器要libncnn.so这玩意
        log
)
```

sync now，给修改的CMakeLists.txt同步一下。

运行如果不报错就说明这步成功了。

<font color = 'red'>按理论来说opencv也能这么导入，但是我导入后不报错，编译也能通过，打开软件就闪退，当时没细看日志，现在写文档也不想验证了。通过其他方法导入了，以后有空再看为什么，或者不看了，能用就行</font>

#### opencv移植

查教程发现opencv大多通过inport model的方法导入，最后也是通过这个方法导入了。

把opencv-4.7.0-android-sdk.zip解压，记住路径。这里我的路径是：

```bash
E:\Imgseg\libs\opencv-4.7.0-android-sdk\OpenCV-android-sdk
#这个路径下要有个sdk文件夹
```



![image-20250905160306076](.\pic\import_cv.png)

这里把：sdk改成opencv，不改也行，后面有些步骤修改一下就行。

![image-20250905160528328](.\pic\import_cv_1.png)

这里报错是正常现象。打开opencv目录下的build.gradle，需要修改里面的compileSdkVersion，minSdkVersion和targetSdkVersion。在任意位置新建一个Android Stdio工程，与刚才建立的区别是选择Empty Views Activity项目而不是Active C++,其他流程全选一样的。找到项目的build.gradle，记下里面的compileSDK，minSDK，targetSdk。在Native C++模板下生成的工程build.gradle没有这些信息。**用完就可以给这个Empty Views Activity项目删除，就看个信息，后面用不到**

![image-20250905161407366](.\pic\build_gradle.png)

![image-20250905162259675](.\pic\empty_project.png)

**修改compileSdkVersion，minSdkVersion和targetSdkVersion；注释opencv/build.gradle；在android模块下添加命名空间。**

```bash
#opencv/build.gradle
compileSdkVersion 26
minSdkVersion 21
targetSdkVersion 26

#然后注释opencv/build.gradle这句
apply plugin: 'kotlin-android'

#opencv/build.gradle的android模块下添加命名空间
namespace "org.opencv"
```

运行没错的话就初步成功了。接着导入cv，让代码能调用。file-->Project Structure。Dependencies-->app-->+号-->3 Module Dependency。勾选opencv，确定，然后apply在OK。

![image-20250905172617277](.\pic\import_cv_2.png)

![image-20250905173609787](.\pic\import_cv_3.png)

这里运行会报错，提示aidl错误，给文件加进来

```bash
#运行时报错
#在工程目录下创建app/src/main/aidl/org/opencv/engine
#把原sdk路径下的OpenCVEngineInterface.aidl复制过去
#绝对路径
E:\Imgseg\libs\opencv-4.7.0-android-sdk\OpenCV-android-sdk\sdk\java\src\org\opencv\engine\OpenCVEngineInterface.aidl
```

然后在opencv的build.gradle文件中加入一段

![image-20250905174449685](.\pic\add2cv_build.png)

```java
buildFeatures {
    aidl true
}
```

在工程的gradle.properties文件中最后加入这句

```bash
android.defaults.buildfeatures.buildconfig=true
```

重新构建一下，能运行就基本成功了，可以用java掉用opencv的接口了，C++使用还需要在CMakeLists.txt进行修改。



现在修改C++调用opencv的CMakeLists.txt

```cmake
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
```

现在opencv和ncnn全部导入Android stdio工程了，可以在cpp文件夹下增加代码实现调用了。

首先把第二部windows上C++实现的逻辑拿过来，新建MeterReader.h和MeterReader.cpp。代码比较长，这里不放了，直接看文件吧，主要逻辑就是读入model.param和model.bin的路径，加载模型和参数。然后把处理的归一化字符串返回。

**这里的model.param和model.bin放入Android stdio的assets目录，这个目录的东西不会直接编译，会保持不动。通过java在创建时把assets目录下bin和param复制到安卓的私有目录下，然后C++调用。也可以C++代码通过jni调用。这里项目要求能用就行，我选择复制过来。复制的代码是AssetCopyer.java这个文件实现的和MainActivity.java在同一目录下。这个代码后续也没有修改，就不放了。**

<font color = 'red'>在app/src/main/下新建assets目录，然后把model.param和model.bin复制过来然后放一张测试图片110.jpg，修改上文的CMakeLIsts.txt，看CMakeLIsts.txt的注释。然后修改native-lib.cpp如下</font>

```cpp
//native-lib.cpp
#include <jni.h>
#include <string>
#include "MeterReader.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_asnet_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject, /* this */
        jstring javaPath /*传递路径，找到param和bin的路径*/) {
    // 将jstring转换为C风格的字符串
    const char* cPath = env->GetStringUTFChars(javaPath, nullptr);
    std::string result(cPath);
    env->ReleaseStringUTFChars(javaPath, cPath);


    std::string param_path =  result + "/model.param";
    std::string bin_path =  result + "/model.bin";
    std::string image_path =  result + "/110.jpg";

    MeterReader reader;
    reader.loadModel(param_path, bin_path);
    cv::Mat image = cv::imread(image_path);

    // 处理图像
    float ratio = reader.process(image);

    std::string hello = std::to_string(ratio);
    return env->NewStringUTF(hello.c_str());
}
```

<font color = 'red'>修改MainActivity.java，主要是jin接口参数修改和简单调用</font>

```java
package com.example.asnet;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import com.example.asnet.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'asnet' library on application startup.
    static {
        System.loadLibrary("asnet");
    }

    private ActivityMainBinding binding;
    private String filePath; // 参数和模型路径

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //Environment.getExternalStorageDirectory().getAbsolutePath() = /storage/emulated/0
        //this.getExternalFilesDir(null).getAbsolutePath() = /storage/emulated/0/Android/data/com.example.cv/files
        //this.getApplicationContext()
        //复制参数和模型到/storage/emulated/0/Android/data/com.example.cv/files
        filePath = this.getExternalFilesDir(null).getAbsolutePath();
        AssetCopyer.copyAllAssets(this.getApplicationContext(),filePath);

        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        // Example of a call to a native method
        TextView tv = binding.sampleText;
        tv.setText(stringFromJNI(filePath));
    }

    /**
     * A native method that is implemented by the 'asnet' native library,
     * which is packaged with this application.
     * 修改参数jni的参数列表
     */
    public native String stringFromJNI(String javaPath);
}
```

看看运行效果。**这里给出了归一化的检测结果，0.65*1.6=1.04，原图大概1.11这样。**

![image-20250906195150009](.\pic\run_1.png)

#### 安卓开发

<font color = 'red'>到这一步其实就和C++没关系了，纯安卓开发了，整体思路就是修改activity_main.xml创建一个ImageView，两个按钮，一个默认的TextView。点击第一个按钮打开图片并用ImageView展示，点击第二个按钮把图片路径通过jni调用C++代码识别，把识别结果返回并通过TextView展示。</font>

<font color = 'red'>这个xml设计器类似qt拖拽创建，修改id代码匹配。还算好写,注意一下id就行，通过id绑定,可以修改成代码模式，直接复制过去完成设计，也可以自己设计</font>

![image-20250906200837224](.\pic\code_or_design.png)

<font color = 'red' size = '6'>这里用到了@string/，这个在strings.xml修改</font>

**activity_main.xml路径：app/src/main/res/layout/activity_main.xml**

**strings.xml路径:app/src/main/res/values/strings.xml**

修改activity_main.xml

```xml
<?xml version="1.0" encoding="utf-8"?>
<androidx.constraintlayout.widget.ConstraintLayout xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:app="http://schemas.android.com/apk/res-auto"
    xmlns:tools="http://schemas.android.com/tools"
    android:layout_width="match_parent"
    android:layout_height="match_parent"
    tools:context=".MainActivity">

    <ImageView
        android:id="@+id/imageView"
        android:layout_width="294dp"
        android:layout_height="252dp"
        android:layout_marginTop="36dp"
        android:contentDescription="@string/photo"
        app:layout_constraintEnd_toEndOf="parent"
        app:layout_constraintHorizontal_bias="0.495"
        app:layout_constraintStart_toStartOf="parent"
        app:layout_constraintTop_toTopOf="parent"
        app:srcCompat="@android:drawable/ic_menu_camera" />
	
    <Button
        android:id="@+id/openphoto"
        android:layout_width="126dp"
        android:layout_height="52dp"
        android:text="@string/open_photo"
        app:layout_constraintBottom_toTopOf="@+id/sample_text"
        app:layout_constraintEnd_toStartOf="@+id/detect"
        app:layout_constraintHorizontal_bias="0.563"
        app:layout_constraintStart_toStartOf="parent"
        app:layout_constraintTop_toBottomOf="@+id/imageView"
        app:layout_constraintVertical_bias="0.503" />

    <Button
        android:id="@+id/detect"
        android:layout_width="126dp"
        android:layout_height="52dp"
        android:layout_marginEnd="56dp"
        android:text="@string/detect"
        app:layout_constraintBottom_toTopOf="@+id/sample_text"
        app:layout_constraintEnd_toEndOf="parent"
        app:layout_constraintTop_toBottomOf="@+id/imageView"
        app:layout_constraintVertical_bias="0.503" />

    <TextView
        android:id="@+id/sample_text"
        android:layout_width="290dp"
        android:layout_height="155dp"
        android:textSize="20sp"
        app:layout_constraintBottom_toBottomOf="parent"
        app:layout_constraintEnd_toEndOf="parent"
        app:layout_constraintStart_toStartOf="parent"
        app:layout_constraintTop_toBottomOf="@+id/imageView" />

</androidx.constraintlayout.widget.ConstraintLayout>
```

修改strings.xml

```xml
<resources>
    <string name="app_name">asnet</string>
    <string name="open_photo">打开照片</string>
    <string name="detect">预测</string>
    <string name="result">result</string>
    <string name="photo">photo</string>
</resources>
```

最后是绑定一下这几个控件，这样就可以通过代码控制。

在MainActivity.java中绑定,下面代码只是一部分，**不能用啊**，首先定义TextView，Button，ImageView这么几个属性变量，然后通过findViewById绑定，最后通过setOnClickListener设置触发函数。

```java
//定义这么几个属性的变量
private TextView tv;
private Button open, detect;
private ImageView img;

//绑定按钮
tv = findViewById(R.id.sample_text);
img = findViewById(R.id.imageView);
open = findViewById(R.id.openphoto);
detect = findViewById(R.id.detect);

//设置按钮监听
open.setOnClickListener(v -> on_open_button_clicked());
detect.setOnClickListener(v -> on_detect_button_clicked());
detect.setEnabled(false);
```

<font color = 'red' size = '5'>监听函数和整体实现就放工程里面了，完结撒花！！！</font>

<font color = 'blue' size = '5'>上个忧郁蓝调。同门说上个项目要求30M以内的apk，这个项目光模型和参数就80M了，生成的apk200+了，不过目前只是一个demo，演示时不用管大小。后面如果接项目了真实去做，只能祝愿看我这个教程的学弟学妹好运了。思路大概是先给模型改小，做个轻量化的模型，然后Android stdio好像有个功能可以优化没用到的代码，比如这个项目的opencv就是做了一个图片的腐蚀操作，如果不行就给cv库去掉，只保留需要的核心代码直接写项目里把。</font>

























