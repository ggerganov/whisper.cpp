package com.litongjava.whisper.android.java;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.widget.TextView;

import com.blankj.utilcode.util.ThreadUtils;
import com.litongjava.android.view.inject.annotation.FindViewById;
import com.litongjava.android.view.inject.annotation.FindViewByIdLayout;
import com.litongjava.android.view.inject.annotation.OnClick;
import com.litongjava.android.view.inject.utils.ViewInjectUtils;
import com.litongjava.jfinal.aop.Aop;
import com.litongjava.jfinal.aop.AopManager;
import com.litongjava.whisper.android.java.services.WhisperService;
import com.litongjava.whisper.android.java.task.LoadModelTask;
import com.litongjava.whisper.android.java.task.TranscriptionTask;
import com.litongjava.whisper.android.java.utils.AssetUtils;
import com.whispercpp.java.whisper.CDELog;
import com.whispercpp.java.whisper.CDEUtils;
import com.whispercpp.java.whisper.WhisperLib;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;


@FindViewByIdLayout(R.layout.activity_main)
public class MainActivity extends AppCompatActivity {

    @FindViewById(R.id.sample_text)
    private TextView tv;
    @FindViewById(R.id.sysinfo)
    private TextView tvSysInfo;
    private static final String TAG = WhisperService.class.getName();
    private static final int SDK_PERMISSION_REQUEST = 4;
    Logger log = LoggerFactory.getLogger(this.getClass());
    private WhisperService whisperService = Aop.get(WhisperService.class);

    @RequiresApi(api = Build.VERSION_CODES.O)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //setContentView(R.layout.activity_main);
        ViewInjectUtils.injectActivity(this, this);
        initAopBean();
        showSystemInfo();

        requestPermissions();
    }

    private void initAopBean() {
        Handler mainHandler = new Handler(Looper.getMainLooper());
        AopManager.me().addSingletonObject(mainHandler);
    }

    @RequiresApi(api = Build.VERSION_CODES.O)
    @OnClick(R.id.loadModelBtn)
    public void loadModelBtn_OnClick(View v) {
        Context context = getBaseContext();
        ThreadUtils.executeByIo(new LoadModelTask(tv));
    }

    @OnClick(R.id.transcriptSampleBtn)
    public void transcriptSampleBtn_OnClick(View v) {
        Context context = getBaseContext();

        long start = System.currentTimeMillis();
        String sampleFilePath = CDEUtils.getDataPath() + "samples/jfk.wav";
        File sampleFile = new File(sampleFilePath);
        long end = System.currentTimeMillis();
        String msg = "copy file:" + (end - start) + "ms";
        CDELog.j(TAG, msg);
        outputMsg(tv, msg);
        CDELog.j(TAG, "sample file:" + sampleFile.getAbsolutePath());
        ThreadUtils.executeByIo(new TranscriptionTask(tv, sampleFile));
    }

    private void outputMsg(TextView tv, String msg) {
        tv.append(msg + "\n");
        log.info(msg);
        CDELog.j(TAG, msg);
    }


    @RequiresApi(api = Build.VERSION_CODES.O)
    public void showSystemInfo() {
        String systemInfo = WhisperLib.getSystemInfo();
        String phoneInfo = "Device info:" + "\n"
                + "Brand:" + Build.BRAND + "\n"
                + "Arch:" + Build.CPU_ABI + "(" + systemInfo + ")" + "\n"
                + "Hardware:" + Build.HARDWARE +  "\n"
                /*+ "Fingerprint:" + Build.FINGERPRINT + "\n"*/
                + "OS:" + "Android " + android.os.Build.VERSION.RELEASE;
        tvSysInfo.append(phoneInfo + "\n\n");
        tvSysInfo.append("Powered by whisper.cpp(https://github.com/ggerganov/whisper.cpp)\n\n");
    }


    @RequiresApi(api = Build.VERSION_CODES.O)
    @Override
    protected void onDestroy() {
        super.onDestroy();
        whisperService.release();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == SDK_PERMISSION_REQUEST) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                CDELog.j(TAG, "PERMISSIONS was granted");
            } else {
                CDELog.j(TAG, "PERMISSIONS was denied");
                CDEUtils.exitAPK(this);
            }
        }
    }


    @TargetApi(23)
    private void requestPermissions() {
        String permissionInfo = " ";

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
                    || ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED
            ) {
                ActivityCompat.requestPermissions(this,
                        new String[]{
                                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                                Manifest.permission.READ_EXTERNAL_STORAGE
                        },
                        SDK_PERMISSION_REQUEST);
            }

        }
    }

}