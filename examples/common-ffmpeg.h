#pragma once
extern "C" {
    #include <libavutil/opt.h>
    #include <libavutil/channel_layout.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/samplefmt.h>
    #include <libswresample/swresample.h>
}
#include <atomic>
#include <cstdint>
#include <vector>
#include <mutex>
#include <thread>

//
// FFmpeg Audio capture
//

class audio_capture {
public:
    audio_capture(int len_ms);
    ~audio_capture();

    // open the url and use the audio stream with the specified id
    // if stream_id < 0, use the first audio stream
    // resample audio to the specified sample_rate
    bool init(const char * url, int stream_id, int sample_rate);

    // start decoding and resampling the audio stream in a separate thread
    // keep last len_ms seconds of audio in a circular buffer
    bool resume();
    bool pause();
    bool clear();

    // decode and resample a single packet
    bool decode_packet();

    // callback to be called by the audio decoder thread
    void callback(uint8_t * stream, int len);

    // get audio data from the circular buffer
    void get(int ms, std::vector<float> & audio);

private:

    int m_len_ms = 0;

    std::atomic_bool m_running;
    bool             m_initiated;
    std::mutex       m_mutex;

    std::vector<float> m_audio;
    size_t             m_audio_pos = 0;
    size_t             m_audio_len = 0;

    int m_stream_id        = -1;
    int m_sample_rate      = 0;
    int max_dst_nb_samples = 1024;
    int dst_linesize       = 0;
    uint8_t **dst_data     = NULL;

    AVFormatContext *ctx;
    AVCodecContext* soundCodecContext;
    AVPacket *packet;
    AVFrame* frame;
    struct SwrContext *swr_ctx;
    std::thread decode_thread;
};
