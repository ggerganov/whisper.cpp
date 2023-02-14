#ifndef __RUBY_WHISPER_H
#define __RUBY_WHISPER_H

#include "whisper.h"

typedef struct {
  struct whisper_context *context;
} ruby_whisper;

typedef struct {
  struct whisper_full_params params;
  bool diarize;
} ruby_whisper_params;

#endif
