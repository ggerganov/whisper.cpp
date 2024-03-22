#include "common-ffmpeg.h"

audio_capture::audio_capture(int len_ms)
    : m_len_ms(len_ms), m_running(false), m_initiated(false)
{
}

audio_capture::~audio_capture()
{
    pause();
    av_frame_free(&frame);
    avcodec_close(soundCodecContext);
    avcodec_free_context(&soundCodecContext);
    av_packet_free(&packet);
    avformat_close_input(&ctx);
    if (dst_data) {
        av_freep(&dst_data[0]);
    }
    av_freep(&dst_data);
    swr_close(swr_ctx);
    swr_free(&swr_ctx);
}

bool audio_capture::init(const char * url, int stream_id, int sample_rate)
{
    av_log_set_level(AV_LOG_INFO);

    int ret = 0;
    ctx = NULL;
    if ((ret = avformat_open_input(&ctx, url, NULL, NULL)) < 0) {
        fprintf(stderr, "Cannot open input: %s\n", url);
        return false;
    }
    if ((ret = avformat_find_stream_info(ctx, NULL)) < 0) {
        fprintf(stderr, "Cannot find stream information\n");
        return false;
    }
    int streamCount = ctx->nb_streams;
    if (stream_id >= streamCount) {
        fprintf(stderr, "Audio stream index out of range\n");
        return false;
    }
    if (stream_id < 0) {
        // use the first audio stream
        for (int i = 0; i < streamCount; ++i) {
            AVStream *stream = ctx->streams[i];
            AVCodecParameters *codecpar = stream->codecpar;
            AVMediaType codecType = codecpar->codec_type;
            av_dump_format(ctx, i, url, 0);
            if (codecType == AVMEDIA_TYPE_AUDIO) {
                stream_id = i;
                break;
            }
        }
    }
    if (stream_id < 0) {
        fprintf(stderr, "No audio stream found\n");
        return false;
    }
    AVStream *stream = ctx->streams[stream_id];
    AVCodecParameters *codecpar = stream->codecpar;
    AVCodecID codecId = codecpar->codec_id;
    AVMediaType codecType = codecpar->codec_type;
    if (codecType != AVMEDIA_TYPE_AUDIO) {
        fprintf(stderr, "Stream %d is not an audio stream\n", stream_id);
        return false;
    }
    m_stream_id = stream_id;
    int src_rate = codecpar->sample_rate;
    const AVCodec* soundCodec = avcodec_find_decoder(codecId);
    if (!soundCodec) {
        fprintf(stderr, "Cannot find codec\n");
        return false;
    }
    soundCodecContext = avcodec_alloc_context3(soundCodec);
    if (!soundCodecContext) {
        fprintf(stderr, "Cannot allocate codec context\n");
        return false;
    }
    if (avcodec_parameters_to_context(soundCodecContext, codecpar) < 0) {
        fprintf(stderr, "Cannot initialize codec context\n");
        return false;
    }
    if (avcodec_open2(soundCodecContext, soundCodec, NULL) < 0) {
        fprintf(stderr, "Cannot open codec\n");
        return false;
    }

    int64_t src_ch_layout = codecpar->channel_layout;

    // resample to mono, float format and the specified sample rate
    int dst_ch_layout = AV_CH_LAYOUT_MONO;
    m_sample_rate = sample_rate;
    m_audio.resize((m_sample_rate*m_len_ms)/1000);

    // create resampler context
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        return false;
    }

    // set resample options
    av_opt_set_int(swr_ctx, "in_channel_layout",    src_ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate",       src_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", (AVSampleFormat)codecpar->format, 0);

    av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate",       m_sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

    // initialize the resampling context
    if ((ret = swr_init(swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        return false;
    }

    max_dst_nb_samples = 1024;
    dst_linesize = 0;
    dst_data = NULL;

    ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, 1, max_dst_nb_samples, AV_SAMPLE_FMT_FLT, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate destination samples\n");
        return false;
    }
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Cannot allocate frame\n");
        return false;
    }
    packet = av_packet_alloc();
    if (!packet) {
        fprintf(stderr, "Cannot allocate packet\n");
        return false;
    }
    m_initiated = true;
    return true;
}

bool audio_capture::decode_packet()
{
    if (!m_initiated) {
        return false;
    }
    int ret = 0;
    if ((ret = av_read_frame(ctx, packet)) < 0) {
        return false;
    }
    if (packet->stream_index != m_stream_id) {
        av_packet_unref(packet);
        return true;
    }
    ret = avcodec_send_packet(soundCodecContext, packet);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
        fprintf(stderr, "Error while sending a packet to the decoder: %s\n", errbuf);
        return false;
    }
    while (ret >= 0) {
        ret = avcodec_receive_frame(soundCodecContext, frame);
        if (ret != 0) {
            break;
        }
        int src_nb_samples = frame->nb_samples;
        int dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, m_sample_rate) + src_nb_samples, m_sample_rate, m_sample_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            av_freep(&dst_data[0]);
            ret = av_samples_alloc(dst_data, &dst_linesize, 1, dst_nb_samples, AV_SAMPLE_FMT_FLT, 1);
            if (ret < 0) {
                break;
            }
            max_dst_nb_samples = dst_nb_samples;
        }

        // convert frame data to destination format
        ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)frame->data, src_nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            return false;
        }
        int dst_bufsize = av_samples_get_buffer_size(&dst_linesize, 1, ret, AV_SAMPLE_FMT_FLT, 1);
        if (dst_bufsize < 0) {
            fprintf(stderr, "Could not get sample buffer size\n");
            return false;
        }

        callback(dst_data[0], dst_bufsize);

        av_frame_unref(frame);
    }
    av_packet_unref(packet);
    return true;
}

void audio_capture::callback(uint8_t * stream, int len) {
    if (!m_running) {
        return;
    }

    size_t n_samples = len / sizeof(float);

    if (n_samples > m_audio.size()) {
        n_samples = m_audio.size();

        stream += (len - (n_samples * sizeof(float)));
    }

    //fprintf(stderr, "%s: %zu samples, pos %zu, len %zu\n", __func__, n_samples, m_audio_pos, m_audio_len);

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_audio_pos + n_samples > m_audio.size()) {
            const size_t n0 = m_audio.size() - m_audio_pos;

            memcpy(&m_audio[m_audio_pos], stream, n0 * sizeof(float));
            memcpy(&m_audio[0], stream + n0 * sizeof(float), (n_samples - n0) * sizeof(float));

            m_audio_pos = (m_audio_pos + n_samples) % m_audio.size();
            m_audio_len = m_audio.size();
        } else {
            memcpy(&m_audio[m_audio_pos], stream, n_samples * sizeof(float));

            m_audio_pos = (m_audio_pos + n_samples) % m_audio.size();
            m_audio_len = std::min(m_audio_len + n_samples, m_audio.size());
        }
    }
}

bool audio_capture::clear() {
    if (!m_initiated) {
        fprintf(stderr, "%s: not initiated!\n", __func__);
        return false;
    }
    if (!m_running) {
        fprintf(stderr, "%s: not running!\n", __func__);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_audio_pos = 0;
        m_audio_len = 0;
    }

    return true;
}

void audio_capture::get(int ms, std::vector<float> & result) {
    if (!m_initiated) {
        fprintf(stderr, "%s: not initiated!\n", __func__);
        return;
    }
    if (!m_running) {
        fprintf(stderr, "%s: not running!\n", __func__);
        return;
    }

    result.clear();

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (ms <= 0) {
            ms = m_len_ms;
        }

        size_t n_samples = (m_sample_rate * ms) / 1000;
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
}

static void audio_decoder(audio_capture * capture, std::atomic_bool & running)
{
    while (running) {
        if (!capture->decode_packet()) {
            break;
        }
    }
}

bool audio_capture::resume()
{
    if (!m_initiated) {
        fprintf(stderr, "%s: not initiated!\n", __func__);
        return false;
    }
    if (m_running) {
        return true;
    }
    decode_thread = std::thread(audio_decoder, this, std::ref(m_running));
    m_running = true;
    return true;
}

bool audio_capture::pause()
{
    if (!m_initiated) {
        fprintf(stderr, "%s: not initiated!\n", __func__);
        return false;
    }
    if (!m_running) {
        return true;
    }
    m_running = false;
    decode_thread.join();
    return true;
}
