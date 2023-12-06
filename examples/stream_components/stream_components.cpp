#include <iostream>
#include "stream_components_audio.h"
#include "stream_components_params.h"
#include "stream_components_output.h"
#include "stream_components_server.h"

using namespace stream_components;

struct whisper_params {
    audio_params audio;
    server_params server;

    void initialize() {
        audio.initialize();
        server.initialize();
    }
};


void whisper_print_usage(int argc, char ** argv, const whisper_params & params);

bool whisper_params_parse(int argc, char ** argv, whisper_params & params) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
        else if (arg == "-t"   || arg == "--threads")       { params.server.n_threads     = std::stoi(argv[++i]); }
        else if (                 arg == "--step")          { params.audio.step_ms        = std::stoi(argv[++i]); }
        else if (                 arg == "--length")        { params.audio.length_ms      = std::stoi(argv[++i]); }
        else if (                 arg == "--keep")          { params.audio.keep_ms        = std::stoi(argv[++i]); }
        else if (arg == "-c"   || arg == "--capture")       { params.audio.capture_id     = std::stoi(argv[++i]); }
        //else if (arg == "-mt"  || arg == "--max-tokens")    { params.max_tokens    = std::stoi(argv[++i]); }
        else if (arg == "-ac"  || arg == "--audio-ctx")     { params.audio.audio_ctx      = std::stoi(argv[++i]); }
        else if (arg == "-vth" || arg == "--vad-thold")     { params.audio.vad_thold      = std::stof(argv[++i]); }
        else if (arg == "-fth" || arg == "--freq-thold")    { params.audio.freq_thold     = std::stof(argv[++i]); }
        else if (arg == "-su"  || arg == "--speed-up")      { params.server.speed_up      = true; }
        else if (arg == "-tr"  || arg == "--translate")     { params.server.translate     = true; }
        else if (arg == "-nf"  || arg == "--no-fallback")   { params.server.no_fallback   = true; }
        //else if (arg == "-ps"  || arg == "--print-special") { params.print_special = true; }
        else if (arg == "-kc"  || arg == "--keep-context")  { params.server.no_context    = false; }
        else if (arg == "-l"   || arg == "--language")      { params.server.language      = argv[++i]; }
        else if (arg == "-m"   || arg == "--model")         { params.server.model         = argv[++i]; }
        //else if (arg == "-f"   || arg == "--file")          { params.fname_out     = argv[++i]; }
        else if (arg == "-tdrz" || arg == "--tinydiarize")  { params.server.tinydiarize   = true; }
        //else if (arg == "-sa"  || arg == "--save-audio")    { params.save_audio    = true; }

        else {
            fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
    }

    return true;
}

void whisper_print_usage(int /*argc*/, char ** argv, const whisper_params & params) {
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h,       --help          [default] show this help message and exit\n");
    fprintf(stderr, "  -t N,     --threads N     [%-7d] number of threads to use during computation\n",    params.server.n_threads);
    fprintf(stderr, "            --step N        [%-7d] audio step size in milliseconds\n",                params.audio.step_ms);
    fprintf(stderr, "            --length N      [%-7d] audio length in milliseconds\n",                   params.audio.length_ms);
    fprintf(stderr, "            --keep N        [%-7d] audio to keep from previous step in ms\n",         params.audio.keep_ms);
    fprintf(stderr, "  -c ID,    --capture ID    [%-7d] capture device ID\n",                              params.audio.capture_id);
    //fprintf(stderr, "  -mt N,    --max-tokens N  [%-7d] maximum number of tokens per audio chunk\n",       params.max_tokens);
    fprintf(stderr, "  -ac N,    --audio-ctx N   [%-7d] audio context size (0 - all)\n",                   params.audio.audio_ctx);
    fprintf(stderr, "  -vth N,   --vad-thold N   [%-7.2f] voice activity detection threshold\n",           params.audio.vad_thold);
    fprintf(stderr, "  -fth N,   --freq-thold N  [%-7.2f] high-pass frequency cutoff\n",                   params.audio.freq_thold);
    fprintf(stderr, "  -su,      --speed-up      [%-7s] speed up audio by x2 (reduced accuracy)\n",        params.server.speed_up ? "true" : "false");
    fprintf(stderr, "  -tr,      --translate     [%-7s] translate from source language to english\n",      params.server.translate ? "true" : "false");
    fprintf(stderr, "  -nf,      --no-fallback   [%-7s] do not use temperature fallback while decoding\n", params.server.no_fallback ? "true" : "false");
    //fprintf(stderr, "  -ps,      --print-special [%-7s] print special tokens\n",                           params.print_special ? "true" : "false");
    fprintf(stderr, "  -kc,      --keep-context  [%-7s] keep context between audio chunks\n",              params.server.no_context ? "false" : "true");
    fprintf(stderr, "  -l LANG,  --language LANG [%-7s] spoken language\n",                                params.server.language.c_str());
    fprintf(stderr, "  -m FNAME, --model FNAME   [%-7s] model path\n",                                     params.server.model.c_str());
    //fprintf(stderr, "  -f FNAME, --file FNAME    [%-7s] text output file name\n",                          params.fname_out.c_str());
    fprintf(stderr, "  -tdrz,     --tinydiarize  [%-7s] enable tinydiarize (requires a tdrz model)\n",     params.server.tinydiarize ? "true" : "false");
    //fprintf(stderr, "  -sa,      --save-audio    [%-7s] save the recorded audio to a file\n",              params.save_audio ? "true" : "false");
    fprintf(stderr, "\n");
}

int main(int argc, char ** argv) {

    // Read parameters...
    whisper_params params;

    if (whisper_params_parse(argc, argv, params) == false) {
        return 1;
    }

    // Compute derived parameters
    params.initialize();

    // Check parameters
    if (params.server.language != "auto" && whisper_lang_id(params.server.language.c_str()) == -1){
        fprintf(stderr, "error: unknown language '%s'\n", params.server.language.c_str());
        whisper_print_usage(argc, argv, params);
        exit(0);
    }

    // Instantiate the audio input
    stream_components::LocalSDLMicrophone audio(params.audio);

    // Instantiate the server
    stream_components::WhisperServer server(params.server, params.audio);

    // Print the 'header'...
    WhisperOutput::server_to_json(std::cout, params.server, server.ctx);    

    // Run until Ctrl + C
    bool is_running = true;
    while (is_running) {

        // handle Ctrl + C
        is_running = sdl_poll_events();
        if (!is_running) {
            break;
        }

        // get next audio section
        auto pcmf32 = audio.get_next();

        // get the whisper output
        auto result = server.process(pcmf32.data(), pcmf32.size());

        // write the output as json to stdout (for this example)
        if (result) {
            result->transcription_to_json(std::cout);
        }
    }

    std::cout << "EXITED MAIN LOOP" << std::endl;
    return 0;
}