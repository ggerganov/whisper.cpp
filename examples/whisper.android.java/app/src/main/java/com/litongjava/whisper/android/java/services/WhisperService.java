package com.litongjava.whisper.android.java.services;

import android.content.Context;
import android.os.Build;
import android.widget.TextView;

import androidx.annotation.RequiresApi;

import com.litongjava.jfinal.aop.AopManager;
import com.litongjava.whisper.android.java.utils.AssetUtils;
import com.litongjava.whisper.android.java.utils.WaveEncoder;
import com.whispercppdemo.whisper.WhisperContext;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;
import java.util.concurrent.ExecutionException;

public class WhisperService {
  private Logger log = LoggerFactory.getLogger(this.getClass());
  private WhisperContext whisperContext;
  private final Object lock = new Object();

  @RequiresApi(api = Build.VERSION_CODES.O)
  public void loadModel(Context context, TextView tv) {

    File filesDir = context.getFilesDir();
    String modelFilePath = "models/ggml-tiny.bin";
    File modelFile = AssetUtils.copyFileIfNotExists(context, filesDir, modelFilePath);
    modelFilePath = modelFile.getAbsolutePath();

    String msg = "load model from :" + modelFilePath + "\n";
    log.info(msg);
    tv.append(msg);
    if (whisperContext == null) {
      long start = System.currentTimeMillis();

      synchronized (lock){
        whisperContext = WhisperContext.createContextFromFile(modelFilePath);
      }

//      AopManager.me().addSingletonObject(whisperContext);
      long end = System.currentTimeMillis();
      msg = "model load successful:" + (end - start) + "ms\n";
      log.info(msg);
      tv.append(msg);
    } else {
      msg = "model loaded\n";
      log.info(msg);
      tv.append(msg);
    }
  }

  public void transcribeSample(Context context, TextView tv) {
    String sampleFilePath = "samples/jfk.wav";
    File filesDir = context.getFilesDir();
    File sampleFile = AssetUtils.copyFileIfNotExists(context, filesDir, sampleFilePath);
    log.info("transcribe file from :{}", sampleFile.getAbsolutePath());
    float[] audioData = new float[0];  // 读取音频样本
    try {
      audioData = WaveEncoder.decodeWaveFile(sampleFile);
    } catch (IOException e) {
      e.printStackTrace();
      return;
    }

    String transcription = null;  // 转录音频数据

    String msg = "";
    try {
      if (whisperContext == null) {
        msg = "please load model or wait model loaded";
        log.info(msg);

      } else {
        long start = System.currentTimeMillis();
        synchronized (whisperContext){
          transcription = whisperContext.transcribeData(audioData);
        }
        long end = System.currentTimeMillis();
        msg = "Transcript successful:" + (end - start) + "ms";
        log.info(msg);
        tv.append(msg + "\n");

        msg = "Transcription:" + transcription;
        log.info(msg);
        tv.append(msg + "\n");
      }


    } catch (ExecutionException e) {
      e.printStackTrace();
    } catch (InterruptedException e) {
      e.printStackTrace();
    }


  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  public void release() {
    if (whisperContext != null) {
      try {
        whisperContext.release();
      } catch (ExecutionException e) {
        e.printStackTrace();
      } catch (InterruptedException e) {
        e.printStackTrace();
      }
      whisperContext = null;
    }
  }
}
