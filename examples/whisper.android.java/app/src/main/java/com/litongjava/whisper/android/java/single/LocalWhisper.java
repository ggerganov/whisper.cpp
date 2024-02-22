package com.litongjava.whisper.android.java.single;

import android.app.Application;
import android.os.Build;
import android.os.Handler;

import androidx.annotation.RequiresApi;

import com.blankj.utilcode.util.ToastUtils;
import com.blankj.utilcode.util.Utils;
import com.litongjava.jfinal.aop.Aop;
import com.litongjava.whisper.android.java.bean.WhisperSegment;
import com.litongjava.whisper.android.java.utils.AssetUtils;
import com.whispercpp.java.whisper.CDEAssetLoader;
import com.whispercpp.java.whisper.CDELog;
import com.whispercpp.java.whisper.CDEUtils;
import com.whispercpp.java.whisper.WhisperContext;

import java.io.File;
import java.util.List;
import java.util.concurrent.ExecutionException;


@RequiresApi(api = Build.VERSION_CODES.O)
public enum LocalWhisper {
  INSTANCE;

  public static final String modelFilePath = "models/ggml-tiny.en.bin";
  //public static final String modelFilePath = "models/ggml-base.bin";

  private WhisperContext whisperContext;

  @RequiresApi(api = Build.VERSION_CODES.O)
  LocalWhisper() {
    String TAG = LocalWhisper.class.getName();
    Application context = Utils.getApp();
    File filesDir = context.getFilesDir();
    CDELog.j(TAG, "files dir:" + filesDir.getAbsolutePath());
    //File modelFile = AssetUtils.copyFileIfNotExists(context, filesDir, modelFilePath);
    File directoryPath = new File(filesDir.getAbsolutePath() + File.separator + "models");
    if (!directoryPath.exists()) {
      CDELog.j(TAG, "create dir: " + directoryPath);
      directoryPath.mkdirs();
    } else {
      CDELog.j(TAG, directoryPath + " already exist");
    }
    CDEUtils.copyFile(context, CDEUtils.getDataPath() + modelFilePath, filesDir.getAbsolutePath() + File.separator + modelFilePath);
    String realModelFilePath = filesDir.getAbsolutePath() + File.separator + modelFilePath;
    //String realModelFilePath = CDEUtils.getDataPath() + modelFilePath;
    CDELog.j(TAG, "realModelFilePath:" + realModelFilePath);
    whisperContext = WhisperContext.createContextFromFile(realModelFilePath);
  }

  public synchronized String transcribeData(float[] data) throws ExecutionException, InterruptedException {
    if(whisperContext==null){
        toastModelLoading();
        return null;
    }else{
      return whisperContext.transcribeData(data);
    }
  }

    private static void toastModelLoading() {
        Aop.get(Handler.class).post(()->{
          ToastUtils.showShort("please wait for model loading");
        });
    }

    public List<WhisperSegment> transcribeDataWithTime(float[] audioData) throws ExecutionException, InterruptedException {
    if(whisperContext==null){
        toastModelLoading();
      return null;
    }else{
      return whisperContext.transcribeDataWithTime(audioData);
    }
  }

  public void init() {
    //noting to do.but init
  }


}
