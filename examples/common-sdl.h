#pragma once

#include <SDL.h>
#include <SDL_audio.h>

#include <atomic>
#include <cstdint>
#include <vector>
#include <mutex>

//
// SDL Audio capture
//

class audio_async {
public:
    audio_async(int len_ms);
    ~audio_async();

    bool init(int capture_id, int sample_rate);

    // start capturing audio via the provided SDL callback
    // keep last len_ms seconds of audio in a circular buffer
    bool resume();
    bool pause();
    bool clear();

    // callback to be called by SDL
    void callback(uint8_t * stream, int len);

    // get audio data from the circular buffer
    void get(int ms, std::vector<float> & audio);

    // check if the audio input has overwritten audio data we haven't
    // yet read from the circular buffer since the last time we called
    // overran()
    bool overran();

private:
    SDL_AudioDeviceID m_dev_id_in = 0;

    int m_len_ms = 0;
    int m_sample_rate = 0;

    std::atomic_bool m_running;
    std::mutex       m_mutex;

    std::vector<float> m_audio;
    size_t             m_audio_pos = 0;
    size_t             m_audio_len = 0;
    // m_audio_read_cursor is the offset into the samples array of the
    // oldest sample that has not yet been read by a call to get()
    size_t             m_audio_read_cursor = -1;
    bool               overran_flag = false;
};

// Return false if need to quit
bool sdl_poll_events();
