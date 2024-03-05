package com.whispercppdemo.recorder

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import com.whispercppdemo.media.encodeWaveFile
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.withContext
import java.io.File
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean
import android.util.Log

import kotlinx.coroutines.runBlocking

private const val TAG = "Recorder"



class Recorder() {
    private val scope: CoroutineScope = CoroutineScope(
        Executors.newSingleThreadExecutor().asCoroutineDispatcher()
    )

    private var recorder: AudioRecordThread? = null
    private var audioStream: AudioStreamThread? = null


    suspend fun startRecording(outputFile: File, onError: (Exception) -> Unit) =
        withContext(scope.coroutineContext) {
            recorder = AudioRecordThread(outputFile, onError)
            recorder?.start()
        }

    fun startStreaming(onDataReceived: AudioDataReceivedListener, onError: (Exception) -> Unit) {
        if (audioStream == null) {
            audioStream = AudioStreamThread(onDataReceived, onError)
            audioStream?.start()
        } else {
            Log.i(TAG, "AudioStreamThread is already running")
        }
    }


    fun stopRecording() {
        recorder?.stopRecording()
        audioStream?.stopRecording()
        runBlocking {
            audioStream?.join()
            audioStream = null
            recorder?.join()
            recorder = null
        }
    }


    private class AudioRecordThread(
        private val outputFile: File,
        private val onError: (Exception) -> Unit
    ) :
        Thread("AudioRecorder") {
        private var quit = AtomicBoolean(false)

        @SuppressLint("MissingPermission")
        override fun run() {
            try {
                val bufferSize = AudioRecord.getMinBufferSize(
                    16000,
                    AudioFormat.CHANNEL_IN_MONO,
                    AudioFormat.ENCODING_PCM_16BIT
                ) * 4
                val buffer = ShortArray(bufferSize / 2)

                val audioRecord = AudioRecord(
                    MediaRecorder.AudioSource.MIC,
                    16000,
                    AudioFormat.CHANNEL_IN_MONO,
                    AudioFormat.ENCODING_PCM_16BIT,
                    bufferSize
                )

                try {
                    audioRecord.startRecording()

                    val allData = mutableListOf<Short>()

                    while (!quit.get()) {
                        val read = audioRecord.read(buffer, 0, buffer.size)
                        if (read > 0) {
                            for (i in 0 until read) {
                                allData.add(buffer[i])
                            }
                        } else {
                            throw java.lang.RuntimeException("audioRecord.read returned $read")
                        }
                    }

                    audioRecord.stop()
                    encodeWaveFile(
                        outputFile,
                        allData.toShortArray()
                    )
                } finally {
                    audioRecord.release()
                }
            } catch (e: Exception) {
                onError(e)
            }
        }

        fun stopRecording() {
            quit.set(true)
        }


    }

    interface AudioDataReceivedListener {
        fun onAudioDataReceived(data: FloatArray)
    }
    private class AudioStreamThread(
        private val onDataReceived: AudioDataReceivedListener,
        private val onError: (Exception) -> Unit
    ) : Thread("AudioStreamer") {
        private val quit = AtomicBoolean(false)

        @SuppressLint("MissingPermission")
        override fun run() {
            val bufferSize = AudioRecord.getMinBufferSize(
                16000,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_FLOAT) * 4
            val floatBuffer = FloatArray(bufferSize / 2)
            val audioRecord = AudioRecord(
                MediaRecorder.AudioSource.MIC,
                16000,
                AudioFormat.CHANNEL_IN_MONO,
                AudioFormat.ENCODING_PCM_FLOAT,
                bufferSize)

            if (audioRecord.state != AudioRecord.STATE_INITIALIZED) {
                Log.e(TAG, "AudioRecord initialization failed")
                return
            }

            try {
                audioRecord.startRecording()
                while (!quit.get()) {


                    val readResult = audioRecord.read(floatBuffer, 0, floatBuffer.size, AudioRecord.READ_BLOCKING)
                    Log.i(TAG, "readResult: $readResult")
                    if (readResult > 0) {
                        Log.i(TAG, "READING FROM THE floatBuffer")

                        onDataReceived.onAudioDataReceived(floatBuffer.copyOf(readResult))
                    } else if (readResult < 0) {
                        throw RuntimeException("AudioRecord.read error: $readResult")
                    }
                }
            } catch (e: Exception) {
                onError(e)
            } finally {
                audioRecord.stop()
                audioRecord.release()
            }
        }

        fun stopRecording() {
            quit.set(true)
        }
    }
}
