package com.litongjava.whisper.android.java.services;

import android.content.Context;
import android.os.Build;
import android.os.Handler;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.RequiresApi;

import com.blankj.utilcode.util.ToastUtils;
import com.blankj.utilcode.util.Utils;
import com.litongjava.android.utils.dialog.AlertDialogUtils;
import com.litongjava.jfinal.aop.Aop;
import com.litongjava.whisper.android.java.bean.WhisperSegment;
import com.litongjava.whisper.android.java.single.LocalWhisper;
import com.litongjava.whisper.android.java.utils.WaveEncoder;
import com.whispercpp.java.whisper.CDELog;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.concurrent.ExecutionException;

public class WhisperService {
  private Logger log = LoggerFactory.getLogger(this.getClass());
  private static final String TAG = WhisperService.class.getName();
  private final Object lock = new Object();

  @RequiresApi(api = Build.VERSION_CODES.O)
  public void loadModel(TextView tv) {
    String modelFilePath = LocalWhisper.modelFilePath;
    String msg = "load model from :" + modelFilePath + "\n";
    CDELog.j(TAG, msg);
    outputMsg(tv, msg);

    long start = System.currentTimeMillis();
    LocalWhisper.INSTANCE.init();
    long end = System.currentTimeMillis();
    msg = "model load successful and cost:" + (end - start) + "ms";
    CDELog.j(TAG, msg);
    outputMsg(tv, msg);
    ToastUtils.showLong(msg);

  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  public void transcribeSample(TextView tv, File sampleFile) {
    String msg = "";
    msg = "transcribe file from :" + sampleFile.getAbsolutePath();
    outputMsg(tv, msg);
    CDELog.j(TAG, msg);

    Long start = System.currentTimeMillis();
    float[] audioData = new float[0];  // 读取音频样本
    try {
      audioData = WaveEncoder.decodeWaveFile(sampleFile);
    } catch (IOException e) {
      CDELog.j(TAG, "exception:" + e.toString());
      e.printStackTrace();
      return;
    }
    long end = System.currentTimeMillis();
    msg = "decode wave file:" + (end - start) + "ms";
    outputMsg(tv, msg);
    CDELog.j(TAG, msg);

    start = System.currentTimeMillis();
    List<WhisperSegment> transcription = null;
    try {
      //transcription = LocalWhisper.INSTANCE.transcribeData(audioData);
      CDELog.j(TAG, "start transcribe data");
      msg = "start transcribe audio data";
      outputMsg(tv, msg);
      transcription = LocalWhisper.INSTANCE.transcribeDataWithTime(audioData);
      CDELog.j(TAG, "after start transcribe data");
    } catch (ExecutionException e) {
      e.printStackTrace();
      CDELog.j(TAG, "exception:" + e.toString());
    } catch (InterruptedException e) {
      e.printStackTrace();
      CDELog.j(TAG, "exception:" + e.toString());
    }
    end = System.currentTimeMillis();
    if(transcription!=null){
      ToastUtils.showLong(transcription.toString());
      msg = "Transcript successful and cost:" + (end - start) + "ms";
      outputMsg(tv, msg);
      CDELog.j(TAG, msg);

      outputMsg(tv, transcription.toString());
      CDELog.j(TAG, transcription.toString());

    }else{
      msg = "Transcript failed:" + (end - start) + "ms";
      outputMsg(tv, msg);
      CDELog.j(TAG, msg);
    }

  }

  private void outputMsg(TextView tv, String msg) {
    log.info(msg);
    if(tv!=null){
      Aop.get(Handler.class).post(()->{ tv.append(msg + "\n");});
    }
  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  public void release() {
    //noting to do
  }
}
