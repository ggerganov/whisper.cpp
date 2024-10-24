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

static ID id_to_s;
static ID id_call;
static ID id___method__;
static ID id_to_enum;

/*
 * call-seq:
 *   lang_max_id -> Integer
 */
static VALUE ruby_whisper_s_lang_max_id(VALUE self) {
  return INT2NUM(whisper_lang_max_id());
}

/*
 * call-seq:
 *   lang_id(lang_name) -> Integer
 */
static VALUE ruby_whisper_s_lang_id(VALUE self, VALUE lang) {
  const char * lang_str = StringValueCStr(lang);
  const int id = whisper_lang_id(lang_str);
  if (-1 == id) {
    rb_raise(rb_eArgError, "language not found: %s", lang_str);
  }
  return INT2NUM(id);
}

/*
 * call-seq:
 *   lang_str(lang_id) -> String
 */
static VALUE ruby_whisper_s_lang_str(VALUE self, VALUE id) {
  const int lang_id = NUM2INT(id);
  const char * str = whisper_lang_str(lang_id);
  if (nullptr == str) {
    rb_raise(rb_eIndexError, "id %d outside of language id", lang_id);
  }
  return rb_str_new2(str);
}

/*
 * call-seq:
 *   lang_str(lang_id) -> String
 */
static VALUE ruby_whisper_s_lang_str_full(VALUE self, VALUE id) {
  const int lang_id = NUM2INT(id);
  const char * str_full = whisper_lang_str_full(lang_id);
  if (nullptr == str_full) {
    rb_raise(rb_eIndexError, "id %d outside of language id", lang_id);
  }
  return rb_str_new2(str_full);
}

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
  rb_gc_mark(rwp->new_segment_callback_container->user_data);
  rb_gc_mark(rwp->new_segment_callback_container->callback);
  rb_gc_mark(rwp->new_segment_callback_container->callbacks);
}

void rb_whisper_params_free(ruby_whisper_params *rwp) {
  // How to free user_data and callback only when not referred to by others?
  ruby_whisper_params_free(rwp);
  free(rwp);
}

static VALUE ruby_whisper_allocate(VALUE klass) {
  ruby_whisper *rw;
  rw = ALLOC(ruby_whisper);
  rw->context = NULL;
  return Data_Wrap_Struct(klass, rb_whisper_mark, rb_whisper_free, rw);
}

static ruby_whisper_callback_container * rb_whisper_callback_container_allocate() {
  ruby_whisper_callback_container *container;
  container = ALLOC(ruby_whisper_callback_container);
  container->context = nullptr;
  container->user_data = Qnil;
  container->callback = Qnil;
  container->callbacks = rb_ary_new();
  return container;
}

static VALUE ruby_whisper_params_allocate(VALUE klass) {
  ruby_whisper_params *rwp;
  rwp = ALLOC(ruby_whisper_params);
  rwp->params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  rwp->new_segment_callback_container = rb_whisper_callback_container_allocate();
  return Data_Wrap_Struct(klass, rb_whisper_params_mark, rb_whisper_params_free, rwp);
}

/*
 * call-seq:
 *   new("path/to/model.bin") -> Whisper::Context
 */
static VALUE ruby_whisper_initialize(int argc, VALUE *argv, VALUE self) {
  ruby_whisper *rw;
  VALUE whisper_model_file_path;

  // TODO: we can support init from buffer here too maybe another ruby object to expose
  rb_scan_args(argc, argv, "01", &whisper_model_file_path);
  Data_Get_Struct(self, ruby_whisper, rw);

  if (!rb_respond_to(whisper_model_file_path, id_to_s)) {
    rb_raise(rb_eRuntimeError, "Expected file path to model to initialize Whisper::Context");
  }
  rw->context = whisper_init_from_file_with_params(StringValueCStr(whisper_model_file_path), whisper_context_default_params());
  if (rw->context == nullptr) {
    rb_raise(rb_eRuntimeError, "error: failed to initialize whisper context");
  }
  return self;
}

// High level API
static VALUE rb_whisper_segment_initialize(VALUE context, int index);

/*
 * transcribe a single file
 * can emit to a block results
 *
 *   params = Whisper::Params.new
 *   params.duration = 60_000
 *   whisper.transcribe "path/to/audio.wav", params do |text|
 *     puts text
 *   end
 *
 * call-seq:
 *   transcribe(path_to_audio, params) {|text| ...}
 **/
static VALUE ruby_whisper_transcribe(int argc, VALUE *argv, VALUE self) {
  ruby_whisper *rw;
  ruby_whisper_params *rwp;
  VALUE wave_file_path, blk, params;

  rb_scan_args(argc, argv, "02&", &wave_file_path, &params, &blk);
  Data_Get_Struct(self, ruby_whisper, rw);
  Data_Get_Struct(params, ruby_whisper_params, rwp);

  if (!rb_respond_to(wave_file_path, id_to_s)) {
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

  if (!NIL_P(rwp->new_segment_callback_container->callback) || 0 != RARRAY_LEN(rwp->new_segment_callback_container->callbacks)) {
    rwp->params.new_segment_callback = [](struct whisper_context * ctx, struct whisper_state * state, int n_new, void * user_data) {
      const ruby_whisper_callback_container *container = (ruby_whisper_callback_container *)user_data;

      // Currently, doesn't support state because
      // those require to resolve GC-related problems.
      if (!NIL_P(container->callback)) {
        rb_funcall(container->callback, id_call, 4, *container->context, Qnil, INT2NUM(n_new), container->user_data);
      }
      const long callbacks_len = RARRAY_LEN(container->callbacks);
      if (0 == callbacks_len) {
        return;
      }
      const int n_segments = whisper_full_n_segments_from_state(state);
      for (int i = n_new; i > 0; i--) {
        int i_segment = n_segments - i;
        VALUE segment = rb_whisper_segment_initialize(*container->context, i_segment);
        for (int j = 0; j < callbacks_len; j++) {
          VALUE cb = rb_ary_entry(container->callbacks, j);
          rb_funcall(cb, id_call, 1, segment);
        }
      }
    };
    rwp->new_segment_callback_container->context = &self;
    rwp->params.new_segment_callback_user_data = rwp->new_segment_callback_container;
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
  VALUE idCall = id_call;
  if (blk != Qnil) {
    rb_funcall(blk, idCall, 1, output);
  }
  return self;
}

/*
 * Number of segments.
 *
 * call-seq:
 *   full_n_segments -> Integer
 */
static VALUE ruby_whisper_full_n_segments(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_full_n_segments(rw->context));
}

/*
 * Language ID, which can be converted to string by Whisper.lang_str and Whisper.lang_str_full.
 *
 * call-seq:
 *   full_lang_id -> Integer
 */
static VALUE ruby_whisper_full_lang_id(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_full_lang_id(rw->context));
}

static int ruby_whisper_full_check_segment_index(const ruby_whisper * rw, const VALUE i_segment) {
  const int c_i_segment = NUM2INT(i_segment);
  if (c_i_segment < 0 || c_i_segment >= whisper_full_n_segments(rw->context)) {
    rb_raise(rb_eIndexError, "segment index %d out of range", c_i_segment);
  }
  return c_i_segment;
}

/*
 * Start time of a segment indexed by +segment_index+ in centiseconds (10 times milliseconds).
 *
 *   full_get_segment_t0(3) # => 1668 (16680 ms)
 *
 * call-seq:
 *   full_get_segment_t0(segment_index) -> Integer
 */
static VALUE ruby_whisper_full_get_segment_t0(VALUE self, VALUE i_segment) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  const int c_i_segment = ruby_whisper_full_check_segment_index(rw, i_segment);
  const int64_t t0 = whisper_full_get_segment_t0(rw->context, c_i_segment);
  return INT2NUM(t0);
}

/*
 * End time of a segment indexed by +segment_index+ in centiseconds (10 times milliseconds).
 *
 *   full_get_segment_t1(3) # => 1668 (16680 ms)
 *
 * call-seq:
 *   full_get_segment_t1(segment_index) -> Integer
 */
static VALUE ruby_whisper_full_get_segment_t1(VALUE self, VALUE i_segment) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  const int c_i_segment = ruby_whisper_full_check_segment_index(rw, i_segment);
  const int64_t t1 = whisper_full_get_segment_t1(rw->context, c_i_segment);
  return INT2NUM(t1);
}

/*
 * Whether the next segment indexed by +segment_index+ is predicated as a speaker turn.
 *
 *   full_get_segment_speacker_turn_next(3) # => true
 *
 * call-seq:
 *   full_get_segment_speacker_turn_next(segment_index) -> bool
 */
static VALUE ruby_whisper_full_get_segment_speaker_turn_next(VALUE self, VALUE i_segment) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  const int c_i_segment = ruby_whisper_full_check_segment_index(rw, i_segment);
  const bool speaker_turn_next = whisper_full_get_segment_speaker_turn_next(rw->context, c_i_segment);
  return speaker_turn_next ? Qtrue : Qfalse;
}

/*
 * Text of a segment indexed by +segment_index+.
 *
 *   full_get_segment_text(3) # => "ask not what your country can do for you, ..."
 *
 * call-seq:
 *   full_get_segment_text(segment_index) -> String
 */
static VALUE ruby_whisper_full_get_segment_text(VALUE self, VALUE i_segment) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  const int c_i_segment = ruby_whisper_full_check_segment_index(rw, i_segment);
  const char * text = whisper_full_get_segment_text(rw->context, c_i_segment);
  return rb_str_new2(text);
}

/*
 * params.language = "auto" | "en", etc...
 *
 * call-seq:
 *   language = lang_name -> lang_name
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
/*
 * call-seq:
 *   language -> String
 */
static VALUE ruby_whisper_params_get_language(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  if (rwp->params.language) {
    return rb_str_new2(rwp->params.language);
  } else {
    return rb_str_new2("auto");
  }
}
/*
 * call-seq:
 *   translate = do_translate -> do_translate
 */
static VALUE ruby_whisper_params_set_translate(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, translate, value)
}
/*
 * call-seq:
 *   translate -> bool
 */
static VALUE ruby_whisper_params_get_translate(VALUE self) {
  BOOL_PARAMS_GETTER(self, translate)
}
/*
 * call-seq:
 *   no_context = dont_use_context -> dont_use_context
 */
static VALUE ruby_whisper_params_set_no_context(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, no_context, value)
}
/*
 * If true, does not use past transcription (if any) as initial prompt for the decoder.
 *
 * call-seq:
 *   no_context -> bool
 */
static VALUE ruby_whisper_params_get_no_context(VALUE self) {
  BOOL_PARAMS_GETTER(self, no_context)
}
/*
 * call-seq:
 *   single_segment = force_single -> force_single
 */
static VALUE ruby_whisper_params_set_single_segment(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, single_segment, value)
}
/*
 * If true, forces single segment output (useful for streaming).
 *
 * call-seq:
 *   single_segment -> bool
 */
static VALUE ruby_whisper_params_get_single_segment(VALUE self) {
  BOOL_PARAMS_GETTER(self, single_segment)
}
/*
 * call-seq:
 *   print_special = force_print -> force_print
 */
static VALUE ruby_whisper_params_set_print_special(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, print_special, value)
}
/*
 * If true, prints special tokens (e.g. <SOT>, <EOT>, <BEG>, etc.).
 *
 * call-seq:
 *   print_special -> bool
 */
static VALUE ruby_whisper_params_get_print_special(VALUE self) {
  BOOL_PARAMS_GETTER(self, print_special)
}
/*
 * call-seq:
 *   print_progress = force_print -> force_print
 */
static VALUE ruby_whisper_params_set_print_progress(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, print_progress, value)
}
/*
 * If true, prints progress information.
 *
 * call-seq:
 *   print_progress -> bool
 */
static VALUE ruby_whisper_params_get_print_progress(VALUE self) {
  BOOL_PARAMS_GETTER(self, print_progress)
}
/*
 * call-seq:
 *   print_realtime = force_print -> force_print
 */
static VALUE ruby_whisper_params_set_print_realtime(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, print_realtime, value)
}
/*
 * If true, prints results from within whisper.cpp. (avoid it, use callback instead)
 * call-seq:
 *   print_realtime -> bool
 */
static VALUE ruby_whisper_params_get_print_realtime(VALUE self) {
  BOOL_PARAMS_GETTER(self, print_realtime)
}
/*
 * call-seq:
 *   print_timestamps = force_print -> force_print
 */
static VALUE ruby_whisper_params_set_print_timestamps(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, print_timestamps, value)
}
/*
 * If true, prints timestamps for each text segment when printing realtime.
 *
 * call-seq:
 *   print_timestamps -> bool
 */
static VALUE ruby_whisper_params_get_print_timestamps(VALUE self) {
  BOOL_PARAMS_GETTER(self, print_timestamps)
}
/*
 * call-seq:
 *   suppress_blank = force_suppress -> force_suppress
 */
static VALUE ruby_whisper_params_set_suppress_blank(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, suppress_blank, value)
}
/*
 * If true, suppresses blank outputs.
 *
 * call-seq:
 *   suppress_blank -> bool
 */
static VALUE ruby_whisper_params_get_suppress_blank(VALUE self) {
  BOOL_PARAMS_GETTER(self, suppress_blank)
}
/*
 * call-seq:
 *   suppress_non_speech_tokens = force_suppress -> force_suppress
 */
static VALUE ruby_whisper_params_set_suppress_non_speech_tokens(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, suppress_non_speech_tokens, value)
}
/*
 * If true, suppresses non-speech-tokens.
 *
 * call-seq:
 *   suppress_non_speech_tokens -> bool
 */
static VALUE ruby_whisper_params_get_suppress_non_speech_tokens(VALUE self) {
  BOOL_PARAMS_GETTER(self, suppress_non_speech_tokens)
}
/*
 * If true, enables token-level timestamps.
 *
 * call-seq:
 *   token_timestamps -> bool
 */
static VALUE ruby_whisper_params_get_token_timestamps(VALUE self) {
  BOOL_PARAMS_GETTER(self, token_timestamps)
}
/*
 * call-seq:
 *   token_timestamps = force_timestamps -> force_timestamps
 */
static VALUE ruby_whisper_params_set_token_timestamps(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, token_timestamps, value)
}
/*
 * If true, split on word rather than on token (when used with max_len).
 *
 * call-seq:
 *   translate -> bool
 */
static VALUE ruby_whisper_params_get_split_on_word(VALUE self) {
  BOOL_PARAMS_GETTER(self, split_on_word)
}
/*
 * call-seq:
 *   split_on_word = force_split -> force_split
 */
static VALUE ruby_whisper_params_set_split_on_word(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, split_on_word, value)
}
/*
 * If true, enables diarization.
 *
 * call-seq:
 *   diarize -> bool
 */
static VALUE ruby_whisper_params_get_diarize(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  if (rwp->diarize) {
    return Qtrue;
  } else {
    return Qfalse;
  }
}
/*
 * call-seq:
 *   diarize = force_diarize -> force_diarize
 */
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

/*
 * Start offset in ms.
 *
 * call-seq:
 *   offset -> Integer
 */
static VALUE ruby_whisper_params_get_offset(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return INT2NUM(rwp->params.offset_ms);
}
/*
 * call-seq:
 *   offset = offset_ms -> offset_ms
 */
static VALUE ruby_whisper_params_set_offset(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.offset_ms = NUM2INT(value);
  return value;
}
/*
 * Audio duration to process in ms.
 *
 * call-seq:
 *   duration -> Integer
 */
static VALUE ruby_whisper_params_get_duration(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return INT2NUM(rwp->params.duration_ms);
}
/*
 * call-seq:
 *   duration = duration_ms -> duration_ms
 */
static VALUE ruby_whisper_params_set_duration(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.duration_ms = NUM2INT(value);
  return value;
}

/*
 * Max tokens to use from past text as prompt for the decoder.
 *
 * call-seq:
 *   max_text_tokens -> Integer
 */
static VALUE ruby_whisper_params_get_max_text_tokens(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return INT2NUM(rwp->params.n_max_text_ctx);
}
/*
 * call-seq:
 *   max_text_tokens = n_tokens -> n_tokens
 */
static VALUE ruby_whisper_params_set_max_text_tokens(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.n_max_text_ctx = NUM2INT(value);
  return value;
}
/*
 * Sets new segment callback, called for every newly generated text segment.
 *
 *   params.new_segment_callback = ->(context, _, n_new, user_data) {
 *     # ...
 *   }
 *
 * call-seq:
 *   new_segment_callback = callback -> callback
 */
static VALUE ruby_whisper_params_set_new_segment_callback(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->new_segment_callback_container->callback = value;
  return value;
}
/*
 * Sets user data passed to the last argument of new segment callback.
 *
 * call-seq:
 *   new_segment_callback_user_data = user_data -> use_data
 */
static VALUE ruby_whisper_params_set_new_segment_callback_user_data(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->new_segment_callback_container->user_data = value;
  return value;
}

// High level API

typedef struct {
  VALUE context;
  int index;
} ruby_whisper_segment;

VALUE cSegment;

static void rb_whisper_segment_mark(ruby_whisper_segment *rws) {
  rb_gc_mark(rws->context);
}

static VALUE ruby_whisper_segment_allocate(VALUE klass) {
  ruby_whisper_segment *rws;
  rws = ALLOC(ruby_whisper_segment);
  return Data_Wrap_Struct(klass, rb_whisper_segment_mark, RUBY_DEFAULT_FREE, rws);
}

static VALUE rb_whisper_segment_initialize(VALUE context, int index) {
  ruby_whisper_segment *rws;
  const VALUE segment = ruby_whisper_segment_allocate(cSegment);
  Data_Get_Struct(segment, ruby_whisper_segment, rws);
  rws->context = context;
  rws->index = index;
  return segment;
};

/*
 * Yields each Whisper::Segment:
 *
 *   whisper.transcribe("path/to/audio.wav", params)
 *   whisper.each_segment do |segment|
 *     puts segment.text
 *   end
 *
 * Returns an Enumerator if no block given:
 *
 *   whisper.transcribe("path/to/audio.wav", params)
 *   enum = whisper.each_segment
 *   enum.to_a # => [#<Whisper::Segment>, ...]
 *
 * call-seq:
 *   each_segment {|segment| ... }
 *   each_segment -> Enumerator
 */
static VALUE ruby_whisper_each_segment(VALUE self) {
  if (!rb_block_given_p()) {
    const VALUE method_name = rb_funcall(self, id___method__, 0);
    return rb_funcall(self, id_to_enum, 1, method_name);
  }

  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);

  const int n_segments = whisper_full_n_segments(rw->context);
  for (int i = 0; i < n_segments; ++i) {
    rb_yield(rb_whisper_segment_initialize(self, i));
  }

  return self;
}

/*
 * Hook called on new segment. Yields each Whisper::Segment.
 *
 *   whisper.on_new_segment do |segment|
 *     # ...
 *   end
 *
 * call-seq:
 *   on_new_segment {|segment| ... }
 */
static VALUE ruby_whisper_params_on_new_segment(VALUE self) {
  ruby_whisper_params *rws;
  Data_Get_Struct(self, ruby_whisper_params, rws);
  const VALUE blk = rb_block_proc();
  rb_ary_push(rws->new_segment_callback_container->callbacks, blk);
  return Qnil;
}

/*
 * Start time in milliseconds.
 *
 * call-seq:
 *   start_time -> Integer
 */
static VALUE ruby_whisper_segment_get_start_time(VALUE self) {
  ruby_whisper_segment *rws;
  Data_Get_Struct(self, ruby_whisper_segment, rws);
  ruby_whisper *rw;
  Data_Get_Struct(rws->context, ruby_whisper, rw);
  const int64_t t0 = whisper_full_get_segment_t0(rw->context, rws->index);
  // able to multiply 10 without overflow because to_timestamp() in whisper.cpp does it
  return INT2NUM(t0 * 10);
}

/*
 * End time in milliseconds.
 *
 * call-seq:
 *   end_time -> Integer
 */
static VALUE ruby_whisper_segment_get_end_time(VALUE self) {
  ruby_whisper_segment *rws;
  Data_Get_Struct(self, ruby_whisper_segment, rws);
  ruby_whisper *rw;
  Data_Get_Struct(rws->context, ruby_whisper, rw);
  const int64_t t1 = whisper_full_get_segment_t1(rw->context, rws->index);
  // able to multiply 10 without overflow because to_timestamp() in whisper.cpp does it
  return INT2NUM(t1 * 10);
}

/*
 * Whether the next segment is predicted as a speaker turn.
 *
 * call-seq:
 *   speaker_turn_next? -> bool
 */
static VALUE ruby_whisper_segment_get_speaker_turn_next(VALUE self) {
  ruby_whisper_segment *rws;
  Data_Get_Struct(self, ruby_whisper_segment, rws);
  ruby_whisper *rw;
  Data_Get_Struct(rws->context, ruby_whisper, rw);
  return whisper_full_get_segment_speaker_turn_next(rw->context, rws->index) ? Qtrue : Qfalse;
}

/*
 * call-seq:
 *   text -> String
 */
static VALUE ruby_whisper_segment_get_text(VALUE self) {
  ruby_whisper_segment *rws;
  Data_Get_Struct(self, ruby_whisper_segment, rws);
  ruby_whisper *rw;
  Data_Get_Struct(rws->context, ruby_whisper, rw);
  const char * text = whisper_full_get_segment_text(rw->context, rws->index);
  return rb_str_new2(text);
}

void Init_whisper() {
  id_to_s = rb_intern("to_s");
  id_call = rb_intern("call");
  id___method__ = rb_intern("__method__");
  id_to_enum = rb_intern("to_enum");

  mWhisper = rb_define_module("Whisper");
  cContext = rb_define_class_under(mWhisper, "Context", rb_cObject);
  cParams  = rb_define_class_under(mWhisper, "Params", rb_cObject);

  rb_define_singleton_method(mWhisper, "lang_max_id", ruby_whisper_s_lang_max_id, 0);
  rb_define_singleton_method(mWhisper, "lang_id", ruby_whisper_s_lang_id, 1);
  rb_define_singleton_method(mWhisper, "lang_str", ruby_whisper_s_lang_str, 1);
  rb_define_singleton_method(mWhisper, "lang_str_full", ruby_whisper_s_lang_str_full, 1);

  rb_define_alloc_func(cContext, ruby_whisper_allocate);
  rb_define_method(cContext, "initialize", ruby_whisper_initialize, -1);

  rb_define_method(cContext, "transcribe", ruby_whisper_transcribe, -1);
  rb_define_method(cContext, "full_n_segments", ruby_whisper_full_n_segments, 0);
  rb_define_method(cContext, "full_lang_id", ruby_whisper_full_lang_id, 0);
  rb_define_method(cContext, "full_get_segment_t0", ruby_whisper_full_get_segment_t0, 1);
  rb_define_method(cContext, "full_get_segment_t1", ruby_whisper_full_get_segment_t1, 1);
  rb_define_method(cContext, "full_get_segment_speaker_turn_next", ruby_whisper_full_get_segment_speaker_turn_next, 1);
  rb_define_method(cContext, "full_get_segment_text", ruby_whisper_full_get_segment_text, 1);

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
  rb_define_method(cParams, "diarize", ruby_whisper_params_get_diarize, 0);
  rb_define_method(cParams, "diarize=", ruby_whisper_params_set_diarize, 1);

  rb_define_method(cParams, "offset", ruby_whisper_params_get_offset, 0);
  rb_define_method(cParams, "offset=", ruby_whisper_params_set_offset, 1);
  rb_define_method(cParams, "duration", ruby_whisper_params_get_duration, 0);
  rb_define_method(cParams, "duration=", ruby_whisper_params_set_duration, 1);

  rb_define_method(cParams, "max_text_tokens", ruby_whisper_params_get_max_text_tokens, 0);
  rb_define_method(cParams, "max_text_tokens=", ruby_whisper_params_set_max_text_tokens, 1);

  rb_define_method(cParams, "new_segment_callback=", ruby_whisper_params_set_new_segment_callback, 1);
  rb_define_method(cParams, "new_segment_callback_user_data=", ruby_whisper_params_set_new_segment_callback_user_data, 1);

  // High leve
  cSegment  = rb_define_class_under(mWhisper, "Segment", rb_cObject);

  rb_define_alloc_func(cSegment, ruby_whisper_segment_allocate);
  rb_define_method(cContext, "each_segment", ruby_whisper_each_segment, 0);
  rb_define_method(cParams, "on_new_segment", ruby_whisper_params_on_new_segment, 0);
  rb_define_method(cSegment, "start_time", ruby_whisper_segment_get_start_time, 0);
  rb_define_method(cSegment, "end_time", ruby_whisper_segment_get_end_time, 0);
  rb_define_method(cSegment, "speaker_next_turn?", ruby_whisper_segment_get_speaker_turn_next, 0);
  rb_define_method(cSegment, "text", ruby_whisper_segment_get_text, 0);
}
#ifdef __cplusplus
}
#endif
