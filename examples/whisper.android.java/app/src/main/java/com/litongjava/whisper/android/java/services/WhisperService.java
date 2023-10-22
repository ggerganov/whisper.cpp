package com.litongjava.whisper.android.java.services;

import android.content.Context;
import android.os.Build;
import android.widget.TextView;

import androidx.annotation.RequiresApi;

import com.litongjava.jfinal.aop.AopManager;
import com.litongjava.whisper.android.java.bean.WhisperSegment;
import com.litongjava.whisper.android.java.single.LocalWhisper;
import com.litongjava.whisper.android.java.utils.AssetUtils;
import com.litongjava.whisper.android.java.utils.WaveEncoder;
import com.whispercppdemo.whisper.WhisperContext;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.concurrent.ExecutionException;

public class WhisperService {
  private Logger log = LoggerFactory.getLogger(this.getClass());

  private final Object lock = new Object();

  @RequiresApi(api = Build.VERSION_CODES.O)
  public void loadModel(Context context, TextView tv) {
    String modelFilePath = LocalWhisper.modelFilePath;
    String msg = "load model from :" + modelFilePath + "\n";
    outputMsg(tv, msg);

    long start = System.currentTimeMillis();
    LocalWhisper.INSTANCE.init();
    long end = System.currentTimeMillis();
    msg = "model load successful:" + (end - start) + "ms";
    outputMsg(tv, msg);

  }

  public void transcribeSample(Context context, TextView tv) {
    String msg = "";
    long start = System.currentTimeMillis();
    String sampleFilePath = "samples/jfk.wav";
    File filesDir = context.getFilesDir();
    File sampleFile = AssetUtils.copyFileIfNotExists(context, filesDir, sampleFilePath);
    long end = System.currentTimeMillis();
    msg = "copy file:" + (end - start) + "ms";
    outputMsg(tv, msg);

    msg = "transcribe file from :" + sampleFile.getAbsolutePath();
    outputMsg(tv, msg);

    start = System.currentTimeMillis();
    float[] audioData = new float[0];  // 读取音频样本
    try {
      audioData = WaveEncoder.decodeWaveFile(sampleFile);
    } catch (IOException e) {
      e.printStackTrace();
      return;
    }
    end = System.currentTimeMillis();
    msg = "decode wave file:" + (end - start) + "ms";
    outputMsg(tv, msg);

    start = System.currentTimeMillis();
    List<WhisperSegment> transcription = null;
    try {

      //transcription = LocalWhisper.INSTANCE.transcribeData(audioData);
      transcription = LocalWhisper.INSTANCE.transcribeDataWithTime(audioData);
    } catch (ExecutionException e) {
      e.printStackTrace();
    } catch (InterruptedException e) {
      e.printStackTrace();
    }
    end = System.currentTimeMillis();
    msg = "Transcript successful:" + (end - start) + "ms";
    outputMsg(tv, msg);

    msg = "Transcription:" + transcription.toString();
    outputMsg(tv, msg);
  }

  private void outputMsg(TextView tv, String msg) {
    tv.append(msg + "\n");
  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  public void release() {
    //noting to do
  }
}
