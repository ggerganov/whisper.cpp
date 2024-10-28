#ifndef __RUBY_WHISPER_H
#define __RUBY_WHISPER_H

#include "whisper.h"

typedef struct {
  VALUE *context;
  VALUE user_data;
  VALUE callback;
  VALUE callbacks;
} ruby_whisper_callback_container;

typedef struct {
  struct whisper_context *context;
} ruby_whisper;

typedef struct {
  struct whisper_full_params params;
  bool diarize;
  ruby_whisper_callback_container *new_segment_callback_container;
} ruby_whisper_params;

#endif
