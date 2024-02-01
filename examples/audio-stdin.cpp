#include "audio-stdin.h"

#include <csignal>
#include <cstring>

// Because the original happened to handle OS signals in the same library as
// handled the audio, this is implemented here.
// TODO: split this out to something a bit more coherent

bool should_quit = false;

void quit_signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    should_quit = true;
  }
}

void install_signal_handler() {
    std::signal(SIGINT, quit_signal_handler);
    std::signal(SIGTERM, quit_signal_handler);
}

bool should_keep_running() {
  return !should_quit;
}



audio_stdin::audio_stdin(int len_ms) {
     m_len_ms = len_ms;

     m_running = false;
}

audio_stdin::~audio_stdin() {
  // Nothing to do here, we don't own m_fd
}

/*
Setup the stdin reader.  For simplicity, let's say that the file descriptor
passed in needs to already be open, and that the destructor doesn't close it.
*/
bool audio_stdin::init(int fd, int sample_rate) {
  m_fd = fd;
  m_sample_rate = sample_rate;

  size_t buffer_size = (m_sample_rate*m_len_ms)/1000;
  m_audio.resize(buffer_size);
  m_in_buffer.resize(buffer_size);

  return true;
}

bool audio_stdin::resume() {
  // In this initial implementation, we're assuming that we don't even have to
  // do anything in the background.  Getting data off stdin can be assumed to be
  // fast enough that we can do it synchronously, so `resume` can be a noop.
  m_running = true;
  return true;
}

bool audio_stdin::pause() {
  // Similarly to `resume`, we don't need to do anything here.  We just never
  // read from stdin.
  m_running = false;
  return true;
}

bool audio_stdin::clear() {

    if (!m_running) {
      fprintf(stderr, "%s: not running!\n", __func__);
      return false;
    }

    // Now while *we're* not doing anything with threads, that doen't mean
    // nobody else is
    {
      std::lock_guard<std::mutex> lock(m_mutex);

      m_audio_pos = 0;
      m_audio_len = 0;
    }

    return true;
}

// Important: len is a number of bytes
bool audio_stdin::callback(int len) {
  // We aren't called by SDL.  Instead we're called whenever whisper runs close enough to
  // being out of audio that the next iteration would stall.
    if (!m_running) {
        return true;
    }

    size_t n_samples = len / sizeof(float);

    if (n_samples > m_audio.size()) {
        n_samples = m_audio.size();
    }

    {
      //        std::lock_guard<std::mutex> lock(m_mutex);

	// stdin is PCM mono 16khz in s16le format.  Use ffmpeg to make that happen.
        int nread = read(m_fd, m_in_buffer.data(), m_in_buffer.size());
	if (nread < 0) { /* TODO then we need to barf, that's a fail */ }
	else if (nread == 0) { should_quit = true; return false; }

	//Nicked this from drwav.h, we're basically doing the same as drwav_s16_to_f32
	float scale_factor = 0.000030517578125f;

        if (m_audio_pos + n_samples > m_audio.size()) {
            const size_t n0 = m_audio.size() - m_audio_pos;

	    // Now we pull as much as we need from stdin, blocking if we have to

	    for(int i = m_audio_pos; i < n0; i+=4) {
	      m_audio[i] = m_in_buffer[i] * scale_factor;
 	    }
	    for(int i = 0; i < n_samples - n0; i++) {
	      m_audio[i] = m_in_buffer[i] * scale_factor;
	    }

            m_audio_pos = (m_audio_pos + n_samples) % m_audio.size();
            m_audio_len = m_audio.size();
        } else {
	    for(int i = 0; i < n_samples; i++) {
	      m_audio[i] = m_in_buffer[i] * scale_factor;
	    }

            m_audio_pos = (m_audio_pos + n_samples) % m_audio.size();
            m_audio_len = std::min(m_audio_len + n_samples, m_audio.size());
        }
    }
    return true;
}

// Identical to audio_async, except that we can signal that the audio stream is closed
bool audio_stdin::get(int ms, std::vector<float> & result) {

    if (!m_running) {
        fprintf(stderr, "%s: not running!\n", __func__);
        return true;
    }

    result.clear();

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (ms <= 0) {
            ms = m_len_ms;
        }

        size_t n_samples = (m_sample_rate * ms) / 1000;

	// we're double-buffering here, but I'll take that out if it works.
	// It's just ungainly, not actually a problem.

	if (!callback(n_samples * sizeof(float))){
	  return false;
	}

        if (n_samples > m_audio_len) {
            n_samples = m_audio_len;
        }
        result.resize(n_samples);

        int s0 = m_audio_pos - n_samples;
        if (s0 < 0) {
            s0 += m_audio.size();
        }

        if (s0 + n_samples > m_audio.size()) {
            const size_t n0 = m_audio.size() - s0;

            memcpy(result.data(), &m_audio[s0], n0 * sizeof(float));
            memcpy(&result[n0], &m_audio[0], (n_samples - n0) * sizeof(float));
        } else {
            memcpy(result.data(), &m_audio[s0], n_samples * sizeof(float));
        }
    }
    return true;
}
