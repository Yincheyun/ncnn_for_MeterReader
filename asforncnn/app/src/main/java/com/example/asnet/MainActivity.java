package com.example.asnet;

import androidx.appcompat.app.AppCompatActivity;

import android.app.Activity;
import android.content.Intent;

import android.graphics.Bitmap;
import android.net.Uri;

import android.os.Bundle;

import android.provider.MediaStore;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;


import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'asnet' library on application startup.
    static {
        System.loadLibrary("asnet");
    }

    private String imgPath; // 保存图片路径
    private String filePath; // 参数和模型路径
    private static final int PICK_IMAGE_REQUEST = 1;
    private TextView tv;
    private Button open, detect;
    private ImageView img;
    private Bitmap currentBitmap;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        //binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(R.layout.activity_main);


        //Environment.getExternalStorageDirectory().getAbsolutePath() = /storage/emulated/0
        //this.getExternalFilesDir(null).getAbsolutePath() = /storage/emulated/0/Android/data/com.example.cv/files
        //this.getApplicationContext()
        //复制参数和模型到/storage/emulated/0/Android/data/com.example.cv/files
        filePath = this.getExternalFilesDir(null).getAbsolutePath();
        AssetCopyer.copyAllAssets(this.getApplicationContext(),filePath);

        //绑定按钮
        tv = findViewById(R.id.sample_text);
        img = findViewById(R.id.imageView);
        open = findViewById(R.id.openphoto);
        detect = findViewById(R.id.detect);
        //tv.setText(stringFromJNI());

        //设置按钮监听
        open.setOnClickListener(v -> on_open_button_clicked());
        detect.setOnClickListener(v -> on_detect_button_clicked());
        detect.setEnabled(false);
    }

    private void on_open_button_clicked() {
        Intent intent = new Intent(Intent.ACTION_PICK);
        intent.setType("image/*");
        startActivityForResult(intent, PICK_IMAGE_REQUEST);
    }

    private void on_detect_button_clicked() {
        String as = "/110.jpg";
        imgPath = filePath + as;
        String result = stringFromJNI(filePath, imgPath);
        tv.setText(result);
        Toast.makeText(this, result, Toast.LENGTH_LONG).show();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        // 首先调用父类的 onActivityResult 方法
        super.onActivityResult(requestCode, resultCode, data);

        // 检查请求码、结果码以及数据是否为空
        if (requestCode == PICK_IMAGE_REQUEST && resultCode == Activity.RESULT_OK && data != null && data.getData() != null) {
            Uri imageUri = data.getData();

            try {
                currentBitmap = MediaStore.Images.Media.getBitmap(getContentResolver(), imageUri);
                img.setImageBitmap(currentBitmap);
                detect.setEnabled(true);
                // 获取应用的外部私有文件目录。这个路径对应的就是 /storage/emulated/0/Android/data/com.example.cv/files
                // 注意：如果getExternalFilesDir(null)返回null，说明外部存储不可用，需要处理
                File externalFilesDir = getExternalFilesDir(null);
                if (externalFilesDir == null) {
                    throw new IOException("外部存储不可用或无法访问。");
                }

                // 定义目标文件名
                String destinationFileName = "110.jpg";
                // 在外部私有目录创建目标文件
                File destinationFile = new File(externalFilesDir, destinationFileName);

                // 使用 try-with-resources 确保输入流和输出流在操作完成后自动关闭
                try (InputStream inputStream = getContentResolver().openInputStream(imageUri);
                     FileOutputStream outputStream = new FileOutputStream(destinationFile)) {

                    if (inputStream == null) {
                        throw new IOException("无法打开图片输入流。");
                    }

                    byte[] buffer = new byte[1024]; // 缓冲区
                    int bytesRead; // 每次读取的字节数

                    // 循环读取输入流数据并写入输出流
                    while ((bytesRead = inputStream.read(buffer)) != -1) {
                        outputStream.write(buffer, 0, bytesRead);
                    }

                    // 更新 imgPath 变量，保存的是复制到私有目录后的文件绝对路径
                    imgPath = destinationFile.getAbsolutePath();

                    // 在TextView中显示保存的路径，给予用户反馈
                    if (tv != null) {
                        tv.setText("打开文件成功！");
                    }
                    Toast.makeText(this, "打开文件成功！", Toast.LENGTH_LONG).show();

                    // 现在你可以使用 imgPath (例如，加载到 ImageView 中显示)

                } catch (IOException e) {
                    // 处理文件读写错误
                    e.printStackTrace();
                    String errorMessage = "保存图片时发生IO错误: " + e.getMessage();
                    Toast.makeText(this, errorMessage, Toast.LENGTH_LONG).show();
                    if (tv != null) {
                        tv.setText("错误: " + errorMessage);
                    }
                }

            } catch (Exception e) {
                // 处理其他可能发生的异常
                e.printStackTrace();
                String errorMessage = "处理图片时发生意外错误: " + e.getMessage();
                Toast.makeText(this, errorMessage, Toast.LENGTH_LONG).show();
                if (tv != null) {
                    tv.setText("错误: " + errorMessage);
                }
            }
        } else if (resultCode == Activity.RESULT_CANCELED) {
            // 用户取消了图片选择
            Toast.makeText(this, "图片选择已取消。", Toast.LENGTH_SHORT).show();
        } else {
            // 图片选择失败（例如，没有选择图片）
            Toast.makeText(this, "未能成功选择图片。", Toast.LENGTH_SHORT).show();
        }
    }

    /**
     * A native method that is implemented by the 'asnet' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI(String javaPath, String imagePath);
}