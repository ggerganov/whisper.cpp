#include "io_github_ggerganov_whispercpp_WhisperJNI.h"
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <string.h>
#include <whisper.h>
#include <ggml.h>
//#include "../../../../whisper.h"
//#include "../../../../ggml.h"

#define TAG "JNI"

#define LOGI(...) printf(__VA_ARGS__)
#define LOGW(...) printf(__VA_ARGS__)

static inline int min(int a, int b) {
    return (a < b) ? a : b;
}

static inline int max(int a, int b) {
    return (a > b) ? a : b;
}

struct input_stream_context {
    size_t offset;
    JNIEnv * env;
    jobject thiz;
    jobject input_stream;

    jmethodID mid_available;
    jmethodID mid_read;
};

size_t inputStreamRead(void * ctx, void * output, size_t read_size) {
    struct input_stream_context* is = (struct input_stream_context*)ctx;

    jint avail_size = (*is->env).CallIntMethod(is->input_stream, is->mid_available);
    jint size_to_copy = read_size < avail_size ? (jint)read_size : avail_size;

    jbyteArray byte_array = (*is->env).NewByteArray(size_to_copy);

    jint n_read = (*is->env).CallIntMethod(is->input_stream, is->mid_read, byte_array, 0, size_to_copy);

    if (size_to_copy != read_size || size_to_copy != n_read) {
        LOGI("Insufficient Read: Req=%zu, ToCopy=%d, Available=%d", read_size, size_to_copy, n_read);
    }

    jbyte* byte_array_elements = (*is->env).GetByteArrayElements(byte_array, NULL);
    memcpy(output, byte_array_elements, size_to_copy);
    (*is->env).ReleaseByteArrayElements(byte_array, byte_array_elements, JNI_ABORT);

    (*is->env).DeleteLocalRef(byte_array);

    is->offset += size_to_copy;

    return size_to_copy;
}

bool inputStreamEof(void * ctx) {
    struct input_stream_context* is = (struct input_stream_context*)ctx;

    jint result = (*is->env).CallIntMethod(is->input_stream, is->mid_available);
    return result <= 0;
}

void inputStreamClose(void * ctx) {

}

// Various functions for loading a ggml whisper model.
// Allocate (almost) all memory needed for the model.
// Return NULL on failure
JNIEXPORT jlong JNICALL Java_io_github_ggerganov_whispercpp_WhisperJNI_initContextFromInputStream
  (JNIEnv *env, jobject thiz, jobject input_stream) {
    struct whisper_context *context = NULL;
    struct whisper_model_loader loader = {};
    struct input_stream_context inp_ctx = {};

    inp_ctx.offset = 0;
    inp_ctx.env = env;
    inp_ctx.thiz = thiz;
    inp_ctx.input_stream = input_stream;

    jclass cls = env->GetObjectClass(input_stream);
    inp_ctx.mid_available = env->GetMethodID(cls, "available", "()I");
    inp_ctx.mid_read = env->GetMethodID(cls, "read", "([BII)I");

    loader.context = &inp_ctx;
    loader.read = inputStreamRead;
    loader.eof = inputStreamEof;
    loader.close = inputStreamClose;

    loader.eof(loader.context);

    context = whisper_init(&loader);
    return (jlong) context;
}

JNIEXPORT jlong JNICALL Java_io_github_ggerganov_whispercpp_WhisperJNI_initContext
  (JNIEnv *env, jobject thiz, jstring model_path_str) {
    struct whisper_context *context = NULL;
    const char *model_path_chars = env->GetStringUTFChars(model_path_str, NULL);
    context = whisper_init_from_file(model_path_chars);
    env->ReleaseStringUTFChars(model_path_str, model_path_chars);
    return (jlong) context;
}

JNIEXPORT void JNICALL Java_io_github_ggerganov_whispercpp_WhisperJNI_freeContext
  (JNIEnv *env, jobject thiz, jlong context_ptr) {
    struct whisper_context *context = (struct whisper_context *) context_ptr;
    whisper_free(context);
}

JNIEXPORT void JNICALL Java_io_github_ggerganov_whispercpp_WhisperJNI_fullTranscribe
  (JNIEnv *env, jobject thiz, jlong context_ptr, jfloatArray audio_data) {
      struct whisper_context *context = (struct whisper_context *) context_ptr;
      jfloat *audio_data_arr = env->GetFloatArrayElements(audio_data, NULL);
      const jsize audio_data_length = env->GetArrayLength(audio_data);

      // Leave 2 processors free (i.e. the high-efficiency cores).
      int max_threads = max(1, min(8, get_nprocs() - 2));
      LOGI("Selecting %d threads", max_threads);

      // The below adapted from the Objective-C iOS sample
      struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
      params.print_realtime = true;
      params.print_progress = false;
      params.print_timestamps = true;
      params.print_special = false;
      params.translate = false;
      params.language = "en";
      params.n_threads = max_threads;
      params.offset_ms = 0;
      params.no_context = true;
      params.single_segment = false;

      whisper_reset_timings(context);

      LOGI("About to run whisper_full");
      if (whisper_full(context, params, audio_data_arr, audio_data_length) != 0) {
          LOGI("Failed to run the model");
      } else {
          whisper_print_timings(context);
      }
      env->ReleaseFloatArrayElements(audio_data, audio_data_arr, JNI_ABORT);
}

JNIEXPORT jint JNICALL Java_io_github_ggerganov_whispercpp_WhisperJNI_getTextSegmentCount
  (JNIEnv *env, jobject thiz, jlong context_ptr) {
    struct whisper_context *context = (struct whisper_context *) context_ptr;
    return whisper_full_n_segments(context);
}

JNIEXPORT jstring JNICALL Java_io_github_ggerganov_whispercpp_WhisperJNI_getTextSegment
  (JNIEnv *env, jobject thiz, jlong context_ptr, jint index) {
    struct whisper_context *context = (struct whisper_context *) context_ptr;
    const char *text = whisper_full_get_segment_text(context, index);
    return env->NewStringUTF(text);
}

JNIEXPORT jstring JNICALL Java_io_github_ggerganov_whispercpp_WhisperJNI_getSystemInfo
  (JNIEnv *env, jobject thiz) {
    const char *sysinfo = whisper_print_system_info();
    return env->NewStringUTF(sysinfo);
}

JNIEXPORT jstring JNICALL Java_io_github_ggerganov_whispercpp_WhisperJNI_benchMemcpy
  (JNIEnv *env, jobject thiz, jint n_threads) {
    const char *bench_ggml_memcpy = whisper_bench_memcpy_str(n_threads);
    return env->NewStringUTF(bench_ggml_memcpy);
}

JNIEXPORT jstring JNICALL Java_io_github_ggerganov_whispercpp_WhisperJNI_benchGgmlMulMat
  (JNIEnv *env, jobject thiz, jint n_threads) {
    const char *bench_ggml_mul_mat = whisper_bench_ggml_mul_mat_str(n_threads);
    return env->NewStringUTF(bench_ggml_mul_mat);
}
