package com.whispercpp.java.whisper;

import android.app.Activity;
import android.content.Context;
import android.os.Build;
import android.os.Environment;
import android.text.TextUtils;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;


public class CDEUtils {
    private final static String TAG = CDEUtils.class.getName();

    public static String getDataPath(Context context) {
        return new StringBuilder("/data/data/").append(context.getPackageName()).append("/").toString();
    }


    public static String getDataPath() {
        String state = Environment.getExternalStorageState();
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP_MR1) {
            CDELog.j(TAG, "Android version > API22：Android 5.1 (Android L)Lollipop_MR1");
            if (!state.equals(Environment.MEDIA_MOUNTED)) {
                CDELog.d(TAG, "can't read/write extern device");
                return null;
            }
        } else {
            CDELog.j(TAG, "Android version <= API22：Android 5.1 (Android L)Lollipop");
        }

        File sdcardPath = Environment.getExternalStorageDirectory();
        CDELog.j(TAG, "sdcardPath name:" + sdcardPath.getName() + ",sdcardPath:" + sdcardPath.getPath());

        String dataDirectoryPath = null;
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP_MR1) {
            dataDirectoryPath = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separator;
        } else {
            dataDirectoryPath = Environment.getExternalStorageDirectory().getAbsolutePath() +
                    File.separator + "Android" + File.separator + "data" + File.separator + "com.litongjava.whisper.android.java" + File.separator + "files" + File.separator;
        }

        File directoryPath = new File(dataDirectoryPath);
        if (!directoryPath.exists()) {
            CDELog.j(TAG, "create dir: " + dataDirectoryPath);
            directoryPath.mkdirs();
        } else {
            CDELog.j(TAG, dataDirectoryPath + " already exist");
        }


        return dataDirectoryPath;
    }


    public static void dumpDeviceInfo() {
        CDELog.j(TAG, "******** Android build informations******\n");
        addBuildField("BOARD", Build.BOARD);
        addBuildField("BOOTLOADER", Build.BOOTLOADER);
        addBuildField("BRAND", Build.BRAND);
        addBuildField("CPU_ABI", Build.CPU_ABI);
        addBuildField("DEVICE", Build.DEVICE);
        addBuildField("DISPLAY", Build.DISPLAY);
        addBuildField("FINGERPRINT", Build.FINGERPRINT);
        addBuildField("HARDWARE", Build.HARDWARE);
        addBuildField("HOST", Build.HOST);
        addBuildField("ID", Build.ID);
        addBuildField("MANUFACTURER", Build.MANUFACTURER);
        addBuildField("MODEL", Build.MODEL);
        addBuildField("PRODUCT", Build.PRODUCT);
        addBuildField("SERIAL", Build.SERIAL);
        addBuildField("TAGS", Build.TAGS);
        addBuildField("TYPE", Build.TYPE);
        addBuildField("USER", Build.USER);
        addBuildField("ANDROID SDK", String.valueOf(Build.VERSION.SDK_INT));
        addBuildField("OS Version", Build.VERSION.RELEASE);
        CDELog.j(TAG, "******************************************\n");

    }


    private static void addBuildField(String name, String value) {
        CDELog.j(TAG, "  " + name + ": " + value + "\n");
    }


    public static String bytesToBinaryString(byte[] var0) {
        String var1 = "";

        for (int var2 = 0; var2 < var0.length; ++var2) {
            byte var3 = var0[var2];

            for (int var4 = 0; var4 < 8; ++var4) {
                int var5 = var3 >>> var4 & 1;
                var1 = var1 + var5;
            }

            if (var2 != var0.length - 1) {
                var1 = var1 + " ";
            }
        }

        return var1;
    }


    public static String bytesToHexString(byte[] src) {
        StringBuilder stringBuilder = new StringBuilder("");
        if (src == null || src.length <= 0) {
            return null;
        }
        for (int i = 0; i < src.length; i++) {
            int v = src[i] & 0xFF;
            String hv = Integer.toHexString(v);
            if (hv.length() < 2) {
                stringBuilder.append(0);
            }
            stringBuilder.append(hv);
        }
        return stringBuilder.toString();
    }


    public static String intsToHexString(int[] src) {
        StringBuilder stringBuilder = new StringBuilder("");
        if (src == null || src.length <= 0) {
            return null;
        }
        for (int i = 0; i < src.length; i++) {
            String hv = Integer.toHexString(src[i]);
            stringBuilder.append("0x" + hv + " ");
        }
        return stringBuilder.toString();
    }


    private static byte charToByte(char c) {
        return (byte) "0123456789ABCDEF".indexOf(c);
    }


    public static byte[] hexStringToBytes(String hexString) {
        if (hexString == null || hexString.equals("")) {
            return null;
        }
        hexString = hexString.toUpperCase();
        int length = hexString.length() / 2;
        char[] hexChars = hexString.toCharArray();
        byte[] d = new byte[length];
        for (int i = 0; i < length; i++) {
            int pos = i * 2;
            d[i] = (byte) (charToByte(hexChars[pos]) << 4 | charToByte(hexChars[pos + 1]));
        }
        return d;
    }


    public static void printHexString(byte[] b) {
        for (int i = 0; i < b.length; i++) {
            String hex = Integer.toHexString(b[i] & 0xFF);
            if (hex.length() == 1) {
                hex = '0' + hex;
            }
            System.out.print(hex.toUpperCase());
        }

    }


    public static boolean isSdCardAvailable() {
        String dataDirectoryPath = null;
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.LOLLIPOP_MR1) {
            dataDirectoryPath = Environment.getExternalStorageDirectory().getAbsolutePath() + File.separator + "whisper";
        } else {
            dataDirectoryPath = Environment.getExternalStorageDirectory().getAbsolutePath() +
                    File.separator + "Android" + File.separator + "data" + File.separator + "com.litongjava.whisper.android.java" + File.separator + "files" + File.separator + "whisper";
        }

        String state = Environment.getExternalStorageState();
        return (!TextUtils.isEmpty(state) && state.equals("mounted") && Environment.getExternalStorageDirectory() != null);
    }


    public static void copyAssetFile(Context context, String sourceFilePath, String destFilePath) {
        InputStream inStream = null;
        FileOutputStream outStream = null;
        try {
            inStream = context.getAssets().open(sourceFilePath);
            outStream = new FileOutputStream(destFilePath);

            int bytesRead = 0;
            byte[] buf = new byte[4096];
            while ((bytesRead = inStream.read(buf)) != -1) {
                outStream.write(buf, 0, bytesRead);
            }
        } catch (Exception e) {
            CDELog.d(TAG, "error: " + e.toString());
            e.printStackTrace();
        } finally {
            close(inStream);
            close(outStream);
        }
    }


    public static void copyFile(Context context, String sourceFilePath, String destFilePath) {
        InputStream inStream = null;
        FileOutputStream outStream = null;
        try {
            CDELog.j(TAG, "src:" + sourceFilePath);
            CDELog.j(TAG, "dst:" + destFilePath);
            File newfile = new File(destFilePath);
            if (!newfile.exists())
                newfile.createNewFile();
            inStream = new FileInputStream(sourceFilePath);
            outStream = new FileOutputStream(destFilePath);

            int bytesRead = 0;
            byte[] buf = new byte[4096];
            while ((bytesRead = inStream.read(buf)) != -1) {
                outStream.write(buf, 0, bytesRead);
            }
        } catch (Exception e) {
            CDELog.d(TAG, "error: " + e.toString());
            e.printStackTrace();
        } finally {
            close(inStream);
            close(outStream);
        }
    }

    public static void close(InputStream is) {
        if (is == null)
            return;

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
        }
    }


    @Deprecated
    public static void setProp(String command) {
        try {
            Process process = Runtime.getRuntime().exec(command);
            InputStreamReader inputStreamReader = new InputStreamReader(process.getInputStream());
            BufferedReader bufferedReader = new BufferedReader(inputStreamReader);
            if (bufferedReader.readLine() == null) {
                CDELog.j(TAG, command + " success");
            } else {
                CDELog.j(TAG, command + " failed");
                CDELog.j(TAG, "command result:" + bufferedReader.readLine());
            }
        } catch (IOException e) {
            e.printStackTrace();
            CDELog.j(TAG, "error: " + e.toString());
        }
    }

    @Deprecated
    public static int getProp(String command) {
        try {
            Process process = Runtime.getRuntime().exec(command);
            InputStreamReader inputStreamReader = new InputStreamReader(process.getInputStream());
            BufferedReader bufferedReader = new BufferedReader(inputStreamReader);
            if (bufferedReader == null) {
                CDELog.j(TAG, command + " failed");
                return -1;
            }
            String commandResult = bufferedReader.readLine();
            if (commandResult == null) {
                CDELog.j(TAG, command + " failed");
                return -1;
            } else {
                CDELog.j(TAG, "command result: " + commandResult);
                CDELog.j(TAG, command + " return: " + Integer.parseInt(commandResult));
                return Integer.parseInt(commandResult);
            }
        } catch (IOException e) {
            e.printStackTrace();
            CDELog.j(TAG, "error: " + e.toString());
            return -1;
        }
    }


    public static void exitAPK(Activity activity) {
        if (Build.VERSION.SDK_INT >= 16 && Build.VERSION.SDK_INT < 21) {
            activity.finishAffinity();
        } else if (Build.VERSION.SDK_INT >= 21) {
            activity.finishAffinity();
        } else {
            activity.finish();
        }
        System.exit(0);
    }
}
