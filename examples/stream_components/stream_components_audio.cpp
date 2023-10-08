#include "stream_components_audio.h"

using namespace stream_components;


// -- LocalSDLMicrophone -- 

LocalSDLMicrophone::LocalSDLMicrophone(audio_params & params):
    params(params),
    audio(params.length_ms),
    pcmf32_new(params.n_samples_30s, 0.0f),
    pcmf32(params.n_samples_30s, 0.0f)
{
    fprintf(stderr, "%s: processing %d samples (step = %.1f sec / len = %.1f sec / keep = %.1f sec) ...\n",
        __func__,
        params.n_samples_step,
        float(params.n_samples_step)/WHISPER_SAMPLE_RATE,
        float(params.n_samples_len )/WHISPER_SAMPLE_RATE,
        float(params.n_samples_keep)/WHISPER_SAMPLE_RATE);
    
    if (params.use_vad) {
        fprintf(stderr, "%s: using VAD, will transcribe on speech activity\n", __func__);
    }
                
    if (!audio.init(params.capture_id, WHISPER_SAMPLE_RATE)) {
        throw std::runtime_error("LocalSDLMicrophone(): audio.init() failed!");
    }

    audio.resume();

    t_last  = std::chrono::high_resolution_clock::now();
    t_start = t_last;
}

LocalSDLMicrophone::~LocalSDLMicrophone() 
{
    audio.pause();
}

std::vector<float>& LocalSDLMicrophone::get_next()
{
    if (!params.use_vad) {
        while (true) {
            audio.get(params.step_ms, pcmf32_new);

            if ((int) pcmf32_new.size() > 2*params.n_samples_step) {
                fprintf(stderr, "\n\n%s: WARNING: cannot process audio fast enough, dropping audio ...\n\n", __func__);
                audio.clear();
                continue;
            }

            if ((int) pcmf32_new.size() >= params.n_samples_step) {
                audio.clear();
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        const int n_samples_new = pcmf32_new.size();

        // take up to params.length_ms audio from previous iteration
        const int n_samples_take = std::min((int) pcmf32_old.size(), std::max(0, params.n_samples_keep + params.n_samples_len - n_samples_new));

        //printf("processing: take = %d, new = %d, old = %d\n", n_samples_take, n_samples_new, (int) pcmf32_old.size());

        pcmf32.resize(n_samples_new + n_samples_take);

        for (int i = 0; i < n_samples_take; i++) {
            pcmf32[i] = pcmf32_old[pcmf32_old.size() - n_samples_take + i];
        }

        memcpy(pcmf32.data() + n_samples_take, pcmf32_new.data(), n_samples_new*sizeof(float));

        pcmf32_old = pcmf32;
    } else {
        while (true) {
            const auto t_now  = std::chrono::high_resolution_clock::now();
            const auto t_diff = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_last).count();

            if (t_diff < 2000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                continue;
            }

            audio.get(2000, pcmf32_new);

            if (::vad_simple(pcmf32_new, WHISPER_SAMPLE_RATE, 1000, params.vad_thold, params.freq_thold, false)) {
                audio.get(params.length_ms, pcmf32);
                t_last = t_now;

                // done!
                break;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                continue;
            }
        }
    }

    return pcmf32;
}
