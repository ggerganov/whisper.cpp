#include "common.h"

#include "whisper.h"

#include "httplib.h"
#include "json.hpp"

using namespace httplib;
using json = nlohmann::json;

struct server_params {
    std::string hostname = "127.0.0.1";
    std::string public_path = "examples/server/public";
    int32_t port = 8089;
    int32_t read_timeout = 600;
    int32_t write_timeout = 600;
};

struct whisper_params {
    int32_t n_threads = std::min(12, (int32_t) std::thread::hardware_concurrency());
    int32_t n_processors = 1;
    int32_t offset_t_ms = 0;
    int32_t offset_n = 0;
    int32_t duration_ms = 0;
    int32_t progress_step = 5;
    int32_t max_context = -1;
    int32_t max_len = 0;
    int32_t best_of = 2;
    int32_t beam_size = -1;

    std::string language = "en";
    std::string prompt;
    std::string font_path = "/System/Library/Fonts/Supplemental/Courier New Bold.ttf";
    std::string model = "models/ggml-base.en.bin";
};

void whisper_print_usage(int argc, char **argv, const whisper_params &params, const server_params &serverParams) {
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s [options] file0.wav file1.wav ...\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h,        --help              [default] show this help message and exit\n");
    fprintf(stderr, "  -t N,      --threads N         [%-7d] number of threads to use during computation\n",
            params.n_threads);
    fprintf(stderr, "  -p N,      --processors N      [%-7d] number of processors to use during computation\n",
            params.n_processors);
    fprintf(stderr, "  -ot N,     --offset-t N        [%-7d] time offset in milliseconds\n", params.offset_t_ms);
    fprintf(stderr, "  -on N,     --offset-n N        [%-7d] segment index offset\n", params.offset_n);
    fprintf(stderr, "  -d  N,     --duration N        [%-7d] duration of audio to process in milliseconds\n",
            params.duration_ms);
    fprintf(stderr, "  -mc N,     --max-context N     [%-7d] maximum number of text context tokens to store\n",
            params.max_context);
    fprintf(stderr, "  -ml N,     --max-len N         [%-7d] maximum segment length in characters\n", params.max_len);
    fprintf(stderr, "  -bo N,     --best-of N         [%-7d] number of best candidates to keep\n", params.best_of);
    fprintf(stderr, "  -bs N,     --beam-size N       [%-7d] beam size for beam search\n", params.beam_size);
    fprintf(stderr, "  -l LANG,   --language LANG     [%-7s] spoken language ('auto' for auto-detect)\n",
            params.language.c_str());
    fprintf(stderr, "             --prompt PROMPT     [%-7s] initial prompt\n", params.prompt.c_str());
    fprintf(stderr, "  -m FNAME,  --model FNAME       [%-7s] model path\n", params.model.c_str());
    fprintf(stderr, "  -f FNAME,  --file FNAME        [%-7s] input WAV file path\n", "");
    fprintf(stderr, "  --host                ip address to listen (default  (default: %s)\n",
            serverParams.hostname.c_str());
    fprintf(stderr, "  --port PORT           port to listen (default  (default: %d)\n", serverParams.port);
    fprintf(stderr, "  --path PUBLIC_PATH    path from which to serve static files (default %s)\n",
            serverParams.public_path.c_str());
    fprintf(stderr, "  -to N, --timeout N    server read/write timeout in seconds (default: %d)\n",
            serverParams.read_timeout);
    fprintf(stderr, "\n");
}

bool whisper_params_parse(int argc, char **argv, whisper_params &params, server_params &serverParams) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            whisper_print_usage(argc, argv, params, serverParams);
            exit(0);
        } else if (arg == "-t" || arg == "--threads") { params.n_threads = std::stoi(argv[++i]); }
        else if (arg == "-p" || arg == "--processors") { params.n_processors = std::stoi(argv[++i]); }
        else if (arg == "-ot" || arg == "--offset-t") { params.offset_t_ms = std::stoi(argv[++i]); }
        else if (arg == "-on" || arg == "--offset-n") { params.offset_n = std::stoi(argv[++i]); }
        else if (arg == "-d" || arg == "--duration") { params.duration_ms = std::stoi(argv[++i]); }
        else if (arg == "-mc" || arg == "--max-context") { params.max_context = std::stoi(argv[++i]); }
        else if (arg == "-ml" || arg == "--max-len") { params.max_len = std::stoi(argv[++i]); }
        else if (arg == "-bo" || arg == "--best-of") { params.best_of = std::stoi(argv[++i]); }
        else if (arg == "-bs" || arg == "--beam-size") { params.beam_size = std::stoi(argv[++i]); }
        else if (arg == "-l" || arg == "--language") { params.language = argv[++i]; }
        else if (arg == "--prompt") { params.prompt = argv[++i]; }
        else if (arg == "-m" || arg == "--model") { params.model = argv[++i]; }
        else if (arg == "--port") {
            serverParams.port = std::stoi(argv[i]);
        } else if (arg == "--host") {
            serverParams.hostname = argv[i];
        } else if (arg == "--timeout" || arg == "-to") {
            serverParams.read_timeout = std::stoi(argv[i]);
            serverParams.write_timeout = std::stoi(argv[i]);
        } else {
            fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            whisper_print_usage(argc, argv, params, serverParams);
            exit(0);
        }
    }

    return true;
}

char *escape_double_quotes_and_backslashes(const char *str) {
    if (str == NULL) {
        return NULL;
    }

    size_t escaped_length = strlen(str) + 1;

    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '"' || str[i] == '\\') {
            escaped_length++;
        }
    }

    char *escaped = (char *)calloc(escaped_length, 1); // pre-zeroed
    if (escaped == NULL) {
        return NULL;
    }

    size_t pos = 0;
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '"' || str[i] == '\\') {
            escaped[pos++] = '\\';
        }
        escaped[pos++] = str[i];
    }

    // no need to set zero due to calloc() being used prior

    return escaped;
}

std::string to_timestamp(int64_t t, bool comma = false) {
    int64_t msec = t * 10;
    int64_t hr = msec / (1000 * 60 * 60);
    msec = msec - hr * (1000 * 60 * 60);
    int64_t min = msec / (1000 * 60);
    msec = msec - min * (1000 * 60);
    int64_t sec = msec / 1000;
    msec = msec - sec * 1000;

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d%s%03d", (int) hr, (int) min, (int) sec, comma ? "," : ".", (int) msec);

    return std::string(buf);
}

bool output_json(struct whisper_context * ctx, std::stringstream &fout, const whisper_params & params) {
    int indent = 0;

    auto doindent = [&]() {
        for (int i = 0; i < indent; i++) fout << "\t";
    };

    auto start_arr = [&](const char *name) {
        doindent();
        fout << "\"" << name << "\": [\n";
        indent++;
    };

    auto end_arr = [&](bool end) {
        indent--;
        doindent();
        fout << (end ? "]\n" : "},\n");
    };

    auto start_obj = [&](const char *name) {
        doindent();
        if (name) {
            fout << "\"" << name << "\": {\n";
        } else {
            fout << "{\n";
        }
        indent++;
    };

    auto end_obj = [&](bool end) {
        indent--;
        doindent();
        fout << (end ? "}\n" : "},\n");
    };

    auto start_value = [&](const char *name) {
        doindent();
        fout << "\"" << name << "\": ";
    };

    auto value_s = [&](const char *name, const char *val, bool end) {
        start_value(name);
        char * val_escaped = escape_double_quotes_and_backslashes(val);
        fout << "\"" << val_escaped << (end ? "\"\n" : "\",\n");
        free(val_escaped);
    };

    auto end_value = [&](bool end) {
        fout << (end ? "\n" : ",\n");
    };

    auto value_i = [&](const char *name, const int64_t val, bool end) {
        start_value(name);
        fout << val;
        end_value(end);
    };

    auto value_b = [&](const char *name, const bool val, bool end) {
        start_value(name);
        fout << (val ? "true" : "false");
        end_value(end);
    };

    start_obj(nullptr);
    value_s("systeminfo", whisper_print_system_info(), false);
    start_obj("model");
    value_s("type", whisper_model_type_readable(ctx), false);
    value_b("multilingual", whisper_is_multilingual(ctx), false);
    value_i("vocab", whisper_model_n_vocab(ctx), false);
    start_obj("audio");
    value_i("ctx", whisper_model_n_audio_ctx(ctx), false);
    value_i("state", whisper_model_n_audio_state(ctx), false);
    value_i("head", whisper_model_n_audio_head(ctx), false);
    value_i("layer", whisper_model_n_audio_layer(ctx), true);
    end_obj(false);
    start_obj("text");
    value_i("ctx", whisper_model_n_text_ctx(ctx), false);
    value_i("state", whisper_model_n_text_state(ctx), false);
    value_i("head", whisper_model_n_text_head(ctx), false);
    value_i("layer", whisper_model_n_text_layer(ctx), true);
    end_obj(false);
    value_i("mels", whisper_model_n_mels(ctx), false);
    value_i("ftype", whisper_model_ftype(ctx), true);
    end_obj(false);
    start_obj("params");
    value_s("model", params.model.c_str(), false);
    value_s("language", params.language.c_str(), true);
    end_obj(false);
    start_obj("result");
    value_s("language", whisper_lang_str(whisper_full_lang_id(ctx)), true);
    end_obj(false);
    start_arr("transcription");

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);

        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

        start_obj(nullptr);
        start_obj("timestamps");
        value_s("from", to_timestamp(t0, true).c_str(), false);
        value_s("to", to_timestamp(t1, true).c_str(), true);
        end_obj(false);
        start_obj("offsets");
        value_i("from", t0 * 10, false);
        value_i("to", t1 * 10, true);
        end_obj(false);
        value_s("text", text, true);

        end_obj(i == (n_segments - 1));
    }

    end_arr(true);
    end_obj(true);
    return true;
}

int main(int argc, char **argv) {
    server_params serverParams;
    whisper_params whisperParams;

    whisper_params_parse(argc, argv, whisperParams, serverParams);

    // TODO check that --model is passed

    struct whisper_context *ctx = whisper_init_from_file(whisperParams.model.c_str());
    if (ctx == nullptr) {
        fprintf(stderr, "error: failed to initialize whisper context\n");
        return 3;
    }

//
//    wparams.strategy = whisperParams.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY;
//
//    wparams.print_realtime = false;
//    wparams.language = whisperParams.language.c_str();
//    wparams.n_threads = whisperParams.n_threads;
//    wparams.n_max_text_ctx = whisperParams.max_context >= 0 ? whisperParams.max_context : wparams.n_max_text_ctx;
//    wparams.offset_ms = whisperParams.offset_t_ms;
//    wparams.duration_ms = whisperParams.duration_ms;
//
//    wparams.token_timestamps = whisperParams.max_len > 0;
//
//
//    wparams.initial_prompt = whisperParams.prompt.c_str();
//
//    wparams.greedy.best_of = whisperParams.best_of;
//    wparams.beam_search.beam_size = whisperParams.beam_size;

    Server server;

    server.set_default_headers({{"Server",                       "whisper.cpp"},
                                {"Access-Control-Allow-Origin",  "*"},
                                {"Access-Control-Allow-Headers", "content-type"}});

    server.Post("/convert", [&ctx, &whisperParams](const Request &req, Response &res) {
        fprintf(stderr, "Received request\n");
        auto body = json::parse(req.body);
        std::string file_in;

        if (body.count("filename") != 0) {
            file_in = body["filename"];
        } else {
            // TODO return error json
        }

        fprintf(stderr, "Reading %s\n", file_in.c_str());

        std::vector<float> pcmf32;               // mono-channel F32 PCM
        std::vector<std::vector<float>> pcmf32s; // stereo-channel F32 PCM

        if (!::read_wav(file_in, pcmf32, pcmf32s, false)) {
            fprintf(stderr, "error: failed to read WAV file '%s'\n", file_in.c_str());
            // TODO return error json
            res.status = 404;
            return 9;
        }

        whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

//        {
//            static bool is_aborted = false; // NOTE: this should be atomic to avoid data race
//
//            wparams.encoder_begin_callback = [](struct whisper_context * /*ctx*/, struct whisper_state * /*state*/,
//                                                void *user_data) {
//                bool is_aborted = *(bool *) user_data;
//                return !is_aborted;
//            };
//            wparams.encoder_begin_callback_user_data = &is_aborted;
//        }

        fprintf(stderr, "processors: %i", whisperParams.n_processors);
        if (whisper_full_parallel(ctx, wparams, pcmf32.data(), pcmf32.size(), whisperParams.n_processors) != 0) {
            fprintf(stderr, "%s: failed to process audio\n");
            res.status = 500;
            return 10;
        }

        std::stringstream out;
        output_json(ctx, out, whisperParams);

        fprintf(stderr, "output: %s", out.str().c_str());

        res.status = 200;
        res.set_content(out.str(), "application/json");
        return 0;
    });

    server.set_exception_handler([](const Request &, Response &res, std::exception_ptr ep) {
        const char fmt[] = "500 Internal Server Error\n%s";
        char buf[BUFSIZ];
        try {
            std::rethrow_exception(std::move(ep));
        } catch (std::exception &e) {
            snprintf(buf, sizeof(buf), fmt, e.what());
        } catch (...) {
            snprintf(buf, sizeof(buf), fmt, "Unknown Exception");
        }
        res.set_content(buf, "text/plain");
        res.status = 500;
    });

    server.set_error_handler([](const Request &, Response &res) {
        if (res.status == 400) {
            res.set_content("Invalid request", "text/plain");
        } else if (res.status != 500) {
            res.set_content("File Not Found", "text/plain");
            res.status = 404;
        }
    });

    server.set_read_timeout(serverParams.read_timeout);
    server.set_write_timeout(serverParams.write_timeout);

    if (!server.bind_to_port(serverParams.hostname, serverParams.port)) {
        fprintf(stderr, "\ncouldn't bind to server socket: hostname=%s:%d\n\n", serverParams.hostname.c_str(),
                serverParams.port);
        return 1;
    }

    fprintf(stderr, "\nListening on hostname=%s:%d\n\n", serverParams.hostname.c_str(), serverParams.port);

    if (!server.listen_after_bind()) {
        return 1;
    }

    return 0;
}