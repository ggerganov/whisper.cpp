#include <ruby.h>
#include "ruby_whisper.h"

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

extern VALUE cParams;

extern ID id_call;

extern VALUE rb_whisper_segment_initialize(VALUE context, int index);

static void
rb_whisper_callbcack_container_mark(ruby_whisper_callback_container *rwc)
{
  rb_gc_mark(rwc->user_data);
  rb_gc_mark(rwc->callback);
  rb_gc_mark(rwc->callbacks);
}

static ruby_whisper_callback_container*
rb_whisper_callback_container_allocate() {
  ruby_whisper_callback_container *container;
  container = ALLOC(ruby_whisper_callback_container);
  container->context = NULL;
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

void register_callbacks(ruby_whisper_params * rwp, VALUE * context) {
  if (!NIL_P(rwp->new_segment_callback_container->callback) || 0 != RARRAY_LEN(rwp->new_segment_callback_container->callbacks)) {
    rwp->new_segment_callback_container->context = context;
    rwp->params.new_segment_callback = new_segment_callback;
    rwp->params.new_segment_callback_user_data = rwp->new_segment_callback_container;
  }

  if (!NIL_P(rwp->progress_callback_container->callback) || 0 != RARRAY_LEN(rwp->progress_callback_container->callbacks)) {
    rwp->progress_callback_container->context = context;
    rwp->params.progress_callback = progress_callback;
    rwp->params.progress_callback_user_data = rwp->progress_callback_container;
  }

  if (!NIL_P(rwp->abort_callback_container->callback) || 0 != RARRAY_LEN(rwp->abort_callback_container->callbacks)) {
    rwp->abort_callback_container->context = context;
    rwp->params.abort_callback = abort_callback;
    rwp->params.abort_callback_user_data = rwp->abort_callback_container;
  }
}

void
rb_whisper_params_mark(ruby_whisper_params *rwp)
{
  rb_whisper_callbcack_container_mark(rwp->new_segment_callback_container);
  rb_whisper_callbcack_container_mark(rwp->progress_callback_container);
  rb_whisper_callbcack_container_mark(rwp->abort_callback_container);
}

void
ruby_whisper_params_free(ruby_whisper_params *rwp)
{
}

void
rb_whisper_params_free(ruby_whisper_params *rwp)
{
  // How to free user_data and callback only when not referred to by others?
  ruby_whisper_params_free(rwp);
  free(rwp);
}

static VALUE
ruby_whisper_params_allocate(VALUE klass)
{
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
 * params.language = "auto" | "en", etc...
 *
 * call-seq:
 *   language = lang_name -> lang_name
 */
static VALUE
ruby_whisper_params_set_language(VALUE self, VALUE value)
{
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
static VALUE
ruby_whisper_params_get_language(VALUE self)
{
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
static VALUE
ruby_whisper_params_set_translate(VALUE self, VALUE value)
{
  BOOL_PARAMS_SETTER(self, translate, value)
}
/*
 * call-seq:
 *   translate -> bool
 */
static VALUE
ruby_whisper_params_get_translate(VALUE self)
{
  BOOL_PARAMS_GETTER(self, translate)
}
/*
 * call-seq:
 *   no_context = dont_use_context -> dont_use_context
 */
static VALUE
ruby_whisper_params_set_no_context(VALUE self, VALUE value)
{
  BOOL_PARAMS_SETTER(self, no_context, value)
}
/*
 * If true, does not use past transcription (if any) as initial prompt for the decoder.
 *
 * call-seq:
 *   no_context -> bool
 */
static VALUE
ruby_whisper_params_get_no_context(VALUE self)
{
  BOOL_PARAMS_GETTER(self, no_context)
}
/*
 * call-seq:
 *   single_segment = force_single -> force_single
 */
static VALUE
ruby_whisper_params_set_single_segment(VALUE self, VALUE value)
{
  BOOL_PARAMS_SETTER(self, single_segment, value)
}
/*
 * If true, forces single segment output (useful for streaming).
 *
 * call-seq:
 *   single_segment -> bool
 */
static VALUE
ruby_whisper_params_get_single_segment(VALUE self)
{
  BOOL_PARAMS_GETTER(self, single_segment)
}
/*
 * call-seq:
 *   print_special = force_print -> force_print
 */
static VALUE
ruby_whisper_params_set_print_special(VALUE self, VALUE value)
{
  BOOL_PARAMS_SETTER(self, print_special, value)
}
/*
 * If true, prints special tokens (e.g. <SOT>, <EOT>, <BEG>, etc.).
 *
 * call-seq:
 *   print_special -> bool
 */
static VALUE
ruby_whisper_params_get_print_special(VALUE self)
{
  BOOL_PARAMS_GETTER(self, print_special)
}
/*
 * call-seq:
 *   print_progress = force_print -> force_print
 */
static VALUE
ruby_whisper_params_set_print_progress(VALUE self, VALUE value)
{
  BOOL_PARAMS_SETTER(self, print_progress, value)
}
/*
 * If true, prints progress information.
 *
 * call-seq:
 *   print_progress -> bool
 */
static VALUE
ruby_whisper_params_get_print_progress(VALUE self)
{
  BOOL_PARAMS_GETTER(self, print_progress)
}
/*
 * call-seq:
 *   print_realtime = force_print -> force_print
 */
static VALUE
ruby_whisper_params_set_print_realtime(VALUE self, VALUE value)
{
  BOOL_PARAMS_SETTER(self, print_realtime, value)
}
/*
 * If true, prints results from within whisper.cpp. (avoid it, use callback instead)
 * call-seq:
 *   print_realtime -> bool
 */
static VALUE
ruby_whisper_params_get_print_realtime(VALUE self)
{
  BOOL_PARAMS_GETTER(self, print_realtime)
}
/*
 * call-seq:
 *   print_timestamps = force_print -> force_print
 */
static VALUE
ruby_whisper_params_set_print_timestamps(VALUE self, VALUE value)
{
  BOOL_PARAMS_SETTER(self, print_timestamps, value)
}
/*
 * If true, prints timestamps for each text segment when printing realtime.
 *
 * call-seq:
 *   print_timestamps -> bool
 */
static VALUE
ruby_whisper_params_get_print_timestamps(VALUE self)
{
  BOOL_PARAMS_GETTER(self, print_timestamps)
}
/*
 * call-seq:
 *   suppress_blank = force_suppress -> force_suppress
 */
static VALUE
ruby_whisper_params_set_suppress_blank(VALUE self, VALUE value)
{
  BOOL_PARAMS_SETTER(self, suppress_blank, value)
}
/*
 * If true, suppresses blank outputs.
 *
 * call-seq:
 *   suppress_blank -> bool
 */
static VALUE
ruby_whisper_params_get_suppress_blank(VALUE self)
{
  BOOL_PARAMS_GETTER(self, suppress_blank)
}
/*
 * call-seq:
 *   suppress_nst = force_suppress -> force_suppress
 */
static VALUE
ruby_whisper_params_set_suppress_nst(VALUE self, VALUE value)
{
  BOOL_PARAMS_SETTER(self, suppress_nst, value)
}
/*
 * If true, suppresses non-speech-tokens.
 *
 * call-seq:
 *   suppress_nst -> bool
 */
static VALUE
ruby_whisper_params_get_suppress_nst(VALUE self)
{
  BOOL_PARAMS_GETTER(self, suppress_nst)
}
/*
 * If true, enables token-level timestamps.
 *
 * call-seq:
 *   token_timestamps -> bool
 */
static VALUE
ruby_whisper_params_get_token_timestamps(VALUE self)
{
  BOOL_PARAMS_GETTER(self, token_timestamps)
}
/*
 * call-seq:
 *   token_timestamps = force_timestamps -> force_timestamps
 */
static VALUE
ruby_whisper_params_set_token_timestamps(VALUE self, VALUE value)
{
  BOOL_PARAMS_SETTER(self, token_timestamps, value)
}
/*
 * If true, split on word rather than on token (when used with max_len).
 *
 * call-seq:
 *   translate -> bool
 */
static VALUE
ruby_whisper_params_get_split_on_word(VALUE self)
{
  BOOL_PARAMS_GETTER(self, split_on_word)
}
/*
 * call-seq:
 *   split_on_word = force_split -> force_split
 */
static VALUE
ruby_whisper_params_set_split_on_word(VALUE self, VALUE value)
{
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
static VALUE
ruby_whisper_params_get_initial_prompt(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return rwp->params.initial_prompt == NULL ? Qnil : rb_str_new2(rwp->params.initial_prompt);
}
/*
 * call-seq:
 *   initial_prompt = prompt -> prompt
 */
static VALUE
ruby_whisper_params_set_initial_prompt(VALUE self, VALUE value)
{
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
static VALUE
ruby_whisper_params_get_diarize(VALUE self)
{
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
static VALUE
ruby_whisper_params_set_diarize(VALUE self, VALUE value)
{
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
static VALUE
ruby_whisper_params_get_offset(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return INT2NUM(rwp->params.offset_ms);
}
/*
 * call-seq:
 *   offset = offset_ms -> offset_ms
 */
static VALUE
ruby_whisper_params_set_offset(VALUE self, VALUE value)
{
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
static VALUE
ruby_whisper_params_get_duration(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return INT2NUM(rwp->params.duration_ms);
}
/*
 * call-seq:
 *   duration = duration_ms -> duration_ms
 */
static VALUE
ruby_whisper_params_set_duration(VALUE self, VALUE value)
{
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
static VALUE
ruby_whisper_params_get_max_text_tokens(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return INT2NUM(rwp->params.n_max_text_ctx);
}
/*
 * call-seq:
 *   max_text_tokens = n_tokens -> n_tokens
 */
static VALUE
ruby_whisper_params_set_max_text_tokens(VALUE self, VALUE value)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.n_max_text_ctx = NUM2INT(value);
  return value;
}
/*
 * call-seq:
 *   temperature -> Float
 */
static VALUE
ruby_whisper_params_get_temperature(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.temperature);
}
/*
 * call-seq:
 *   temperature = temp -> temp
 */
static VALUE
ruby_whisper_params_set_temperature(VALUE self, VALUE value)
{
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
static VALUE
ruby_whisper_params_get_max_initial_ts(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.max_initial_ts);
}
/*
 * call-seq:
 *   max_initial_ts = timestamp -> timestamp
 */
static VALUE
ruby_whisper_params_set_max_initial_ts(VALUE self, VALUE value)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.max_initial_ts = RFLOAT_VALUE(value);
  return value;
}
/*
 * call-seq:
 *   length_penalty -> Float
 */
static VALUE
ruby_whisper_params_get_length_penalty(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.length_penalty);
}
/*
 * call-seq:
 *   length_penalty = penalty -> penalty
 */
static VALUE
ruby_whisper_params_set_length_penalty(VALUE self, VALUE value)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.length_penalty = RFLOAT_VALUE(value);
  return value;
}
/*
 * call-seq:
 *   temperature_inc -> Float
 */
static VALUE
ruby_whisper_params_get_temperature_inc(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.temperature_inc);
}
/*
 * call-seq:
 *   temperature_inc = inc -> inc
 */
static VALUE
ruby_whisper_params_set_temperature_inc(VALUE self, VALUE value)
{
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
static VALUE
ruby_whisper_params_get_entropy_thold(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.entropy_thold);
}
/*
 * call-seq:
 *   entropy_thold = threshold -> threshold
 */
static VALUE
ruby_whisper_params_set_entropy_thold(VALUE self, VALUE value)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.entropy_thold = RFLOAT_VALUE(value);
  return value;
}
/*
 * call-seq:
 *   logprob_thold -> Float
 */
static VALUE
ruby_whisper_params_get_logprob_thold(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.logprob_thold);
}
/*
 * call-seq:
 *   logprob_thold = threshold -> threshold
 */
static VALUE
ruby_whisper_params_set_logprob_thold(VALUE self, VALUE value)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.logprob_thold = RFLOAT_VALUE(value);
  return value;
}
/*
 * call-seq:
 *   no_speech_thold -> Float
 */
static VALUE
ruby_whisper_params_get_no_speech_thold(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return DBL2NUM(rwp->params.no_speech_thold);
}
/*
 * call-seq:
 *   no_speech_thold = threshold -> threshold
 */
static VALUE
ruby_whisper_params_set_no_speech_thold(VALUE self, VALUE value)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params.no_speech_thold = RFLOAT_VALUE(value);
  return value;
}
static VALUE
ruby_whisper_params_get_new_segment_callback(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return rwp->new_segment_callback_container->callback;
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
static VALUE
ruby_whisper_params_set_new_segment_callback(VALUE self, VALUE value)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->new_segment_callback_container->callback = value;
  return value;
}
static VALUE
ruby_whisper_params_get_new_segment_callback_user_data(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return rwp->new_segment_callback_container->user_data;
}
/*
 * Sets user data passed to the last argument of new segment callback.
 *
 * call-seq:
 *   new_segment_callback_user_data = user_data -> use_data
 */
static VALUE
ruby_whisper_params_set_new_segment_callback_user_data(VALUE self, VALUE value)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->new_segment_callback_container->user_data = value;
  return value;
}
static VALUE
ruby_whisper_params_get_progress_callback(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return rwp->progress_callback_container->callback;
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
static VALUE
ruby_whisper_params_set_progress_callback(VALUE self, VALUE value)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->progress_callback_container->callback = value;
  return value;
}
static VALUE
ruby_whisper_params_get_progress_callback_user_data(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return rwp->progress_callback_container->user_data;
}
/*
 * Sets user data passed to the last argument of progress callback.
 *
 * call-seq:
 *   progress_callback_user_data = user_data -> use_data
 */
static VALUE
ruby_whisper_params_set_progress_callback_user_data(VALUE self, VALUE value)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->progress_callback_container->user_data = value;
  return value;
}
static VALUE
ruby_whisper_params_get_abort_callback(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return rwp->abort_callback_container->callback;
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
static VALUE
ruby_whisper_params_set_abort_callback(VALUE self, VALUE value)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->abort_callback_container->callback = value;
  return value;
}
static VALUE
ruby_whisper_params_get_abort_callback_user_data(VALUE self)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return rwp->abort_callback_container->user_data;
}
/*
 * Sets user data passed to the last argument of abort callback.
 *
 * call-seq:
 *   abort_callback_user_data = user_data -> use_data
 */
static VALUE
ruby_whisper_params_set_abort_callback_user_data(VALUE self, VALUE value)
{
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->abort_callback_container->user_data = value;
  return value;
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
static VALUE
ruby_whisper_params_on_new_segment(VALUE self)
{
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
static VALUE
ruby_whisper_params_on_progress(VALUE self)
{
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
static VALUE
ruby_whisper_params_abort_on(VALUE self)
{
  ruby_whisper_params *rws;
  Data_Get_Struct(self, ruby_whisper_params, rws);
  const VALUE blk = rb_block_proc();
  rb_ary_push(rws->abort_callback_container->callbacks, blk);
  return Qnil;
}

void
init_ruby_whisper_params(VALUE *mWhisper)
{
  cParams  = rb_define_class_under(*mWhisper, "Params", rb_cObject);

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

  rb_define_method(cParams, "new_segment_callback", ruby_whisper_params_get_new_segment_callback, 0);
  rb_define_method(cParams, "new_segment_callback=", ruby_whisper_params_set_new_segment_callback, 1);
  rb_define_method(cParams, "new_segment_callback_user_data", ruby_whisper_params_get_new_segment_callback_user_data, 0);
  rb_define_method(cParams, "new_segment_callback_user_data=", ruby_whisper_params_set_new_segment_callback_user_data, 1);
  rb_define_method(cParams, "progress_callback", ruby_whisper_params_get_progress_callback, 0);
  rb_define_method(cParams, "progress_callback=", ruby_whisper_params_set_progress_callback, 1);
  rb_define_method(cParams, "progress_callback_user_data", ruby_whisper_params_get_progress_callback_user_data, 0);
  rb_define_method(cParams, "progress_callback_user_data=", ruby_whisper_params_set_progress_callback_user_data, 1);
  rb_define_method(cParams, "abort_callback", ruby_whisper_params_get_abort_callback, 0);
  rb_define_method(cParams, "abort_callback=", ruby_whisper_params_set_abort_callback, 1);
  rb_define_method(cParams, "abort_callback_user_data", ruby_whisper_params_get_abort_callback_user_data, 0);
  rb_define_method(cParams, "abort_callback_user_data=", ruby_whisper_params_set_abort_callback_user_data, 1);

  rb_define_method(cParams, "on_new_segment", ruby_whisper_params_on_new_segment, 0);
  rb_define_method(cParams, "on_progress", ruby_whisper_params_on_progress, 0);
  rb_define_method(cParams, "abort_on", ruby_whisper_params_abort_on, 0);
}
