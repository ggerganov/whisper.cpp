#include <ruby.h>
#include "ruby_whisper.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include <cmath>
#include <fstream>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

#define BOOL_PARAMS_SETTER(self, prop, value) \
  ruby_whisper_params *rwp; \
  Data_Get_Struct(self, ruby_whisper_params, rwp); \
  if (value == Qfalse || value == Qnil) { \
    rwp->params.prop = false; \
  } else { \
    rwp->params.prop = true; \
  } \
  return value; \

#define BOOL_PARAMS_GETTER(self,  prop) \
  ruby_whisper_params *rwp; \
  Data_Get_Struct(self, ruby_whisper_params, rwp); \
  if (rwp->params.prop) { \
    return Qtrue; \
  } else { \
    return Qfalse; \
  }

VALUE mWhisper;
VALUE cContext;
VALUE cParams;

static void ruby_whisper_free(ruby_whisper *rw) {
  if (rw->context) {
    whisper_free(rw->context);
    rw->context = NULL;
  }
}
static void ruby_whisper_params_free(ruby_whisper_params *rwp) {
}

void rb_whisper_mark(ruby_whisper *rw) {
  // call rb_gc_mark on any ruby references in rw
}

void rb_whisper_free(ruby_whisper *rw) {
  ruby_whisper_free(rw);
  free(rw);
}

void rb_whisper_params_mark(ruby_whisper_params *rwp) {
}

void rb_whisper_params_free(ruby_whisper_params *rwp) {
  ruby_whisper_params_free(rwp);
  free(rwp);
}

static VALUE ruby_whisper_allocate(VALUE klass) {
  ruby_whisper *rw;
  rw = ALLOC(ruby_whisper);
  rw->context = NULL;
  return Data_Wrap_Struct(klass, rb_whisper_mark, rb_whisper_free, rw);
}

static VALUE ruby_whisper_params_allocate(VALUE klass) {
  ruby_whisper_params *rwp;
  rwp = ALLOC(ruby_whisper_params);
  rwp->params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  return Data_Wrap_Struct(klass, rb_whisper_params_mark, rb_whisper_params_free, rwp);
}

static VALUE ruby_whisper_initialize(int argc, VALUE *argv, VALUE self) {
  ruby_whisper *rw;
  VALUE whisper_model_file_path;

  // TODO: we can support init from buffer here too maybe another ruby object to expose
  rb_scan_args(argc, argv, "01", &whisper_model_file_path);
  Data_Get_Struct(self, ruby_whisper, rw);

  if (!rb_respond_to(whisper_model_file_path, rb_intern("to_s"))) {
    rb_raise(rb_eRuntimeError, "Expected file path to model to initialize Whisper::Context");
  }
  rw->context = whisper_init_from_file(StringValueCStr(whisper_model_file_path));
  if (rw->context == nullptr) {
    rb_raise(rb_eRuntimeError, "error: failed to initialize whisper context");
  }
  return self;
}

/*
 * transcribe a single file
 * can emit to a block results
 *
 **/
static VALUE ruby_whisper_transcribe(int argc, VALUE *argv, VALUE self) {
  ruby_whisper *rw;
  ruby_whisper_params *rwp;
  VALUE wave_file_path, blk, params;

  rb_scan_args(argc, argv, "02&", &wave_file_path, &params, &blk);
  Data_Get_Struct(self, ruby_whisper, rw);
  Data_Get_Struct(params, ruby_whisper_params, rwp);

  if (!rb_respond_to(wave_file_path, rb_intern("to_s"))) {
    rb_raise(rb_eRuntimeError, "Expected file path to wave file");
  }

  std::string fname_inp = StringValueCStr(wave_file_path);

  std::vector<float> pcmf32; // mono-channel F32 PCM
  std::vector<std::vector<float>> pcmf32s; // stereo-channel F32 PCM

  // WAV input - this is directly from main.cpp example
  {
    drwav wav;
    std::vector<uint8_t> wav_data; // used for pipe input from stdin

    if (fname_inp == "-") {
      {
        uint8_t buf[1024];
        while (true) {
          const size_t n = fread(buf, 1, sizeof(buf), stdin);
          if (n == 0) {
            break;
          }
          wav_data.insert(wav_data.end(), buf, buf + n);
        }
      }

      if (drwav_init_memory(&wav, wav_data.data(), wav_data.size(), nullptr) == false) {
        fprintf(stderr, "error: failed to open WAV file from stdin\n");
        return self;
      }

      fprintf(stderr, "%s: read %zu bytes from stdin\n", __func__, wav_data.size());
    } else if (drwav_init_file(&wav, fname_inp.c_str(), nullptr) == false) {
      fprintf(stderr, "error: failed to open '%s' as WAV file\n", fname_inp.c_str());
      return self;
    }

    if (wav.channels != 1 && wav.channels != 2) {
      fprintf(stderr, "WAV file '%s' must be mono or stereo\n", fname_inp.c_str());
      return self;
    }

    if (rwp->diarize && wav.channels != 2 && rwp->params.print_timestamps == false) {
      fprintf(stderr, "WAV file '%s' must be stereo for diarization and timestamps have to be enabled\n", fname_inp.c_str());
      return self;
    }

    if (wav.sampleRate != WHISPER_SAMPLE_RATE) {
      fprintf(stderr, "WAV file '%s' must be %i kHz\n", fname_inp.c_str(), WHISPER_SAMPLE_RATE/1000);
      return self;
    }

    if (wav.bitsPerSample != 16) {
      fprintf(stderr, "WAV file '%s' must be 16-bit\n", fname_inp.c_str());
      return self;
    }

    const uint64_t n = wav_data.empty() ? wav.totalPCMFrameCount : wav_data.size()/(wav.channels*wav.bitsPerSample/8);

    std::vector<int16_t> pcm16;
    pcm16.resize(n*wav.channels);
    drwav_read_pcm_frames_s16(&wav, n, pcm16.data());
    drwav_uninit(&wav);

    // convert to mono, float
    pcmf32.resize(n);
    if (wav.channels == 1) {
      for (uint64_t i = 0; i < n; i++) {
        pcmf32[i] = float(pcm16[i])/32768.0f;
      }
    } else {
      for (uint64_t i = 0; i < n; i++) {
        pcmf32[i] = float(pcm16[2*i] + pcm16[2*i + 1])/65536.0f;
      }
    }

    if (rwp->diarize) {
      // convert to stereo, float
      pcmf32s.resize(2);

      pcmf32s[0].resize(n);
      pcmf32s[1].resize(n);
      for (uint64_t i = 0; i < n; i++) {
        pcmf32s[0][i] = float(pcm16[2*i])/32768.0f;
        pcmf32s[1][i] = float(pcm16[2*i + 1])/32768.0f;
      }
    }
  }
  {
    static bool is_aborted = false; // NOTE: this should be atomic to avoid data race

    rwp->params.encoder_begin_callback = [](struct whisper_context * /*ctx*/, struct whisper_state * /*state*/, void * user_data) {
      bool is_aborted = *(bool*)user_data;
      return !is_aborted;
    };
    rwp->params.encoder_begin_callback_user_data = &is_aborted;
  }

  if (whisper_full_parallel(rw->context, rwp->params, pcmf32.data(), pcmf32.size(), 1) != 0) {
    fprintf(stderr, "failed to process audio\n");
    return self;
  }
  const int n_segments = whisper_full_n_segments(rw->context);
  VALUE output = rb_str_new2("");
  for (int i = 0; i < n_segments; ++i) {
    const char * text = whisper_full_get_segment_text(rw->context, i);
    output = rb_str_concat(output, rb_str_new2(text));
  }
  VALUE idCall = rb_intern("call");
  if (blk != Qnil) {
    rb_funcall(blk, idCall, 1, output);
  }
  return self;
}

/*
 * params.language = "auto" | "en", etc...
 */
static VALUE ruby_whisper_params_set_language(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  if (value == Qfalse || value == Qnil) {
    rwp->params.language = "auto";
  } else {
    rwp->params.language = StringValueCStr(value);
  }
  return value;
}
static VALUE ruby_whisper_params_get_language(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  if (rwp->params.language) {
    return rb_str_new2(rwp->params.language);
  } else {
    return rb_str_new2("auto");
  }
}
static VALUE ruby_whisper_params_set_translate(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, translate, value)
}
static VALUE ruby_whisper_params_get_translate(VALUE self) {
  BOOL_PARAMS_GETTER(self, translate)
}
static VALUE ruby_whisper_params_set_no_context(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, no_context, value)
}
static VALUE ruby_whisper_params_get_no_context(VALUE self) {
  BOOL_PARAMS_GETTER(self, no_context)
}
static VALUE ruby_whisper_params_set_single_segment(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, single_segment, value)
}
static VALUE ruby_whisper_params_get_single_segment(VALUE self) {
  BOOL_PARAMS_GETTER(self, single_segment)
}
static VALUE ruby_whisper_params_set_print_special(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, print_special, value)
}
static VALUE ruby_whisper_params_get_print_special(VALUE self) {
  BOOL_PARAMS_GETTER(self, print_special)
}
static VALUE ruby_whisper_params_set_print_progress(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, print_progress, value)
}
static VALUE ruby_whisper_params_get_print_progress(VALUE self) {
  BOOL_PARAMS_GETTER(self, print_progress)
}
static VALUE ruby_whisper_params_set_print_realtime(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, print_realtime, value)
}
static VALUE ruby_whisper_params_get_print_realtime(VALUE self) {
  BOOL_PARAMS_GETTER(self, print_realtime)
}
static VALUE ruby_whisper_params_set_print_timestamps(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, print_timestamps, value)
}
static VALUE ruby_whisper_params_get_print_timestamps(VALUE self) {
  BOOL_PARAMS_GETTER(self, print_timestamps)
}
static VALUE ruby_whisper_params_set_suppress_blank(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, suppress_blank, value)
}
static VALUE ruby_whisper_params_get_suppress_blank(VALUE self) {
  BOOL_PARAMS_GETTER(self, suppress_blank)
}
static VALUE ruby_whisper_params_set_suppress_non_speech_tokens(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, suppress_non_speech_tokens, value)
}
static VALUE ruby_whisper_params_get_suppress_non_speech_tokens(VALUE self) {
  BOOL_PARAMS_GETTER(self, suppress_non_speech_tokens)
}
static VALUE ruby_whisper_params_get_token_timestamps(VALUE self) {
  BOOL_PARAMS_GETTER(self, token_timestamps)
}
static VALUE ruby_whisper_params_set_token_timestamps(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, token_timestamps, value)
}
static VALUE ruby_whisper_params_get_split_on_word(VALUE self) {
  BOOL_PARAMS_GETTER(self, split_on_word)
}
static VALUE ruby_whisper_params_set_split_on_word(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, split_on_word, value)
}
static VALUE ruby_whisper_params_get_speed_up(VALUE self) {
  BOOL_PARAMS_GETTER(self, speed_up)
}
static VALUE ruby_whisper_params_set_speed_up(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, speed_up, value)
}
static VALUE ruby_whisper_params_get_diarize(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  if (rwp->diarize) {
    return Qtrue;
  } else {
    return Qfalse;
  }
}
static VALUE ruby_whisper_params_set_diarize(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  if (value == Qfalse || value == Qnil) {
    rwp->diarize = false;
  } else {
    rwp->diarize = true;
  } \
  return value;
}

static VALUE ruby_whisper_params_get_offset(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return INT2NUM(rwp->params.offset_ms);
}
static VALUE ruby_whisper_params_set_offset(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.offset_ms = NUM2INT(value);
  return value;
}
static VALUE ruby_whisper_params_get_duration(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return INT2NUM(rwp->params.duration_ms);
}
static VALUE ruby_whisper_params_set_duration(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.duration_ms = NUM2INT(value);
  return value;
}

static VALUE ruby_whisper_params_get_max_text_tokens(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return INT2NUM(rwp->params.n_max_text_ctx);
}
static VALUE ruby_whisper_params_set_max_text_tokens(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.n_max_text_ctx = NUM2INT(value);
  return value;
}

void Init_whisper() {
  mWhisper = rb_define_module("Whisper");
  cContext = rb_define_class_under(mWhisper, "Context", rb_cObject);
  cParams  = rb_define_class_under(mWhisper, "Params", rb_cObject);

  rb_define_alloc_func(cContext, ruby_whisper_allocate);
  rb_define_method(cContext, "initialize", ruby_whisper_initialize, -1);

  rb_define_method(cContext, "transcribe", ruby_whisper_transcribe, -1);

  rb_define_alloc_func(cParams, ruby_whisper_params_allocate);

  rb_define_method(cParams, "language=", ruby_whisper_params_set_language, 1);
  rb_define_method(cParams, "language", ruby_whisper_params_get_language, 0);
  rb_define_method(cParams, "translate=", ruby_whisper_params_set_translate, 1);
  rb_define_method(cParams, "translate", ruby_whisper_params_get_translate, 0);
  rb_define_method(cParams, "no_context=", ruby_whisper_params_set_no_context, 1);
  rb_define_method(cParams, "no_context", ruby_whisper_params_get_no_context, 0);
  rb_define_method(cParams, "single_segment=", ruby_whisper_params_set_single_segment, 1);
  rb_define_method(cParams, "single_segment", ruby_whisper_params_get_single_segment, 0);
  rb_define_method(cParams, "print_special", ruby_whisper_params_get_print_special, 0);
  rb_define_method(cParams, "print_special=", ruby_whisper_params_set_print_special, 1);
  rb_define_method(cParams, "print_progress", ruby_whisper_params_get_print_progress, 0);
  rb_define_method(cParams, "print_progress=", ruby_whisper_params_set_print_progress, 1);
  rb_define_method(cParams, "print_realtime", ruby_whisper_params_get_print_realtime, 0);
  rb_define_method(cParams, "print_realtime=", ruby_whisper_params_set_print_realtime, 1);
  rb_define_method(cParams, "print_timestamps", ruby_whisper_params_get_print_timestamps, 0);
  rb_define_method(cParams, "print_timestamps=", ruby_whisper_params_set_print_timestamps, 1);
  rb_define_method(cParams, "suppress_blank", ruby_whisper_params_get_suppress_blank, 0);
  rb_define_method(cParams, "suppress_blank=", ruby_whisper_params_set_suppress_blank, 1);
  rb_define_method(cParams, "suppress_non_speech_tokens", ruby_whisper_params_get_suppress_non_speech_tokens, 0);
  rb_define_method(cParams, "suppress_non_speech_tokens=", ruby_whisper_params_set_suppress_non_speech_tokens, 1);
  rb_define_method(cParams, "token_timestamps", ruby_whisper_params_get_token_timestamps, 0);
  rb_define_method(cParams, "token_timestamps=", ruby_whisper_params_set_token_timestamps, 1);
  rb_define_method(cParams, "split_on_word", ruby_whisper_params_get_split_on_word, 0);
  rb_define_method(cParams, "split_on_word=", ruby_whisper_params_set_split_on_word, 1);
  rb_define_method(cParams, "speed_up", ruby_whisper_params_get_speed_up, 0);
  rb_define_method(cParams, "speed_up=", ruby_whisper_params_set_speed_up, 1);
  rb_define_method(cParams, "diarize", ruby_whisper_params_get_diarize, 0);
  rb_define_method(cParams, "diarize=", ruby_whisper_params_set_diarize, 1);

  rb_define_method(cParams, "offset", ruby_whisper_params_get_offset, 0);
  rb_define_method(cParams, "offset=", ruby_whisper_params_set_offset, 1);
  rb_define_method(cParams, "duration", ruby_whisper_params_get_duration, 0);
  rb_define_method(cParams, "duration=", ruby_whisper_params_set_duration, 1);

  rb_define_method(cParams, "max_text_tokens", ruby_whisper_params_get_max_text_tokens, 0);
  rb_define_method(cParams, "max_text_tokens=", ruby_whisper_params_set_max_text_tokens, 1);
}
#ifdef __cplusplus
}
#endif
