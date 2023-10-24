package com.litongjava.whisper.android.java.task;

import android.content.Context;
import android.os.Build;
import android.widget.TextView;

import com.blankj.utilcode.util.ThreadUtils;
import com.litongjava.jfinal.aop.Aop;
import com.litongjava.whisper.android.java.services.WhisperService;

import java.io.File;

public class LoadModelTask extends ThreadUtils.Task<Object> {
  private final Context context;
  private final TextView tv;
  public LoadModelTask(Context context,TextView tv) {
    this.tv = tv;
    this.context=context;
  }

  @Override
  public Object doInBackground() {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      Aop.get(WhisperService.class).loadModel(context,tv);
    }else{
      tv.append("not supported android devices");
    }
    return null;
  }

  @Override
  public void onSuccess(Object result) {
  }

  @Override
  public void onCancel() {
  }

  @Override
  public void onFail(Throwable t) {
  }
}