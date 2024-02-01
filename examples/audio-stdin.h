#pragma once

#include <atomic>
#include <cstdint>
#include <vector>
#include <mutex>

//
// Stdin wav capture
//

class audio_stdin {
public:
    audio_stdin(int len_ms);
    ~audio_stdin();

    bool init(int fd, int sample_rate);

  // The sdl version captures to a circular buffer; I think we should assume that we need the same
    // start capturing audio via the provided SDL callback
    // keep last len_ms seconds of audio in a circular buffer
    bool resume();
    bool pause();
    bool clear();

    // callback to be called when we run out of data.
    // This is what will do a read() from stdin, and can block if there isn't
    // enough data yet.
    // Returns false if the stream's closed.
    bool callback(int len);

    // get audio data from the circular buffer
    // Returns false if the stream's closed.
    bool get(int ms, std::vector<float> & audio);

private:
    int m_fd = 0;
    int m_len_ms = 0;
    int m_sample_rate = 0;

    std::atomic_bool m_running;
    std::mutex       m_mutex;

    std::vector<float> m_audio;
    // Since the data we plan on receiving needs converting, we need somewhere to hold it while we do that
    std::vector<int16_t> m_in_buffer;
    size_t             m_audio_pos = 0;
    size_t             m_audio_len = 0;
};

// Return false if need to quit - goes false at eof?
bool should_keep_running();
// Call this before needing to quit.
void install_signal_handler();
