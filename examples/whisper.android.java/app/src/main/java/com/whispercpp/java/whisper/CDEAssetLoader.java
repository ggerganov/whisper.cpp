package com.whispercpp.java.whisper;

import android.content.Context;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class CDEAssetLoader {
    private static final String TAG = CDEAssetLoader.class.getName();

    public static String getDataPath(Context context) {
        return new StringBuilder("/data/data/").append(context.getPackageName()).append("/").toString();
    }

    public static void copyAssetFile(Context context, String srcFilePath, String destFilePath) {
        CDELog.j(TAG, "src path: " + srcFilePath);
        CDELog.j(TAG, "dst path: " + destFilePath);
        InputStream inStream = null;
        FileOutputStream outStream = null;
        try {
            File destFile = new File(destFilePath);
            if (!destFile.exists()) {
                destFile.createNewFile();
            }
            inStream = context.getAssets().open(srcFilePath);
            outStream = new FileOutputStream(destFilePath);

            int bytesRead = 0;
            byte[] buf = new byte[2048];
            while ((bytesRead = inStream.read(buf)) != -1) {
                outStream.write(buf, 0, bytesRead);
            }
        } catch (Exception e) {
            CDELog.j(TAG, "error: " + e.toString());
            e.printStackTrace();
        } finally {
            close(inStream);
            close(outStream);
        }
    }

    public static void close(InputStream is) {
        if (is == null) return;
        try {
            is.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static void close(OutputStream os) {
        if (os == null) return;
        try {
            os.close();
        } catch (Exception e) {
            e.printStackTrace();
            CDELog.j(TAG, "error: " + e.toString());
        }
    }

    public static String readTextFromFile(String fileName) {
        File file = new File(fileName);
        BufferedReader reader = null;
        StringBuffer sbf = new StringBuffer();
        try {
            reader = new BufferedReader(new FileReader(file));
            String tempStr;
            while ((tempStr = reader.readLine()) != null) {
                sbf.append(tempStr);
            }
            reader.close();
            return sbf.toString();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            if (reader != null) {
                try {
                    reader.close();
                } catch (IOException e) {
                    e.printStackTrace();
                    CDELog.j(TAG, "error: " + e.toString());
                }
            }
        }
        return sbf.toString();
    }
}
