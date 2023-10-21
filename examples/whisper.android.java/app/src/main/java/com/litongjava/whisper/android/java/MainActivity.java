package com.litongjava.whisper.android.java;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import com.litongjava.android.view.inject.annotation.FindViewById;
import com.litongjava.android.view.inject.annotation.FindViewByIdLayout;
import com.litongjava.android.view.inject.annotation.OnClick;
import com.litongjava.android.view.inject.utils.ViewInjectUtils;
import com.litongjava.jfinal.aop.Aop;
import com.litongjava.whisper.android.java.services.WhisperService;
import com.litongjava.whisper.android.java.utils.AssetUtils;
import com.litongjava.whisper.android.java.utils.WaveEncoder;
import com.whispercppdemo.whisper.WhisperContext;
import com.whispercppdemo.whisper.WhisperLib;

import org.slf4j.ILoggerFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.ExecutionException;


@FindViewByIdLayout(R.layout.activity_main)
public class MainActivity extends AppCompatActivity {

  @FindViewById(R.id.sample_text)
  private TextView tv;

  Logger log = LoggerFactory.getLogger(this.getClass());
  private WhisperService whisperService = Aop.get(WhisperService.class);

  @RequiresApi(api = Build.VERSION_CODES.O)
  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    //setContentView(R.layout.activity_main);
    ViewInjectUtils.injectActivity(this, this);
    showSystemInfo();
  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  @OnClick(R.id.loadModelBtn)
  public void loadModelBtn_OnClick(View v) {
    Context context = getBaseContext();
    whisperService.loadModel(context, tv);
  }

  @OnClick(R.id.transcriptSampleBtn)
  public void transcriptSampleBtn_OnClick(View v) {
    Context context = getBaseContext();
    whisperService.transcribeSample(context, tv);
  }


  @RequiresApi(api = Build.VERSION_CODES.O)
  @OnClick(R.id.systemInfoBtn)
  public void systemInfoBtn_OnClick(View v) {
    showSystemInfo();
  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  public void showSystemInfo() {
    String systemInfo = WhisperLib.getSystemInfo();
    tv.append(systemInfo + "\n");
  }

  @OnClick(R.id.clearBtn)
  public void clearBtn_OnClick(View v) {
    tv.setText("");
  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  @Override
  protected void onDestroy() {
    super.onDestroy();
    whisperService.release();
  }
}