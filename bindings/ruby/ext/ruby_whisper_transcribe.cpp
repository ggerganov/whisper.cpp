#include <ruby.h>
#include "ruby_whisper.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include <string>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif

extern ID id_to_s;
extern ID id_call;

extern void
register_callbacks(ruby_whisper_params * rwp, VALUE * self);

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
VALUE
ruby_whisper_transcribe(int argc, VALUE *argv, VALUE self) {
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
        pcmf32[i] = float((int32_t)pcm16[2*i] + pcm16[2*i + 1])/65536.0f;
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
#ifdef __cplusplus
}
#endif
