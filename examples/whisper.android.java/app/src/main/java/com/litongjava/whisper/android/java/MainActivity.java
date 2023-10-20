package com.litongjava.whisper.android.java;

import androidx.annotation.RequiresApi;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Build;
import android.os.Bundle;
import android.widget.TextView;

import com.whispercppdemo.whisper.WhisperLib;

public class MainActivity extends AppCompatActivity {


  @RequiresApi(api = Build.VERSION_CODES.O)
  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);

    // Example of a call to a native method
    TextView tv = findViewById(R.id.sample_text);
    String systemInfo = WhisperLib.getSystemInfo();
    tv.setText(systemInfo);

  }


}