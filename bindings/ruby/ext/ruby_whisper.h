#ifndef __RUBY_WHISPER_H
#define __RUBY_WHISPER_H

#include "whisper.h"

typedef struct {
  struct whisper_context *context;
  VALUE new_segment_callback;
} ruby_whisper;

typedef struct {
  struct whisper_full_params params;
  bool diarize;
  VALUE new_segment_callback;
} ruby_whisper_params;

#endif
