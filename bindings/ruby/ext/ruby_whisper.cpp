#include <ruby.h>
#include <ruby/memory_view.h>
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
VALUE eError;

VALUE cSegment;
VALUE cModel;

static ID id_to_s;
static ID id_call;
static ID id___method__;
static ID id_to_enum;
static ID id_length;
static ID id_next;
static ID id_new;
static ID id_to_path;
static ID id_URI;
static ID id_pre_converted_models;

static bool is_log_callback_finalized = false;

// High level API
static VALUE rb_whisper_segment_initialize(VALUE context, int index);

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

static VALUE ruby_whisper_s_finalize_log_callback(VALUE self, VALUE id) {
  is_log_callback_finalized = true;
  return Qnil;
}

/*
 * call-seq:
 *   log_set ->(level, buffer, user_data) { ... }, user_data -> nil
 */
static VALUE ruby_whisper_s_log_set(VALUE self, VALUE log_callback, VALUE user_data) {
  VALUE old_callback = rb_iv_get(self, "log_callback");
  if (!NIL_P(old_callback)) {
    rb_undefine_finalizer(old_callback);
  }

  rb_iv_set(self, "log_callback", log_callback);
  rb_iv_set(self, "user_data", user_data);

  VALUE finalize_log_callback = rb_funcall(mWhisper, rb_intern("method"), 1, rb_str_new2("finalize_log_callback"));
  rb_define_finalizer(log_callback, finalize_log_callback);

  whisper_log_set([](ggml_log_level level, const char * buffer, void * user_data) {
    if (is_log_callback_finalized) {
      return;
    }
    VALUE log_callback = rb_iv_get(mWhisper, "log_callback");
    VALUE udata = rb_iv_get(mWhisper, "user_data");
    rb_funcall(log_callback, id_call, 3, INT2NUM(level), rb_str_new2(buffer), udata);
  }, nullptr);

  return Qnil;
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

void rb_whisper_callbcack_container_mark(ruby_whisper_callback_container *rwc) {
  rb_gc_mark(rwc->user_data);
  rb_gc_mark(rwc->callback);
  rb_gc_mark(rwc->callbacks);
}

void rb_whisper_params_mark(ruby_whisper_params *rwp) {
  rb_whisper_callbcack_container_mark(rwp->new_segment_callback_container);
  rb_whisper_callbcack_container_mark(rwp->progress_callback_container);
  rb_whisper_callbcack_container_mark(rwp->abort_callback_container);
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

static void new_segment_callback(struct whisper_context *ctx, struct whisper_state *state, int n_new, void *user_data) {
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
}

static void progress_callback(struct whisper_context *ctx, struct whisper_state *state, int progress_cur, void *user_data) {
  const ruby_whisper_callback_container *container = (ruby_whisper_callback_container *)user_data;
  const VALUE progress = INT2NUM(progress_cur);
  // Currently, doesn't support state because
  // those require to resolve GC-related problems.
  if (!NIL_P(container->callback)) {
    rb_funcall(container->callback, id_call, 4, *container->context, Qnil, progress, container->user_data);
  }
  const long callbacks_len = RARRAY_LEN(container->callbacks);
  if (0 == callbacks_len) {
    return;
  }
  for (int j = 0; j < callbacks_len; j++) {
    VALUE cb = rb_ary_entry(container->callbacks, j);
    rb_funcall(cb, id_call, 1, progress);
  }
}

static bool abort_callback(void * user_data) {
  const ruby_whisper_callback_container *container = (ruby_whisper_callback_container *)user_data;
  if (!NIL_P(container->callback)) {
    VALUE result = rb_funcall(container->callback, id_call, 1, container->user_data);
    if (!NIL_P(result) && Qfalse != result) {
      return true;
    }
  }
  const long callbacks_len = RARRAY_LEN(container->callbacks);
  if (0 == callbacks_len) {
    return false;
  }
  for (int j = 0; j < callbacks_len; j++) {
    VALUE cb = rb_ary_entry(container->callbacks, j);
    VALUE result = rb_funcall(cb, id_call, 1, container->user_data);
    if (!NIL_P(result) && Qfalse != result) {
      return true;
    }
  }
  return false;
}

static VALUE ruby_whisper_params_allocate(VALUE klass) {
  ruby_whisper_params *rwp;
  rwp = ALLOC(ruby_whisper_params);
  rwp->params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  rwp->diarize = false;
  rwp->new_segment_callback_container = rb_whisper_callback_container_allocate();
  rwp->progress_callback_container = rb_whisper_callback_container_allocate();
  rwp->abort_callback_container = rb_whisper_callback_container_allocate();
  return Data_Wrap_Struct(klass, rb_whisper_params_mark, rb_whisper_params_free, rwp);
}

/*
 * call-seq:
 *   new("base.en") -> Whisper::Context
 *   new("path/to/model.bin") -> Whisper::Context
 *   new(Whisper::Model::URI.new("https://example.net/uri/of/model.bin")) -> Whisper::Context
 */
static VALUE ruby_whisper_initialize(int argc, VALUE *argv, VALUE self) {
  ruby_whisper *rw;
  VALUE whisper_model_file_path;

  // TODO: we can support init from buffer here too maybe another ruby object to expose
  rb_scan_args(argc, argv, "01", &whisper_model_file_path);
  Data_Get_Struct(self, ruby_whisper, rw);

  VALUE pre_converted_models = rb_funcall(cModel, id_pre_converted_models, 0);
  VALUE pre_converted_model = rb_hash_aref(pre_converted_models, whisper_model_file_path);
  if (!NIL_P(pre_converted_model)) {
    whisper_model_file_path = pre_converted_model;
  }
  if (TYPE(whisper_model_file_path) == T_STRING) {
    const char * whisper_model_file_path_str = StringValueCStr(whisper_model_file_path);
    if (strncmp("http://", whisper_model_file_path_str, 7) == 0 || strncmp("https://", whisper_model_file_path_str, 8) == 0) {
      VALUE uri_class = rb_const_get(cModel, id_URI);
      whisper_model_file_path = rb_class_new_instance(1, &whisper_model_file_path, uri_class);
    }
  }
  if (rb_obj_is_kind_of(whisper_model_file_path, rb_path2class("URI::HTTP"))) {
    VALUE uri_class = rb_const_get(cModel, id_URI);
    whisper_model_file_path = rb_class_new_instance(1, &whisper_model_file_path, uri_class);
  }
  if (rb_respond_to(whisper_model_file_path, id_to_path)) {
    whisper_model_file_path = rb_funcall(whisper_model_file_path, id_to_path, 0);
  }
  if (!rb_respond_to(whisper_model_file_path, id_to_s)) {
    rb_raise(rb_eRuntimeError, "Expected file path to model to initialize Whisper::Context");
  }
  rw->context = whisper_init_from_file_with_params(StringValueCStr(whisper_model_file_path), whisper_context_default_params());
  if (rw->context == nullptr) {
    rb_raise(rb_eRuntimeError, "error: failed to initialize whisper context");
  }
  return self;
}

static void register_callbacks(ruby_whisper_params * rwp, VALUE * self) {
  if (!NIL_P(rwp->new_segment_callback_container->callback) || 0 != RARRAY_LEN(rwp->new_segment_callback_container->callbacks)) {
    rwp->new_segment_callback_container->context = self;
    rwp->params.new_segment_callback = new_segment_callback;
    rwp->params.new_segment_callback_user_data = rwp->new_segment_callback_container;
  }

  if (!NIL_P(rwp->progress_callback_container->callback) || 0 != RARRAY_LEN(rwp->progress_callback_container->callbacks)) {
    rwp->progress_callback_container->context = self;
    rwp->params.progress_callback = progress_callback;
    rwp->params.progress_callback_user_data = rwp->progress_callback_container;
  }

  if (!NIL_P(rwp->abort_callback_container->callback) || 0 != RARRAY_LEN(rwp->abort_callback_container->callbacks)) {
    rwp->abort_callback_container->context = self;
    rwp->params.abort_callback = abort_callback;
    rwp->params.abort_callback_user_data = rwp->abort_callback_container;
  }
}

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

  register_callbacks(rwp, &self);

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
 * call-seq:
 *   model_n_vocab -> Integer
 */
VALUE ruby_whisper_model_n_vocab(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_vocab(rw->context));
}

/*
 * call-seq:
 *   model_n_audio_ctx -> Integer
 */
VALUE ruby_whisper_model_n_audio_ctx(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_audio_ctx(rw->context));
}

/*
 * call-seq:
 *   model_n_audio_state -> Integer
 */
VALUE ruby_whisper_model_n_audio_state(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_audio_state(rw->context));
}

/*
 * call-seq:
 *   model_n_audio_head -> Integer
 */
VALUE ruby_whisper_model_n_audio_head(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_audio_head(rw->context));
}

/*
 * call-seq:
 *   model_n_audio_layer -> Integer
 */
VALUE ruby_whisper_model_n_audio_layer(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_audio_layer(rw->context));
}

/*
 * call-seq:
 *   model_n_text_ctx -> Integer
 */
VALUE ruby_whisper_model_n_text_ctx(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_text_ctx(rw->context));
}

/*
 * call-seq:
 *   model_n_text_state -> Integer
 */
VALUE ruby_whisper_model_n_text_state(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_text_state(rw->context));
}

/*
 * call-seq:
 *   model_n_text_head -> Integer
 */
VALUE ruby_whisper_model_n_text_head(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_text_head(rw->context));
}

/*
 * call-seq:
 *   model_n_text_layer -> Integer
 */
VALUE ruby_whisper_model_n_text_layer(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_text_layer(rw->context));
}

/*
 * call-seq:
 *   model_n_mels -> Integer
 */
VALUE ruby_whisper_model_n_mels(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_mels(rw->context));
}

/*
 * call-seq:
 *   model_ftype -> Integer
 */
VALUE ruby_whisper_model_ftype(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return INT2NUM(whisper_model_ftype(rw->context));
}

/*
 * call-seq:
 *   model_type -> String
 */
VALUE ruby_whisper_model_type(VALUE self) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  return rb_str_new2(whisper_model_type_readable(rw->context));
}

/*
 * Run the entire model: PCM -> log mel spectrogram -> encoder -> decoder -> text
 * Not thread safe for same context
 * Uses the specified decoding strategy to obtain the text.
 *
 * call-seq:
 *   full(params, samples, n_samples) -> nil
 *   full(params, samples) -> nil
 *
 * The second argument +samples+ must be an array of samples, respond to :length, or be a MemoryView of an array of float. It must be 32 bit float PCM audio data.
 */
VALUE ruby_whisper_full(int argc, VALUE *argv, VALUE self) {
  if (argc < 2 || argc > 3) {
    rb_raise(rb_eArgError, "wrong number of arguments (given %d, expected 2..3)", argc);
  }

  ruby_whisper *rw;
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper, rw);
  VALUE params = argv[0];
  Data_Get_Struct(params, ruby_whisper_params, rwp);
  VALUE samples = argv[1];
  int n_samples;
  rb_memory_view_t view;
  const bool memory_view_available_p = rb_memory_view_available_p(samples);
  if (argc == 3) {
    n_samples = NUM2INT(argv[2]);
    if (TYPE(samples) == T_ARRAY) {
      if (RARRAY_LEN(samples) < n_samples) {
        rb_raise(rb_eArgError, "samples length %ld is less than n_samples %d", RARRAY_LEN(samples), n_samples);
      }
    }
    // Should check when samples.respond_to?(:length)?
  } else {
    if (TYPE(samples) == T_ARRAY) {
      n_samples = RARRAY_LEN(samples);
    } else if (memory_view_available_p) {
      if (!rb_memory_view_get(samples, &view, RUBY_MEMORY_VIEW_SIMPLE)) {
        view.obj = Qnil;
        rb_raise(rb_eArgError, "unable to get a memory view");
      }
      n_samples = view.byte_size / view.item_size;
    } else if (rb_respond_to(samples, id_length)) {
      n_samples = NUM2INT(rb_funcall(samples, id_length, 0));
    } else {
      rb_raise(rb_eArgError, "samples must respond to :length or be a MemoryView of an array of flaot when n_samples is not given");
    }
  }
  float * c_samples = (float *)malloc(n_samples * sizeof(float));
  if (memory_view_available_p)  {
    c_samples = (float *)view.data;
  } else {
    if (TYPE(samples) == T_ARRAY) {
      for (int i = 0; i < n_samples; i++) {
        c_samples[i] = RFLOAT_VALUE(rb_ary_entry(samples, i));
      }
    } else {
      // TODO: use rb_block_call
      VALUE iter = rb_funcall(samples, id_to_enum, 1, rb_str_new2("each"));
      for (int i = 0; i < n_samples; i++) {
        // TODO: check if iter is exhausted and raise ArgumentError appropriately
        VALUE sample = rb_funcall(iter, id_next, 0);
        c_samples[i] = RFLOAT_VALUE(sample);
      }
    }
  }
  register_callbacks(rwp, &self);
  const int result = whisper_full(rw->context, rwp->params, c_samples, n_samples);
  if (0 == result) {
    return Qnil;
  } else {
    rb_exc_raise(rb_funcall(eError, id_new, 1, result));
  }
}

/*
 * Split the input audio in chunks and process each chunk separately using whisper_full_with_state()
 * Result is stored in the default state of the context
 * Not thread safe if executed in parallel on the same context.
 * It seems this approach can offer some speedup in some cases.
 * However, the transcription accuracy can be worse at the beginning and end of each chunk.
 *
 * call-seq:
 *   full_parallel(params, samples) -> nil
 *   full_parallel(params, samples, n_samples) -> nil
 *   full_parallel(params, samples, n_samples, n_processors) -> nil
 *   full_parallel(params, samples, nil, n_processors) -> nil
 */
static VALUE ruby_whisper_full_parallel(int argc, VALUE *argv,VALUE self) {
  if (argc < 2 || argc > 4) {
    rb_raise(rb_eArgError, "wrong number of arguments (given %d, expected 2..3)", argc);
  }

  ruby_whisper *rw;
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper, rw);
  VALUE params = argv[0];
  Data_Get_Struct(params, ruby_whisper_params, rwp);
  VALUE samples = argv[1];
  int n_samples;
  int n_processors;
  rb_memory_view_t view;
  const bool memory_view_available_p = rb_memory_view_available_p(samples);
  switch (argc) {
  case 2:
    n_processors = 1;
    break;
  case 3:
    n_processors = 1;
    break;
  case 4:
    n_processors = NUM2INT(argv[3]);
    break;
  }
  if (argc >= 3 && !NIL_P(argv[2])) {
    n_samples = NUM2INT(argv[2]);
    if (TYPE(samples) == T_ARRAY) {
      if (RARRAY_LEN(samples) < n_samples) {
        rb_raise(rb_eArgError, "samples length %ld is less than n_samples %d", RARRAY_LEN(samples), n_samples);
      }
    }
    // Should check when samples.respond_to?(:length)?
  } else if (memory_view_available_p) {
    if (!rb_memory_view_get(samples, &view, RUBY_MEMORY_VIEW_SIMPLE)) {
      view.obj = Qnil;
      rb_raise(rb_eArgError, "unable to get a memory view");
    }
    n_samples = view.byte_size / view.item_size;
  } else {
    if (TYPE(samples) == T_ARRAY) {
      n_samples = RARRAY_LEN(samples);
    } else if (rb_respond_to(samples, id_length)) {
      n_samples = NUM2INT(rb_funcall(samples, id_length, 0));
    } else {
      rb_raise(rb_eArgError, "samples must respond to :length or be a MemoryView of an array of flaot when n_samples is not given");
    }
  }
  float * c_samples = (float *)malloc(n_samples * sizeof(float));
  if (memory_view_available_p) {
    c_samples = (float *)view.data;
  } else {
    if (TYPE(samples) == T_ARRAY) {
      for (int i = 0; i < n_samples; i++) {
        c_samples[i] = RFLOAT_VALUE(rb_ary_entry(samples, i));
      }
    } else {
      // FIXME: use rb_block_call
      VALUE iter = rb_funcall(samples, id_to_enum, 1, rb_str_new2("each"));
      for (int i = 0; i < n_samples; i++) {
        // TODO: check if iter is exhausted and raise ArgumentError
        VALUE sample = rb_funcall(iter, id_next, 0);
        c_samples[i] = RFLOAT_VALUE(sample);
      }
    }
  }
  register_callbacks(rwp, &self);
  const int result = whisper_full_parallel(rw->context, rwp->params, c_samples, n_samples, n_processors);
  if (0 == result) {
    return Qnil;
  } else {
    rb_exc_raise(rb_funcall(eError, id_new, 1, result));
  }
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
 * call-seq:
 *   full_get_segment_no_speech_prob(segment_index) -> Float
 */
static VALUE ruby_whisper_full_get_segment_no_speech_prob(VALUE self, VALUE i_segment) {
  ruby_whisper *rw;
  Data_Get_Struct(self, ruby_whisper, rw);
  const int c_i_segment = ruby_whisper_full_check_segment_index(rw, i_segment);
  const float no_speech_prob = whisper_full_get_segment_no_speech_prob(rw->context, c_i_segment);
  return DBL2NUM(no_speech_prob);
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
 *   suppress_nst = force_suppress -> force_suppress
 */
static VALUE ruby_whisper_params_set_suppress_nst(VALUE self, VALUE value) {
  BOOL_PARAMS_SETTER(self, suppress_nst, value)
}
/*
 * If true, suppresses non-speech-tokens.
 *
 * call-seq:
 *   suppress_nst -> bool
 */
static VALUE ruby_whisper_params_get_suppress_nst(VALUE self) {
  BOOL_PARAMS_GETTER(self, suppress_nst)
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
 * Tokens to provide to the whisper decoder as initial prompt
 * these are prepended to any existing text context from a previous call
 * use whisper_tokenize() to convert text to tokens.
 * Maximum of whisper_n_text_ctx()/2 tokens are used (typically 224).
 *
 * call-seq:
 *   initial_prompt -> String
 */
static VALUE ruby_whisper_params_get_initial_prompt(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return rwp->params.initial_prompt == nullptr ? Qnil : rb_str_new2(rwp->params.initial_prompt);
}
/*
 * call-seq:
 *   initial_prompt = prompt -> prompt
 */
static VALUE ruby_whisper_params_set_initial_prompt(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.initial_prompt = StringValueCStr(value);
  return value;
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
 * call-seq:
 *   temperature -> Float
 */
static VALUE ruby_whisper_params_get_temperature(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.temperature);
}
/*
 * call-seq:
 *   temperature = temp -> temp
 */
static VALUE ruby_whisper_params_set_temperature(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.temperature = RFLOAT_VALUE(value);
  return value;
}
/*
 * See https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/decoding.py#L97
 *
 * call-seq:
 *   max_initial_ts -> Flaot
 */
static VALUE ruby_whisper_params_get_max_initial_ts(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.max_initial_ts);
}
/*
 * call-seq:
 *   max_initial_ts = timestamp -> timestamp
 */
static VALUE ruby_whisper_params_set_max_initial_ts(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.max_initial_ts = RFLOAT_VALUE(value);
  return value;
}
/*
 * call-seq:
 *   length_penalty -> Float
 */
static VALUE ruby_whisper_params_get_length_penalty(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.length_penalty);
}
/*
 * call-seq:
 *   length_penalty = penalty -> penalty
 */
static VALUE ruby_whisper_params_set_length_penalty(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.length_penalty = RFLOAT_VALUE(value);
  return value;
}
/*
 * call-seq:
 *   temperature_inc -> Float
 */
static VALUE ruby_whisper_params_get_temperature_inc(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.temperature_inc);
}
/*
 * call-seq:
 *   temperature_inc = inc -> inc
 */
static VALUE ruby_whisper_params_set_temperature_inc(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.temperature_inc = RFLOAT_VALUE(value);
  return value;
}
/*
 * Similar to OpenAI's "compression_ratio_threshold"
 *
 * call-seq:
 *   entropy_thold -> Float
 */
static VALUE ruby_whisper_params_get_entropy_thold(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.entropy_thold);
}
/*
 * call-seq:
 *   entropy_thold = threshold -> threshold
 */
static VALUE ruby_whisper_params_set_entropy_thold(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.entropy_thold = RFLOAT_VALUE(value);
  return value;
}
/*
 * call-seq:
 *   logprob_thold -> Float
 */
static VALUE ruby_whisper_params_get_logprob_thold(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.logprob_thold);
}
/*
 * call-seq:
 *   logprob_thold = threshold -> threshold
 */
static VALUE ruby_whisper_params_set_logprob_thold(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.logprob_thold = RFLOAT_VALUE(value);
  return value;
}
/*
 * call-seq:
 *   no_speech_thold -> Float
 */
static VALUE ruby_whisper_params_get_no_speech_thold(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.no_speech_thold);
}
/*
 * call-seq:
 *   no_speech_thold = threshold -> threshold
 */
static VALUE ruby_whisper_params_set_no_speech_thold(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.no_speech_thold = RFLOAT_VALUE(value);
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
/*
 * Sets progress callback, called on each progress update.
 *
 *   params.new_segment_callback = ->(context, _, n_new, user_data) {
 *     # ...
 *   }
 *
 * call-seq:
 *   progress_callback = callback -> callback
 */
static VALUE ruby_whisper_params_set_progress_callback(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->progress_callback_container->callback = value;
  return value;
}
/*
 * Sets user data passed to the last argument of progress callback.
 *
 * call-seq:
 *   progress_callback_user_data = user_data -> use_data
 */
static VALUE ruby_whisper_params_set_progress_callback_user_data(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->progress_callback_container->user_data = value;
  return value;
}
/*
 * Sets abort callback, called to check if the process should be aborted.
 *
 *   params.abort_callback = ->(user_data) {
 *     # ...
 *   }
 *
 * call-seq:
 *   abort_callback = callback -> callback
 */
static VALUE ruby_whisper_params_set_abort_callback(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->abort_callback_container->callback = value;
  return value;
}
/*
 * Sets user data passed to the last argument of abort callback.
 *
 * call-seq:
 *   abort_callback_user_data = user_data -> use_data
 */
static VALUE ruby_whisper_params_set_abort_callback_user_data(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->abort_callback_container->user_data = value;
  return value;
}

// High level API

typedef struct {
  VALUE context;
  int index;
} ruby_whisper_segment;

typedef struct {
  VALUE context;
} ruby_whisper_model;

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
 * Hook called on progress update. Yields each progress Integer between 0 and 100.
 *
 *   whisper.on_progress do |progress|
 *     # ...
 *   end
 *
 * call-seq:
 *   on_progress {|progress| ... }
 */
static VALUE ruby_whisper_params_on_progress(VALUE self) {
  ruby_whisper_params *rws;
  Data_Get_Struct(self, ruby_whisper_params, rws);
  const VALUE blk = rb_block_proc();
  rb_ary_push(rws->progress_callback_container->callbacks, blk);
  return Qnil;
}

/*
 * Call block to determine whether abort or not. Return +true+ when you want to abort.
 *
 *   params.abort_on do
 *     if some_condition
 *       true # abort
 *     else
 *       false # continue
 *     end
 *   end
 *
 * call-seq:
 *   abort_on { ... }
 */
static VALUE ruby_whisper_params_abort_on(VALUE self) {
  ruby_whisper_params *rws;
  Data_Get_Struct(self, ruby_whisper_params, rws);
  const VALUE blk = rb_block_proc();
  rb_ary_push(rws->abort_callback_container->callbacks, blk);
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

/*
 * call-seq:
 *   no_speech_prob -> Float
 */
static VALUE ruby_whisper_segment_get_no_speech_prob(VALUE self) {
  ruby_whisper_segment *rws;
  Data_Get_Struct(self, ruby_whisper_segment, rws);
  ruby_whisper *rw;
  Data_Get_Struct(rws->context, ruby_whisper, rw);
  return DBL2NUM(whisper_full_get_segment_no_speech_prob(rw->context, rws->index));
}

static void rb_whisper_model_mark(ruby_whisper_model *rwm) {
  rb_gc_mark(rwm->context);
}

static VALUE ruby_whisper_model_allocate(VALUE klass) {
  ruby_whisper_model *rwm;
  rwm = ALLOC(ruby_whisper_model);
  return Data_Wrap_Struct(klass, rb_whisper_model_mark, RUBY_DEFAULT_FREE, rwm);
}

static VALUE rb_whisper_model_initialize(VALUE context) {
  ruby_whisper_model *rwm;
  const VALUE model = ruby_whisper_model_allocate(cModel);
  Data_Get_Struct(model, ruby_whisper_model, rwm);
  rwm->context = context;
  return model;
};

/*
 * call-seq:
 *   model -> Whisper::Model
 */
static VALUE ruby_whisper_get_model(VALUE self) {
  return rb_whisper_model_initialize(self);
}

/*
 * call-seq:
 *   n_vocab -> Integer
 */
static VALUE ruby_whisper_c_model_n_vocab(VALUE self) {
  ruby_whisper_model *rwm;
  Data_Get_Struct(self, ruby_whisper_model, rwm);
  ruby_whisper *rw;
  Data_Get_Struct(rwm->context, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_vocab(rw->context));
}

/*
 * call-seq:
 *   n_audio_ctx -> Integer
 */
static VALUE ruby_whisper_c_model_n_audio_ctx(VALUE self) {
  ruby_whisper_model *rwm;
  Data_Get_Struct(self, ruby_whisper_model, rwm);
  ruby_whisper *rw;
  Data_Get_Struct(rwm->context, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_audio_ctx(rw->context));
}

/*
 * call-seq:
 *   n_audio_state -> Integer
 */
static VALUE ruby_whisper_c_model_n_audio_state(VALUE self) {
  ruby_whisper_model *rwm;
  Data_Get_Struct(self, ruby_whisper_model, rwm);
  ruby_whisper *rw;
  Data_Get_Struct(rwm->context, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_audio_state(rw->context));
}

/*
 * call-seq:
 *   n_audio_head -> Integer
 */
static VALUE ruby_whisper_c_model_n_audio_head(VALUE self) {
  ruby_whisper_model *rwm;
  Data_Get_Struct(self, ruby_whisper_model, rwm);
  ruby_whisper *rw;
  Data_Get_Struct(rwm->context, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_audio_head(rw->context));
}

/*
 * call-seq:
 *   n_audio_layer -> Integer
 */
static VALUE ruby_whisper_c_model_n_audio_layer(VALUE self) {
  ruby_whisper_model *rwm;
  Data_Get_Struct(self, ruby_whisper_model, rwm);
  ruby_whisper *rw;
  Data_Get_Struct(rwm->context, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_audio_layer(rw->context));
}

/*
 * call-seq:
 *   n_text_ctx -> Integer
 */
static VALUE ruby_whisper_c_model_n_text_ctx(VALUE self) {
  ruby_whisper_model *rwm;
  Data_Get_Struct(self, ruby_whisper_model, rwm);
  ruby_whisper *rw;
  Data_Get_Struct(rwm->context, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_text_ctx(rw->context));
}

/*
 * call-seq:
 *   n_text_state -> Integer
 */
static VALUE ruby_whisper_c_model_n_text_state(VALUE self) {
  ruby_whisper_model *rwm;
  Data_Get_Struct(self, ruby_whisper_model, rwm);
  ruby_whisper *rw;
  Data_Get_Struct(rwm->context, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_text_state(rw->context));
}

/*
 * call-seq:
 *   n_text_head -> Integer
 */
static VALUE ruby_whisper_c_model_n_text_head(VALUE self) {
  ruby_whisper_model *rwm;
  Data_Get_Struct(self, ruby_whisper_model, rwm);
  ruby_whisper *rw;
  Data_Get_Struct(rwm->context, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_text_head(rw->context));
}

/*
 * call-seq:
 *   n_text_layer -> Integer
 */
static VALUE ruby_whisper_c_model_n_text_layer(VALUE self) {
  ruby_whisper_model *rwm;
  Data_Get_Struct(self, ruby_whisper_model, rwm);
  ruby_whisper *rw;
  Data_Get_Struct(rwm->context, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_text_layer(rw->context));
}

/*
 * call-seq:
 *   n_mels -> Integer
 */
static VALUE ruby_whisper_c_model_n_mels(VALUE self) {
  ruby_whisper_model *rwm;
  Data_Get_Struct(self, ruby_whisper_model, rwm);
  ruby_whisper *rw;
  Data_Get_Struct(rwm->context, ruby_whisper, rw);
  return INT2NUM(whisper_model_n_mels(rw->context));
}

/*
 * call-seq:
 *   ftype -> Integer
 */
static VALUE ruby_whisper_c_model_ftype(VALUE self) {
  ruby_whisper_model *rwm;
  Data_Get_Struct(self, ruby_whisper_model, rwm);
  ruby_whisper *rw;
  Data_Get_Struct(rwm->context, ruby_whisper, rw);
  return INT2NUM(whisper_model_ftype(rw->context));
}

/*
 * call-seq:
 *   type -> String
 */
static VALUE ruby_whisper_c_model_type(VALUE self) {
  ruby_whisper_model *rwm;
  Data_Get_Struct(self, ruby_whisper_model, rwm);
  ruby_whisper *rw;
  Data_Get_Struct(rwm->context, ruby_whisper, rw);
  return rb_str_new2(whisper_model_type_readable(rw->context));
}

static VALUE ruby_whisper_error_initialize(VALUE self, VALUE code) {
  const int c_code = NUM2INT(code);
  const char *raw_message;
  switch (c_code) {
  case -2:
    raw_message = "failed to compute log mel spectrogram";
    break;
  case -3:
    raw_message = "failed to auto-detect language";
    break;
  case -4:
    raw_message = "too many decoders requested";
    break;
  case -5:
    raw_message = "audio_ctx is larger than the maximum allowed";
    break;
  case -6:
    raw_message = "failed to encode";
    break;
  case -7:
    raw_message = "whisper_kv_cache_init() failed for self-attention cache";
    break;
  case -8:
    raw_message = "failed to decode";
    break;
  case -9:
    raw_message = "failed to decode";
    break;
  default:
    raw_message = "unknown error";
    break;
  }
  const VALUE message = rb_str_new2(raw_message);
  rb_call_super(1, &message);
  rb_iv_set(self, "@code", code);

  return self;
}


void Init_whisper() {
  id_to_s = rb_intern("to_s");
  id_call = rb_intern("call");
  id___method__ = rb_intern("__method__");
  id_to_enum = rb_intern("to_enum");
  id_length = rb_intern("length");
  id_next = rb_intern("next");
  id_new = rb_intern("new");
  id_to_path = rb_intern("to_path");
  id_URI = rb_intern("URI");
  id_pre_converted_models = rb_intern("pre_converted_models");

  mWhisper = rb_define_module("Whisper");
  cContext = rb_define_class_under(mWhisper, "Context", rb_cObject);
  cParams  = rb_define_class_under(mWhisper, "Params", rb_cObject);
  eError   = rb_define_class_under(mWhisper, "Error", rb_eStandardError);

  rb_define_const(mWhisper, "LOG_LEVEL_NONE", INT2NUM(GGML_LOG_LEVEL_NONE));
  rb_define_const(mWhisper, "LOG_LEVEL_INFO", INT2NUM(GGML_LOG_LEVEL_INFO));
  rb_define_const(mWhisper, "LOG_LEVEL_WARN", INT2NUM(GGML_LOG_LEVEL_WARN));
  rb_define_const(mWhisper, "LOG_LEVEL_ERROR", INT2NUM(GGML_LOG_LEVEL_ERROR));
  rb_define_const(mWhisper, "LOG_LEVEL_DEBUG", INT2NUM(GGML_LOG_LEVEL_DEBUG));
  rb_define_const(mWhisper, "LOG_LEVEL_CONT", INT2NUM(GGML_LOG_LEVEL_CONT));

  rb_define_singleton_method(mWhisper, "lang_max_id", ruby_whisper_s_lang_max_id, 0);
  rb_define_singleton_method(mWhisper, "lang_id", ruby_whisper_s_lang_id, 1);
  rb_define_singleton_method(mWhisper, "lang_str", ruby_whisper_s_lang_str, 1);
  rb_define_singleton_method(mWhisper, "lang_str_full", ruby_whisper_s_lang_str_full, 1);
  rb_define_singleton_method(mWhisper, "log_set", ruby_whisper_s_log_set, 2);
  rb_define_singleton_method(mWhisper, "finalize_log_callback", ruby_whisper_s_finalize_log_callback, 1);

  rb_define_alloc_func(cContext, ruby_whisper_allocate);
  rb_define_method(cContext, "initialize", ruby_whisper_initialize, -1);

  rb_define_method(cContext, "transcribe", ruby_whisper_transcribe, -1);
  rb_define_method(cContext, "model_n_vocab", ruby_whisper_model_n_vocab, 0);
  rb_define_method(cContext, "model_n_audio_ctx", ruby_whisper_model_n_audio_ctx, 0);
  rb_define_method(cContext, "model_n_audio_state", ruby_whisper_model_n_audio_state, 0);
  rb_define_method(cContext, "model_n_audio_head", ruby_whisper_model_n_audio_head, 0);
  rb_define_method(cContext, "model_n_audio_layer", ruby_whisper_model_n_audio_layer, 0);
  rb_define_method(cContext, "model_n_text_ctx", ruby_whisper_model_n_text_ctx, 0);
  rb_define_method(cContext, "model_n_text_state", ruby_whisper_model_n_text_state, 0);
  rb_define_method(cContext, "model_n_text_head", ruby_whisper_model_n_text_head, 0);
  rb_define_method(cContext, "model_n_text_layer", ruby_whisper_model_n_text_layer, 0);
  rb_define_method(cContext, "model_n_mels", ruby_whisper_model_n_mels, 0);
  rb_define_method(cContext, "model_ftype", ruby_whisper_model_ftype, 0);
  rb_define_method(cContext, "model_type", ruby_whisper_model_type, 0);
  rb_define_method(cContext, "full_n_segments", ruby_whisper_full_n_segments, 0);
  rb_define_method(cContext, "full_lang_id", ruby_whisper_full_lang_id, 0);
  rb_define_method(cContext, "full_get_segment_t0", ruby_whisper_full_get_segment_t0, 1);
  rb_define_method(cContext, "full_get_segment_t1", ruby_whisper_full_get_segment_t1, 1);
  rb_define_method(cContext, "full_get_segment_speaker_turn_next", ruby_whisper_full_get_segment_speaker_turn_next, 1);
  rb_define_method(cContext, "full_get_segment_text", ruby_whisper_full_get_segment_text, 1);
  rb_define_method(cContext, "full_get_segment_no_speech_prob", ruby_whisper_full_get_segment_no_speech_prob, 1);
  rb_define_method(cContext, "full", ruby_whisper_full, -1);
  rb_define_method(cContext, "full_parallel", ruby_whisper_full_parallel, -1);

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
  rb_define_method(cParams, "suppress_nst", ruby_whisper_params_get_suppress_nst, 0);
  rb_define_method(cParams, "suppress_nst=", ruby_whisper_params_set_suppress_nst, 1);
  rb_define_method(cParams, "token_timestamps", ruby_whisper_params_get_token_timestamps, 0);
  rb_define_method(cParams, "token_timestamps=", ruby_whisper_params_set_token_timestamps, 1);
  rb_define_method(cParams, "split_on_word", ruby_whisper_params_get_split_on_word, 0);
  rb_define_method(cParams, "split_on_word=", ruby_whisper_params_set_split_on_word, 1);
  rb_define_method(cParams, "initial_prompt", ruby_whisper_params_get_initial_prompt, 0);
  rb_define_method(cParams, "initial_prompt=", ruby_whisper_params_set_initial_prompt, 1);
  rb_define_method(cParams, "diarize", ruby_whisper_params_get_diarize, 0);
  rb_define_method(cParams, "diarize=", ruby_whisper_params_set_diarize, 1);

  rb_define_method(cParams, "offset", ruby_whisper_params_get_offset, 0);
  rb_define_method(cParams, "offset=", ruby_whisper_params_set_offset, 1);
  rb_define_method(cParams, "duration", ruby_whisper_params_get_duration, 0);
  rb_define_method(cParams, "duration=", ruby_whisper_params_set_duration, 1);

  rb_define_method(cParams, "max_text_tokens", ruby_whisper_params_get_max_text_tokens, 0);
  rb_define_method(cParams, "max_text_tokens=", ruby_whisper_params_set_max_text_tokens, 1);
  rb_define_method(cParams, "temperature", ruby_whisper_params_get_temperature, 0);
  rb_define_method(cParams, "temperature=", ruby_whisper_params_set_temperature, 1);
  rb_define_method(cParams, "max_initial_ts", ruby_whisper_params_get_max_initial_ts, 0);
  rb_define_method(cParams, "max_initial_ts=", ruby_whisper_params_set_max_initial_ts, 1);
  rb_define_method(cParams, "length_penalty", ruby_whisper_params_get_length_penalty, 0);
  rb_define_method(cParams, "length_penalty=", ruby_whisper_params_set_length_penalty, 1);
  rb_define_method(cParams, "temperature_inc", ruby_whisper_params_get_temperature_inc, 0);
  rb_define_method(cParams, "temperature_inc=", ruby_whisper_params_set_temperature_inc, 1);
  rb_define_method(cParams, "entropy_thold", ruby_whisper_params_get_entropy_thold, 0);
  rb_define_method(cParams, "entropy_thold=", ruby_whisper_params_set_entropy_thold, 1);
  rb_define_method(cParams, "logprob_thold", ruby_whisper_params_get_logprob_thold, 0);
  rb_define_method(cParams, "logprob_thold=", ruby_whisper_params_set_logprob_thold, 1);
  rb_define_method(cParams, "no_speech_thold", ruby_whisper_params_get_no_speech_thold, 0);
  rb_define_method(cParams, "no_speech_thold=", ruby_whisper_params_set_no_speech_thold, 1);

  rb_define_method(cParams, "new_segment_callback=", ruby_whisper_params_set_new_segment_callback, 1);
  rb_define_method(cParams, "new_segment_callback_user_data=", ruby_whisper_params_set_new_segment_callback_user_data, 1);
  rb_define_method(cParams, "progress_callback=", ruby_whisper_params_set_progress_callback, 1);
  rb_define_method(cParams, "progress_callback_user_data=", ruby_whisper_params_set_progress_callback_user_data, 1);
  rb_define_method(cParams, "abort_callback=", ruby_whisper_params_set_abort_callback, 1);
  rb_define_method(cParams, "abort_callback_user_data=", ruby_whisper_params_set_abort_callback_user_data, 1);

  rb_define_attr(eError, "code", true, false);
  rb_define_method(eError, "initialize", ruby_whisper_error_initialize, 1);

  // High leve
  cSegment  = rb_define_class_under(mWhisper, "Segment", rb_cObject);

  rb_define_alloc_func(cSegment, ruby_whisper_segment_allocate);
  rb_define_method(cContext, "each_segment", ruby_whisper_each_segment, 0);
  rb_define_method(cParams, "on_new_segment", ruby_whisper_params_on_new_segment, 0);
  rb_define_method(cParams, "on_progress", ruby_whisper_params_on_progress, 0);
  rb_define_method(cParams, "abort_on", ruby_whisper_params_abort_on, 0);
  rb_define_method(cSegment, "start_time", ruby_whisper_segment_get_start_time, 0);
  rb_define_method(cSegment, "end_time", ruby_whisper_segment_get_end_time, 0);
  rb_define_method(cSegment, "speaker_next_turn?", ruby_whisper_segment_get_speaker_turn_next, 0);
  rb_define_method(cSegment, "text", ruby_whisper_segment_get_text, 0);
  rb_define_method(cSegment, "no_speech_prob", ruby_whisper_segment_get_no_speech_prob, 0);

  cModel = rb_define_class_under(mWhisper, "Model", rb_cObject);
  rb_define_alloc_func(cModel, ruby_whisper_model_allocate);
  rb_define_method(cContext, "model", ruby_whisper_get_model, 0);
  rb_define_method(cModel, "n_vocab", ruby_whisper_c_model_n_vocab, 0);
  rb_define_method(cModel, "n_audio_ctx", ruby_whisper_c_model_n_audio_ctx, 0);
  rb_define_method(cModel, "n_audio_state", ruby_whisper_c_model_n_audio_state, 0);
  rb_define_method(cModel, "n_audio_head", ruby_whisper_c_model_n_audio_head, 0);
  rb_define_method(cModel, "n_audio_layer", ruby_whisper_c_model_n_audio_layer, 0);
  rb_define_method(cModel, "n_text_ctx", ruby_whisper_c_model_n_text_ctx, 0);
  rb_define_method(cModel, "n_text_state", ruby_whisper_c_model_n_text_state, 0);
  rb_define_method(cModel, "n_text_head", ruby_whisper_c_model_n_text_head, 0);
  rb_define_method(cModel, "n_text_layer", ruby_whisper_c_model_n_text_layer, 0);
  rb_define_method(cModel, "n_mels", ruby_whisper_c_model_n_mels, 0);
  rb_define_method(cModel, "ftype", ruby_whisper_c_model_ftype, 0);
  rb_define_method(cModel, "type", ruby_whisper_c_model_type, 0);

  rb_require("whisper/model/uri");
}
#ifdef __cplusplus
}
#endif
