#include <ruby.h>
#include "ruby_whisper.h"

#define BOOL_PARAMS_SETTER(self, prop, value) \
  ruby_whisper_params *rwp; \
  Data_Get_Struct(self, ruby_whisper_params, rwp); \
  if (value == Qfalse || value == Qnil) { \
    rwp->params->prop = false; \
  } else { \
    rwp->params->prop = true; \
  } \
  return value; \

#define BOOL_PARAMS_GETTER(self,  prop) \
  ruby_whisper_params *rwp; \
  Data_Get_Struct(self, ruby_whisper_params, rwp); \
  if (rwp->params->prop) { \
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
  if (rwp->params) {
    free(rwp->params);
    rwp->params = NULL;
  }
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
  rwp->params = ALLOC(struct whisper_full_params);
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
  return self;
}

/*
 * params.auto_detection = true|false
 */
static VALUE ruby_whisper_params_set_auto_detection(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  if (value == Qfalse || value == Qnil) {
    rwp->params->language = NULL;
  } else {
    rwp->params->language = "auto";
  }
  return value;
}
static VALUE ruby_whisper_params_get_auto_detection(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  if (rwp->params->language) {
    return Qtrue;
  } else {
    return Qfalse;
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

static VALUE ruby_whisper_params_get_offset(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return INT2NUM(rwp->params->offset_ms);
}
static VALUE ruby_whisper_params_set_offset(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params->offset_ms = NUM2INT(value);
  return value;
}
static VALUE ruby_whisper_params_get_duration(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return INT2NUM(rwp->params->duration_ms);
}
static VALUE ruby_whisper_params_set_duration(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params->duration_ms = NUM2INT(value);
  return value;
}

static VALUE ruby_whisper_params_get_max_text_tokens(VALUE self) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  return INT2NUM(rwp->params->n_max_text_ctx);
}
static VALUE ruby_whisper_params_set_max_text_tokens(VALUE self, VALUE value) {
  ruby_whisper_params *rwp;
  Data_Get_Struct(self, ruby_whisper_params, rwp);
  rwp->params->n_max_text_ctx = NUM2INT(value);
  return value;
}

void Init_whisper() {
  mWhisper = rb_define_module("Whisper");
  cContext = rb_define_class_under(mWhisper, "Context", rb_cObject);
  cParams  = rb_define_class_under(mWhisper, "Params", rb_cObject);

  rb_define_alloc_func(cContext, ruby_whisper_allocate);
  rb_define_method(cContext, "initialize", ruby_whisper_initialize, -1);

  rb_define_alloc_func(cParams, ruby_whisper_params_allocate);

  rb_define_method(cParams, "auto_detection=", ruby_whisper_params_set_auto_detection, 1);
  rb_define_method(cParams, "auto_detection", ruby_whisper_params_get_auto_detection, 0);
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

  rb_define_method(cParams, "offset", ruby_whisper_params_get_offset, 0);
  rb_define_method(cParams, "offset=", ruby_whisper_params_set_offset, 1);
  rb_define_method(cParams, "duration", ruby_whisper_params_get_duration, 0);
  rb_define_method(cParams, "duration=", ruby_whisper_params_set_duration, 1);

  rb_define_method(cParams, "max_text_tokens", ruby_whisper_params_get_max_text_tokens, 0);
  rb_define_method(cParams, "max_text_tokens=", ruby_whisper_params_set_max_text_tokens, 1);
}
