#include <ruby.h>
#include <ruby/memory_view.h>
#include <ruby/encoding.h>

static VALUE
jfk_reader_initialize(VALUE self, VALUE audio_path)
{
  rb_iv_set(self, "audio_path", audio_path);
  return Qnil;
}

static bool
jfk_reader_get_memory_view(const VALUE obj, rb_memory_view_t *view, int flags)
{
  VALUE audio_path = rb_iv_get(obj, "audio_path");
  const char *audio_path_str = StringValueCStr(audio_path);
  const int n_samples = 176000;
  float *data = (float *)malloc(n_samples * sizeof(float));
  short *samples = (short *)malloc(n_samples * sizeof(short));
  FILE *file = fopen(audio_path_str, "rb");

  fseek(file, 78, SEEK_SET);
  fread(samples, sizeof(short), n_samples, file);
  fclose(file);
  for (int i = 0; i < n_samples; i++) {
    data[i] = samples[i]/32768.0;
  }

  view->obj = obj;
  view->data = (void *)data;
  view->byte_size = sizeof(float) * n_samples;
  view->readonly = true;
  view->format = "f";
  view->item_size = sizeof(float);
  view->item_desc.components = NULL;
  view->item_desc.length = 0;
  view->ndim = 1;
  view->shape = NULL;
  view->sub_offsets = NULL;
  view->private_data = NULL;

  return true;
}

static bool
jfk_reader_release_memory_view(const VALUE obj, rb_memory_view_t *view)
{
  return true;
}

static bool
jfk_reader_memory_view_available_p(const VALUE obj)
{
  return true;
}

static const rb_memory_view_entry_t jfk_reader_view_entry = {
  jfk_reader_get_memory_view,
  jfk_reader_release_memory_view,
  jfk_reader_memory_view_available_p
};

static VALUE
read_jfk(int argc, VALUE *argv, VALUE obj)
{
  const char *audio_path_str = StringValueCStr(argv[0]);
  const int n_samples = 176000;

  short samples[n_samples];
  FILE *file = fopen(audio_path_str, "rb");

  fseek(file, 78, SEEK_SET);
  fread(samples, sizeof(short), n_samples, file);
  fclose(file);

  VALUE rb_samples = rb_ary_new2(n_samples);
  for (int i = 0; i < n_samples; i++) {
    rb_ary_push(rb_samples, INT2FIX(samples[i]));
  }

  VALUE rb_data = rb_ary_new2(n_samples);
  for (int i = 0; i < n_samples; i++) {
    rb_ary_push(rb_data, DBL2NUM(samples[i]/32768.0));
  }

  float data[n_samples];
  for (int i = 0; i < n_samples; i++) {
    data[i] = samples[i]/32768.0;
  }
  void *c_data = (void *)data;
  VALUE rb_void = rb_enc_str_new((const char *)c_data, sizeof(data), rb_ascii8bit_encoding());

  VALUE rb_result = rb_ary_new3(3, rb_samples, rb_data, rb_void);
  return rb_result;
}

void Init_jfk_reader(void)
{
  VALUE cJFKReader = rb_define_class("JFKReader", rb_cObject);
  rb_memory_view_register(cJFKReader, &jfk_reader_view_entry);
  rb_define_method(cJFKReader, "initialize", jfk_reader_initialize, 1);


  rb_define_global_function("read_jfk", read_jfk, -1);



}
