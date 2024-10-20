#ifndef __RUBY_WHISPER_H
#define __RUBY_WHISPER_H

#include "whisper.h"

typedef struct {
  VALUE *context;
  VALUE user_data;
  VALUE callback;
} ruby_whisper_callback_user_data;

typedef struct {
  struct whisper_context *context;
} ruby_whisper;

typedef struct {
  struct whisper_full_params params;
  bool diarize;
  ruby_whisper_callback_user_data *new_segment_callback_user_data;
} ruby_whisper_params;

#endif
