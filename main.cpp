#include "ggml.h"

#define USE_FLASH_ATTN
#define USE_FLASH_FF

// third-party utilities
// use your favorite implementations
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <vector>

// available whisper models
enum e_model {
    MODEL_UNKNOWN,
    MODEL_TINY,
    MODEL_BASE,
    MODEL_SMALL,
    MODEL_MEDIUM,
    MODEL_LARGE,
};

const std::map<std::string, std::pair<int, std::string>> g_lang = {
    { "en",  { 0,  "english",         } },
    { "zh",  { 1,  "chinese",         } },
    { "de",  { 2,  "german",          } },
    { "es",  { 3,  "spanish",         } },
    { "ru",  { 4,  "russian",         } },
    { "ko",  { 5,  "korean",          } },
    { "fr",  { 6,  "french",          } },
    { "ja",  { 7,  "japanese",        } },
    { "pt",  { 8,  "portuguese",      } },
    { "tr",  { 9,  "turkish",         } },
    { "pl",  { 10, "polish",          } },
    { "ca",  { 11,  "catalan",        } },
    { "nl",  { 12,  "dutch",          } },
    { "ar",  { 13,  "arabic",         } },
    { "sv",  { 14,  "swedish",        } },
    { "it",  { 15,  "italian",        } },
    { "id",  { 16,  "indonesian",     } },
    { "hi",  { 17,  "hindi",          } },
    { "fi",  { 18,  "finnish",        } },
    { "vi",  { 19,  "vietnamese",     } },
    { "iw",  { 20,  "hebrew",         } },
    { "uk",  { 21,  "ukrainian",      } },
    { "el",  { 22,  "greek",          } },
    { "ms",  { 23,  "malay",          } },
    { "cs",  { 24,  "czech",          } },
    { "ro",  { 25,  "romanian",       } },
    { "da",  { 26,  "danish",         } },
    { "hu",  { 27,  "hungarian",      } },
    { "ta",  { 28,  "tamil",          } },
    { "no",  { 29,  "norwegian",      } },
    { "th",  { 30,  "thai",           } },
    { "ur",  { 31,  "urdu",           } },
    { "hr",  { 32,  "croatian",       } },
    { "bg",  { 33,  "bulgarian",      } },
    { "lt",  { 34,  "lithuanian",     } },
    { "la",  { 35,  "latin",          } },
    { "mi",  { 36,  "maori",          } },
    { "ml",  { 37,  "malayalam",      } },
    { "cy",  { 38,  "welsh",          } },
    { "sk",  { 39,  "slovak",         } },
    { "te",  { 40,  "telugu",         } },
    { "fa",  { 41,  "persian",        } },
    { "lv",  { 42,  "latvian",        } },
    { "bn",  { 43,  "bengali",        } },
    { "sr",  { 44,  "serbian",        } },
    { "az",  { 45,  "azerbaijani",    } },
    { "sl",  { 46,  "slovenian",      } },
    { "kn",  { 47,  "kannada",        } },
    { "et",  { 48,  "estonian",       } },
    { "mk",  { 49,  "macedonian",     } },
    { "br",  { 50,  "breton",         } },
    { "eu",  { 51,  "basque",         } },
    { "is",  { 52,  "icelandic",      } },
    { "hy",  { 53,  "armenian",       } },
    { "ne",  { 54,  "nepali",         } },
    { "mn",  { 55,  "mongolian",      } },
    { "bs",  { 56,  "bosnian",        } },
    { "kk",  { 57,  "kazakh",         } },
    { "sq",  { 58,  "albanian",       } },
    { "sw",  { 59,  "swahili",        } },
    { "gl",  { 60,  "galician",       } },
    { "mr",  { 61,  "marathi",        } },
    { "pa",  { 62,  "punjabi",        } },
    { "si",  { 63,  "sinhala",        } },
    { "km",  { 64,  "khmer",          } },
    { "sn",  { 65,  "shona",          } },
    { "yo",  { 66,  "yoruba",         } },
    { "so",  { 67,  "somali",         } },
    { "af",  { 68,  "afrikaans",      } },
    { "oc",  { 69,  "occitan",        } },
    { "ka",  { 70,  "georgian",       } },
    { "be",  { 71,  "belarusian",     } },
    { "tg",  { 72,  "tajik",          } },
    { "sd",  { 73,  "sindhi",         } },
    { "gu",  { 74,  "gujarati",       } },
    { "am",  { 75,  "amharic",        } },
    { "yi",  { 76,  "yiddish",        } },
    { "lo",  { 77,  "lao",            } },
    { "uz",  { 78,  "uzbek",          } },
    { "fo",  { 79,  "faroese",        } },
    { "ht",  { 80,  "haitian creole", } },
    { "ps",  { 81,  "pashto",         } },
    { "tk",  { 82,  "turkmen",        } },
    { "nn",  { 83,  "nynorsk",        } },
    { "mt",  { 84,  "maltese",        } },
    { "sa",  { 85,  "sanskrit",       } },
    { "lb",  { 86,  "luxembourgish",  } },
    { "my",  { 87,  "myanmar",        } },
    { "bo",  { 88,  "tibetan",        } },
    { "tl",  { 89,  "tagalog",        } },
    { "mg",  { 90,  "malagasy",       } },
    { "as",  { 91,  "assamese",       } },
    { "tt",  { 92,  "tatar",          } },
    { "haw", { 93,  "hawaiian",       } },
    { "ln",  { 94,  "lingala",        } },
    { "ha",  { 95,  "hausa",          } },
    { "ba",  { 96,  "bashkir",        } },
    { "jw",  { 97,  "javanese",       } },
    { "su",  { 98,  "sundanese",      } },
};

const size_t MB = 1024*1024;

const std::map<e_model, size_t> MEM_REQ_MODEL = {
    { MODEL_TINY,     86ull*MB },
    { MODEL_BASE,    165ull*MB },
    { MODEL_SMALL,   540ull*MB },
    { MODEL_MEDIUM, 1650ull*MB },
    { MODEL_LARGE,  3260ull*MB },
};

const std::map<e_model, size_t> MEM_REQ_ENCODE = {
    { MODEL_TINY,     80ull*MB },
    { MODEL_BASE,    128ull*MB },
    { MODEL_SMALL,   300ull*MB },
    { MODEL_MEDIUM,  680ull*MB },
    { MODEL_LARGE,  1100ull*MB },
};

const std::map<e_model, size_t> MEM_REQ_ENCODE_LAYER = {
    { MODEL_TINY,     64ull*MB },
    { MODEL_BASE,     84ull*MB },
    { MODEL_SMALL,   128ull*MB },
    { MODEL_MEDIUM,  172ull*MB },
    { MODEL_LARGE,   216ull*MB },
};

const std::map<e_model, size_t> MEM_REQ_DECODE = {
    { MODEL_TINY,    190ull*MB },
    { MODEL_BASE,    190ull*MB },
    { MODEL_SMALL,   190ull*MB },
    { MODEL_MEDIUM,  200ull*MB },
    { MODEL_LARGE,   200ull*MB },
};

const std::map<e_model, size_t> MEM_REQ_DECODE_LAYER = {
    { MODEL_TINY,     32ull*MB },
    { MODEL_BASE,     44ull*MB },
    { MODEL_SMALL,    64ull*MB },
    { MODEL_MEDIUM,   84ull*MB },
    { MODEL_LARGE,   110ull*MB },
};

const int SAMPLE_RATE = 16000;
const int N_FFT       = 400;
const int N_MEL       = 80;
const int HOP_LENGTH  = 160;
const int CHUNK_SIZE  = 30; // seconds

struct whisper_mel {
    int n_len;
    int n_mel;

    std::vector<float> data;
};

struct whisper_filters {
    int32_t n_mel;
    int32_t n_fft;

    std::vector<float> data;
};

struct whisper_vocab {
    using id    = int32_t;
    using token = std::string;

    int n_vocab = 51864;

    std::map<token, id> token_to_id;
    std::map<id, token> id_to_token;

    id token_eot  = 50256;
    id token_sot  = 50257;
    id token_prev = 50360;
    id token_solm = 50361; // ??
    id token_beg  = 50363;

    // available tasks
    const id token_translate  = 50358;
    const id token_transcribe = 50359;

    bool is_multilingual() const {
        return n_vocab == 51865;
    }
};

// command-line parameters
struct whisper_params {
    int32_t seed      = -1; // RNG seed, not used currently
    int32_t n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());

    // sampling parameter - used for the greedy strategy
    int32_t max_tokens_per_iter = 64;

    bool verbose              = false;
    bool translate            = false;
    bool print_special_tokens = false;

    std::string language  = "en";
    std::string model     = "models/ggml-base.en.bin";
    std::string fname_inp = "samples/jfk.wav";
};

void whisper_print_usage(int argc, char ** argv, const whisper_params & params);

bool whisper_params_parse(int argc, char ** argv, whisper_params & params) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-s" || arg == "--seed") {
            params.seed = std::stoi(argv[++i]);
        } else if (arg == "-t" || arg == "--threads") {
            params.n_threads = std::stoi(argv[++i]);
        } else if (arg == "-T" || arg == "--tokens") {
            params.max_tokens_per_iter = std::stoi(argv[++i]);
        } else if (arg == "-v" || arg == "--verbose") {
            params.verbose = true;
        } else if (arg == "--translate") {
            params.translate = true;
        } else if (arg == "-l" || arg == "--language") {
            params.language = argv[++i];
            if (g_lang.find(params.language) == g_lang.end()) {
                fprintf(stderr, "error: unknown language '%s'\n", params.language.c_str());
                whisper_print_usage(argc, argv, params);
                exit(0);
            }
        } else if (arg == "-ps" || arg == "--print_special") {
            params.print_special_tokens = true;
        } else if (arg == "-m" || arg == "--model") {
            params.model = argv[++i];
        } else if (arg == "-f" || arg == "--file") {
            params.fname_inp = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            whisper_print_usage(argc, argv, params);
            exit(0);
        } else {
            fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
    }

    return true;
}

void whisper_print_usage(int argc, char ** argv, const whisper_params & params) {
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h,       --help           show this help message and exit\n");
    fprintf(stderr, "  -s SEED,  --seed SEED      RNG seed (default: -1)\n");
    fprintf(stderr, "  -t N,     --threads N      number of threads to use during computation (default: %d)\n", params.n_threads);
    fprintf(stderr, "  -T N,     --tokens N       maximum number of tokens to generate per iteration (default: %d)\n", params.max_tokens_per_iter);
    fprintf(stderr, "  -v,       --verbose        verbose output\n");
    fprintf(stderr, "            --translate      translate from source language to english\n");
    fprintf(stderr, "  -ps,      --print_special  print special tokens\n");
    fprintf(stderr, "  -l LANG,  --language LANG  spoken language (default: %s)\n", params.language.c_str());
    fprintf(stderr, "  -m FNAME, --model FNAME    model path (default: %s)\n", params.model.c_str());
    fprintf(stderr, "  -f FNAME, --file FNAME     input WAV file path (default: %s)\n", params.fname_inp.c_str());
    fprintf(stderr, "\n");
}


// medium
// hparams: {
// 'n_mels': 80,
// 'n_vocab': 51864,
// 'n_audio_ctx': 1500,
// 'n_audio_state': 1024,
// 'n_audio_head': 16,
// 'n_audio_layer': 24,
// 'n_text_ctx': 448,
// 'n_text_state': 1024,
// 'n_text_head': 16,
// 'n_text_layer': 24
// }
//
// default hparams (Whisper tiny)
struct whisper_hparams {
    int32_t n_vocab       = 51864;
    int32_t n_audio_ctx   = 1500;
    int32_t n_audio_state = 384;
    int32_t n_audio_head  = 6;
    int32_t n_audio_layer = 4;
    int32_t n_text_ctx    = 448;
    int32_t n_text_state  = 384;
    int32_t n_text_head   = 6;
    int32_t n_text_layer  = 4;
    int32_t n_mels        = 80;
    int32_t f16           = 1;
};

// audio encoding layer
struct whisper_layer_encoder {
    // encoder.blocks.*.attn_ln
    struct ggml_tensor * attn_ln_0_w;
    struct ggml_tensor * attn_ln_0_b;

    // encoder.blocks.*.attn.out
    struct ggml_tensor * attn_ln_1_w;
    struct ggml_tensor * attn_ln_1_b;

    // encoder.blocks.*.attn.query
    struct ggml_tensor * attn_q_w;
    struct ggml_tensor * attn_q_b;

    // encoder.blocks.*.attn.key
    struct ggml_tensor * attn_k_w;

    // encoder.blocks.*.attn.value
    struct ggml_tensor * attn_v_w;
    struct ggml_tensor * attn_v_b;

    // encoder.blocks.*.mlp_ln
    struct ggml_tensor * mlp_ln_w;
    struct ggml_tensor * mlp_ln_b;

    // encoder.blocks.*.mlp.0
    struct ggml_tensor * mlp_0_w;
    struct ggml_tensor * mlp_0_b;

    // encoder.blocks.*.mlp.2
    struct ggml_tensor * mlp_1_w;
    struct ggml_tensor * mlp_1_b;
};

// token decoding layer
struct whisper_layer_decoder {
    // decoder.blocks.*.attn_ln
    struct ggml_tensor * attn_ln_0_w;
    struct ggml_tensor * attn_ln_0_b;

    // decoder.blocks.*.attn.out
    struct ggml_tensor * attn_ln_1_w;
    struct ggml_tensor * attn_ln_1_b;

    // decoder.blocks.*.attn.query
    struct ggml_tensor * attn_q_w;
    struct ggml_tensor * attn_q_b;

    // decoder.blocks.*.attn.key
    struct ggml_tensor * attn_k_w;

    // decoder.blocks.*.attn.value
    struct ggml_tensor * attn_v_w;
    struct ggml_tensor * attn_v_b;

    // decoder.blocks.*.cross_attn_ln
    struct ggml_tensor * cross_attn_ln_0_w;
    struct ggml_tensor * cross_attn_ln_0_b;

    // decoder.blocks.*.cross_attn.out
    struct ggml_tensor * cross_attn_ln_1_w;
    struct ggml_tensor * cross_attn_ln_1_b;

    // decoder.blocks.*.cross_attn.query
    struct ggml_tensor * cross_attn_q_w;
    struct ggml_tensor * cross_attn_q_b;

    // decoder.blocks.*.cross_attn.key
    struct ggml_tensor * cross_attn_k_w;

    // decoder.blocks.*.cross_attn.value
    struct ggml_tensor * cross_attn_v_w;
    struct ggml_tensor * cross_attn_v_b;

    // decoder.blocks.*.mlp_ln
    struct ggml_tensor * mlp_ln_w;
    struct ggml_tensor * mlp_ln_b;

    // decoder.blocks.*.mlp.0
    struct ggml_tensor * mlp_0_w;
    struct ggml_tensor * mlp_0_b;

    // decoder.blocks.*.mlp.2
    struct ggml_tensor * mlp_1_w;
    struct ggml_tensor * mlp_1_b;
};

struct whisper_model {
    e_model type = MODEL_UNKNOWN;

    whisper_hparams hparams;
    whisper_filters filters;

    // encoder.positional_embedding
    struct ggml_tensor * e_pe;

    // encoder.conv1
    struct ggml_tensor * e_conv_1_w;
    struct ggml_tensor * e_conv_1_b;

    // encoder.conv2
    struct ggml_tensor * e_conv_2_w;
    struct ggml_tensor * e_conv_2_b;

    // encoder.ln_post
    struct ggml_tensor * e_ln_w;
    struct ggml_tensor * e_ln_b;

    // decoder.positional_embedding
    struct ggml_tensor * d_pe; // DD

    // decoder.token_embedding
    struct ggml_tensor * d_te; // DD

    // decoder.ln
    struct ggml_tensor * d_ln_w; // DD
    struct ggml_tensor * d_ln_b; // DD

    std::vector<whisper_layer_encoder> layers_encoder;
    std::vector<whisper_layer_decoder> layers_decoder;

    // key + value memory
    struct ggml_tensor * memory_k;
    struct ggml_tensor * memory_v;

    struct ggml_tensor * memory_cross_k;
    struct ggml_tensor * memory_cross_v;

    //
    struct ggml_context * ctx;
    std::map<std::string, struct ggml_tensor *> tensors;
};

// load the model from a ggml file
//
// file format:
//
//   - hparams
//   - pre-computed mel filters
//   - vocab
//   - weights
//
// see the convert-pt-to-ggml.py script for details
//
bool whisper_model_load(const std::string & fname, whisper_model & model, whisper_vocab & vocab) {
    printf("%s: loading model from '%s'\n", __func__, fname.c_str());

    auto fin = std::ifstream(fname, std::ios::binary);
    if (!fin) {
        fprintf(stderr, "%s: failed to open '%s'\n", __func__, fname.c_str());
        return false;
    }

    // verify magic
    {
        uint32_t magic;
        fin.read((char *) &magic, sizeof(magic));
        if (magic != 0x67676d6c) {
            fprintf(stderr, "%s: invalid model file '%s' (bad magic)\n", __func__, fname.c_str());
            return false;
        }
    }

    //load hparams
    {
        auto & hparams = model.hparams;

        fin.read((char *) &hparams.n_vocab,       sizeof(hparams.n_vocab));
        fin.read((char *) &hparams.n_audio_ctx,   sizeof(hparams.n_audio_ctx));
        fin.read((char *) &hparams.n_audio_state, sizeof(hparams.n_audio_state));
        fin.read((char *) &hparams.n_audio_head,  sizeof(hparams.n_audio_head));
        fin.read((char *) &hparams.n_audio_layer, sizeof(hparams.n_audio_layer));
        fin.read((char *) &hparams.n_text_ctx,    sizeof(hparams.n_text_ctx));
        fin.read((char *) &hparams.n_text_state,  sizeof(hparams.n_text_state));
        fin.read((char *) &hparams.n_text_head,   sizeof(hparams.n_text_head));
        fin.read((char *) &hparams.n_text_layer,  sizeof(hparams.n_text_layer));
        fin.read((char *) &hparams.n_mels,        sizeof(hparams.n_mels));
        fin.read((char *) &hparams.f16,           sizeof(hparams.f16));

        assert(hparams.n_text_state == hparams.n_audio_state);

        if (hparams.n_audio_layer == 4) {
            model.type = e_model::MODEL_TINY;
        }

        if (hparams.n_audio_layer == 6) {
            model.type = e_model::MODEL_BASE;
        }

        if (hparams.n_audio_layer == 12) {
            model.type = e_model::MODEL_SMALL;
        }

        if (hparams.n_audio_layer == 24) {
            model.type = e_model::MODEL_MEDIUM;
        }

        if (hparams.n_audio_layer == 32) {
            model.type = e_model::MODEL_LARGE;
        }

        printf("%s: n_vocab       = %d\n", __func__, hparams.n_vocab);
        printf("%s: n_audio_ctx   = %d\n", __func__, hparams.n_audio_ctx);
        printf("%s: n_audio_state = %d\n", __func__, hparams.n_audio_state);
        printf("%s: n_audio_head  = %d\n", __func__, hparams.n_audio_head);
        printf("%s: n_audio_layer = %d\n", __func__, hparams.n_audio_layer);
        printf("%s: n_text_ctx    = %d\n", __func__, hparams.n_text_ctx);
        printf("%s: n_text_state  = %d\n", __func__, hparams.n_text_state);
        printf("%s: n_text_head   = %d\n", __func__, hparams.n_text_head);
        printf("%s: n_text_layer  = %d\n", __func__, hparams.n_text_layer);
        printf("%s: n_mels        = %d\n", __func__, hparams.n_mels);
        printf("%s: f16           = %d\n", __func__, hparams.f16);
        printf("%s: type          = %d\n", __func__, model.type);

        // this is the total memory required to run the inference
        const size_t mem_required =
                   MEM_REQ_MODEL.at(model.type) +
                  MEM_REQ_ENCODE.at(model.type) +
            MEM_REQ_ENCODE_LAYER.at(model.type) +
                  MEM_REQ_DECODE.at(model.type) +
            MEM_REQ_DECODE_LAYER.at(model.type);

        printf("%s: mem_required  = %.2f MB\n", __func__, mem_required / 1024.0 / 1024.0);
    }

    // load mel filters
    {
        auto & filters = model.filters;

        fin.read((char *) &filters.n_mel, sizeof(filters.n_mel));
        fin.read((char *) &filters.n_fft, sizeof(filters.n_fft));

        filters.data.resize(filters.n_mel * filters.n_fft);
        fin.read((char *) filters.data.data(), filters.data.size() * sizeof(float));
    }

    // load vocab
    {
        int32_t n_vocab = 0;
        fin.read((char *) &n_vocab, sizeof(n_vocab));

        //if (n_vocab != model.hparams.n_vocab) {
        //    fprintf(stderr, "%s: invalid model file '%s' (bad vocab size %d != %d)\n",
        //            __func__, fname.c_str(), n_vocab, model.hparams.n_vocab);
        //    return false;
        //}

        std::string word;
        for (int i = 0; i < n_vocab; i++) {
            uint32_t len;
            fin.read((char *) &len, sizeof(len));

            word.resize(len);
            fin.read((char *) word.data(), len);

            vocab.token_to_id[word] = i;
            vocab.id_to_token[i] = word;

            //printf("%s: vocab[%d] = '%s'\n", __func__, i, word.c_str());
        }

        vocab.n_vocab = model.hparams.n_vocab;
        if (vocab.is_multilingual()) {
            vocab.token_eot++;
            vocab.token_sot++;
            vocab.token_prev++;
            vocab.token_solm++;
            vocab.token_beg++;
        }

        if (n_vocab < model.hparams.n_vocab) {
            printf("%s: adding %d extra tokens\n", __func__, model.hparams.n_vocab - n_vocab);
            for (int i = n_vocab; i < model.hparams.n_vocab; i++) {
                if (i > vocab.token_beg) {
                    word = "[_TT_" + std::to_string(i - vocab.token_beg) + "]";
                } else if (i == vocab.token_eot) {
                    word = "[_EOT_]";
                } else if (i == vocab.token_sot) {
                    word = "[_SOT_]";
                } else if (i == vocab.token_prev) {
                    word = "[_PREV_]";
                } else if (i == vocab.token_beg) {
                    word = "[_BEG_]";
                } else {
                    word = "[_extra_token_" + std::to_string(i) + "]";
                }
                vocab.token_to_id[word] = i;
                vocab.id_to_token[i] = word;
            }
        }
    }

    // for the big tensors, we have the option to store the data in 16-bit floats
    // in order to save memory and also to speed up the computation
    const ggml_type wtype = model.hparams.f16 ? GGML_TYPE_F16 : GGML_TYPE_F32;

    auto & ctx = model.ctx;

    size_t ctx_size = 0;

    {
        const auto & hparams = model.hparams;

        const int n_vocab = hparams.n_vocab;

        const int n_audio_ctx   = hparams.n_audio_ctx;
        const int n_audio_state = hparams.n_audio_state;
        const int n_audio_layer = hparams.n_audio_layer;

        const int n_text_ctx = hparams.n_text_ctx;
        const int n_text_state = hparams.n_text_state;
        const int n_text_layer = hparams.n_text_layer;

        const int n_mels = hparams.n_mels;

        // encoder
        {
            // TODO: F16 .. maybe not?
            ctx_size += n_audio_ctx*n_audio_state*ggml_type_size(GGML_TYPE_F32); // e_pe;

            ctx_size += 3*n_mels*n_audio_state*ggml_type_size(wtype);         // e_conv_1_w
            ctx_size +=          n_audio_state*ggml_type_size(GGML_TYPE_F32); // e_conv_1_b

            ctx_size += 3*n_audio_state*n_audio_state*ggml_type_size(wtype);         // e_conv_2_w
            ctx_size +=                 n_audio_state*ggml_type_size(GGML_TYPE_F32); // e_conv_2_b

            ctx_size += n_audio_state*ggml_type_size(GGML_TYPE_F32); // e_ln_w;
            ctx_size += n_audio_state*ggml_type_size(GGML_TYPE_F32); // e_ln_b;
        }

        // decoder
        {
            // TODO: F16 .. maybe not?
            ctx_size += n_text_ctx*n_text_state*ggml_type_size(GGML_TYPE_F32); // d_pe;

            ctx_size += n_vocab*n_text_state*ggml_type_size(wtype); // d_te;

            ctx_size += n_text_state*ggml_type_size(GGML_TYPE_F32); // d_ln_w;
            ctx_size += n_text_state*ggml_type_size(GGML_TYPE_F32); // d_ln_b;
        }

        // encoder layers
        {
            ctx_size += n_audio_layer*(n_audio_state*ggml_type_size(GGML_TYPE_F32)); // mlp_ln_w
            ctx_size += n_audio_layer*(n_audio_state*ggml_type_size(GGML_TYPE_F32)); // mlp_ln_b

            ctx_size += n_audio_layer*(4*n_audio_state*n_audio_state*ggml_type_size(wtype));         // mlp_0_w
            ctx_size += n_audio_layer*(              4*n_audio_state*ggml_type_size(GGML_TYPE_F32)); // mlp_0_b

            ctx_size += n_audio_layer*(4*n_audio_state*n_audio_state*ggml_type_size(wtype));         // mlp_1_w
            ctx_size += n_audio_layer*(                n_audio_state*ggml_type_size(GGML_TYPE_F32)); // mlp_1_b

            ctx_size += n_audio_layer*(n_audio_state*ggml_type_size(GGML_TYPE_F32)); // attn_ln_0_w
            ctx_size += n_audio_layer*(n_audio_state*ggml_type_size(GGML_TYPE_F32)); // attn_ln_0_b

            ctx_size += n_audio_layer*(n_audio_state*n_audio_state*ggml_type_size(wtype));         // attn_q_w
            ctx_size += n_audio_layer*(              n_audio_state*ggml_type_size(GGML_TYPE_F32)); // attn_q_b

            ctx_size += n_audio_layer*(n_audio_state*n_audio_state*ggml_type_size(wtype)); // attn_k_w

            ctx_size += n_audio_layer*(n_audio_state*n_audio_state*ggml_type_size(wtype));         // attn_v_w
            ctx_size += n_audio_layer*(              n_audio_state*ggml_type_size(GGML_TYPE_F32)); // attn_v_b

            ctx_size += n_audio_layer*(n_audio_state*n_audio_state*ggml_type_size(wtype));         // attn_ln_1_w
            ctx_size += n_audio_layer*(              n_audio_state*ggml_type_size(GGML_TYPE_F32)); // attn_ln_1_b
        }

        // decoder layers
        {
            ctx_size += n_text_layer*(n_text_state*ggml_type_size(GGML_TYPE_F32)); // mlp_ln_w
            ctx_size += n_text_layer*(n_text_state*ggml_type_size(GGML_TYPE_F32)); // mlp_ln_b

            ctx_size += n_text_layer*(4*n_text_state*n_text_state*ggml_type_size(wtype));         // mlp_0_w
            ctx_size += n_text_layer*(             4*n_text_state*ggml_type_size(GGML_TYPE_F32)); // mlp_0_b

            ctx_size += n_text_layer*(4*n_text_state*n_text_state*ggml_type_size(wtype));         // mlp_1_w
            ctx_size += n_text_layer*(               n_text_state*ggml_type_size(GGML_TYPE_F32)); // mlp_1_b

            ctx_size += n_text_layer*(n_text_state*ggml_type_size(GGML_TYPE_F32)); // attn_ln_0_w
            ctx_size += n_text_layer*(n_text_state*ggml_type_size(GGML_TYPE_F32)); // attn_ln_0_b

            ctx_size += n_text_layer*(n_text_state*n_text_state*ggml_type_size(wtype));         // attn_q_w
            ctx_size += n_text_layer*(             n_text_state*ggml_type_size(GGML_TYPE_F32)); // attn_q_b

            ctx_size += n_text_layer*(n_text_state*n_text_state*ggml_type_size(wtype)); // attn_k_w

            ctx_size += n_text_layer*(n_text_state*n_text_state*ggml_type_size(wtype));         // attn_v_w
            ctx_size += n_text_layer*(             n_text_state*ggml_type_size(GGML_TYPE_F32)); // attn_v_b

            ctx_size += n_text_layer*(n_text_state*n_text_state*ggml_type_size(wtype));         // attn_ln_1_w
            ctx_size += n_text_layer*(             n_text_state*ggml_type_size(GGML_TYPE_F32)); // attn_ln_1_b
                                                                                                //
            ctx_size += n_text_layer*(n_text_state*ggml_type_size(GGML_TYPE_F32)); // cross_attn_ln_0_w
            ctx_size += n_text_layer*(n_text_state*ggml_type_size(GGML_TYPE_F32)); // cross_attn_ln_0_b

            ctx_size += n_text_layer*(n_text_state*n_text_state*ggml_type_size(wtype));         // cross_attn_q_w
            ctx_size += n_text_layer*(             n_text_state*ggml_type_size(GGML_TYPE_F32)); // cross_attn_q_b

            ctx_size += n_text_layer*(n_text_state*n_text_state*ggml_type_size(wtype)); // cross_attn_k_w

            ctx_size += n_text_layer*(n_text_state*n_text_state*ggml_type_size(wtype));         // cross_attn_v_w
            ctx_size += n_text_layer*(             n_text_state*ggml_type_size(GGML_TYPE_F32)); // cross_attn_v_b

            ctx_size += n_text_layer*(n_text_state*n_text_state*ggml_type_size(wtype));         // cross_attn_ln_1_w
            ctx_size += n_text_layer*(             n_text_state*ggml_type_size(GGML_TYPE_F32)); // cross_attn_ln_1_b
        }

        ctx_size += n_text_layer*n_text_ctx*n_text_state*ggml_type_size(GGML_TYPE_F16); // memory_k
        ctx_size += n_text_layer*n_text_ctx*n_text_state*ggml_type_size(GGML_TYPE_F16); // memory_v

        ctx_size += n_text_layer*n_audio_ctx*n_text_state*ggml_type_size(GGML_TYPE_F16); // memory_cross_k
        ctx_size += n_text_layer*n_audio_ctx*n_text_state*ggml_type_size(GGML_TYPE_F16); // memory_cross_v

        ctx_size += (15 + 15*n_audio_layer + 24*n_text_layer)*256; // object overhead

        printf("%s: ggml ctx size = %6.2f MB\n", __func__, ctx_size/(1024.0*1024.0));
    }

    // create the ggml context
    {
        struct ggml_init_params params = {
            .mem_size   = ctx_size,
            .mem_buffer = NULL,
        };

        model.ctx = ggml_init(params);
        if (!model.ctx) {
            fprintf(stderr, "%s: ggml_init() failed\n", __func__);
            return false;
        }
    }

    // prepare memory for the weights
    {
        const auto & hparams = model.hparams;

        const int n_vocab = hparams.n_vocab;

        const int n_audio_ctx   = hparams.n_audio_ctx;
        const int n_audio_state = hparams.n_audio_state;
        const int n_audio_layer = hparams.n_audio_layer;

        const int n_text_ctx = hparams.n_text_ctx;
        const int n_text_state = hparams.n_text_state;
        const int n_text_layer = hparams.n_text_layer;

        const int n_mels = hparams.n_mels;

        model.layers_encoder.resize(n_audio_layer);
        model.layers_decoder.resize(n_text_layer);

        // encoder
        {
            model.e_pe = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, n_audio_state, n_audio_ctx);

            model.e_conv_1_w = ggml_new_tensor_3d(ctx, wtype,         3, n_mels, n_audio_state);
            model.e_conv_1_b = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 1, n_audio_state);

            model.e_conv_2_w = ggml_new_tensor_3d(ctx, wtype,         3, n_audio_state, n_audio_state);
            model.e_conv_2_b = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, 1, n_audio_state);

            model.e_ln_w = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_audio_state);
            model.e_ln_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_audio_state);

            // map by name
            model.tensors["encoder.positional_embedding"] = model.e_pe;

            model.tensors["encoder.conv1.weight"] = model.e_conv_1_w;
            model.tensors["encoder.conv1.bias"]   = model.e_conv_1_b;

            model.tensors["encoder.conv2.weight"] = model.e_conv_2_w;
            model.tensors["encoder.conv2.bias"]   = model.e_conv_2_b;

            model.tensors["encoder.ln_post.weight"] = model.e_ln_w;
            model.tensors["encoder.ln_post.bias"]   = model.e_ln_b;

            for (int i = 0; i < n_audio_layer; ++i) {
                auto & layer = model.layers_encoder[i];

                layer.mlp_ln_w = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_audio_state);
                layer.mlp_ln_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_audio_state);

                layer.mlp_0_w = ggml_new_tensor_2d(ctx, wtype,           n_audio_state, 4*n_audio_state);
                layer.mlp_0_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 4*n_audio_state);

                layer.mlp_1_w = ggml_new_tensor_2d(ctx, wtype,         4*n_audio_state, n_audio_state);
                layer.mlp_1_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32,   n_audio_state);

                layer.attn_ln_0_w = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_audio_state);
                layer.attn_ln_0_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_audio_state);

                layer.attn_q_w = ggml_new_tensor_2d(ctx, wtype,         n_audio_state, n_audio_state);
                layer.attn_q_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_audio_state);

                layer.attn_k_w = ggml_new_tensor_2d(ctx, wtype,         n_audio_state, n_audio_state);

                layer.attn_v_w = ggml_new_tensor_2d(ctx, wtype,         n_audio_state, n_audio_state);
                layer.attn_v_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_audio_state);

                layer.attn_ln_1_w = ggml_new_tensor_2d(ctx, wtype,         n_audio_state, n_audio_state);
                layer.attn_ln_1_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_audio_state);

                // map by name
                model.tensors["encoder.blocks." + std::to_string(i) + ".mlp_ln.weight"] = layer.mlp_ln_w;
                model.tensors["encoder.blocks." + std::to_string(i) + ".mlp_ln.bias"]   = layer.mlp_ln_b;

                model.tensors["encoder.blocks." + std::to_string(i) + ".mlp.0.weight"] = layer.mlp_0_w;
                model.tensors["encoder.blocks." + std::to_string(i) + ".mlp.0.bias"]   = layer.mlp_0_b;

                model.tensors["encoder.blocks." + std::to_string(i) + ".mlp.2.weight"] = layer.mlp_1_w;
                model.tensors["encoder.blocks." + std::to_string(i) + ".mlp.2.bias"]   = layer.mlp_1_b;

                model.tensors["encoder.blocks." + std::to_string(i) + ".attn_ln.weight"] = layer.attn_ln_0_w;
                model.tensors["encoder.blocks." + std::to_string(i) + ".attn_ln.bias"]   = layer.attn_ln_0_b;

                model.tensors["encoder.blocks." + std::to_string(i) + ".attn.query.weight"] = layer.attn_q_w;
                model.tensors["encoder.blocks." + std::to_string(i) + ".attn.query.bias"]   = layer.attn_q_b;

                model.tensors["encoder.blocks." + std::to_string(i) + ".attn.key.weight"] = layer.attn_k_w;

                model.tensors["encoder.blocks." + std::to_string(i) + ".attn.value.weight"] = layer.attn_v_w;
                model.tensors["encoder.blocks." + std::to_string(i) + ".attn.value.bias"]   = layer.attn_v_b;

                model.tensors["encoder.blocks." + std::to_string(i) + ".attn.out.weight"] = layer.attn_ln_1_w;
                model.tensors["encoder.blocks." + std::to_string(i) + ".attn.out.bias"]   = layer.attn_ln_1_b;
            }
        }

        // decoder
        {
            model.d_pe = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, n_text_state, n_text_ctx);

            model.d_te = ggml_new_tensor_2d(ctx, wtype, n_text_state, n_vocab);

            model.d_ln_w = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);
            model.d_ln_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);

            // map by name
            model.tensors["decoder.positional_embedding"] = model.d_pe;

            model.tensors["decoder.token_embedding.weight"] = model.d_te;

            model.tensors["decoder.ln.weight"] = model.d_ln_w;
            model.tensors["decoder.ln.bias"]   = model.d_ln_b;

            for (int i = 0; i < n_text_layer; ++i) {
                auto & layer = model.layers_decoder[i];

                layer.mlp_ln_w = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);
                layer.mlp_ln_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);

                layer.mlp_0_w = ggml_new_tensor_2d(ctx, wtype,           n_text_state, 4*n_text_state);
                layer.mlp_0_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, 4*n_text_state);

                layer.mlp_1_w = ggml_new_tensor_2d(ctx, wtype,         4*n_text_state, n_text_state);
                layer.mlp_1_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32,   n_text_state);

                layer.attn_ln_0_w = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);
                layer.attn_ln_0_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);

                layer.attn_q_w = ggml_new_tensor_2d(ctx, wtype,         n_text_state, n_text_state);
                layer.attn_q_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);

                layer.attn_k_w = ggml_new_tensor_2d(ctx, wtype,         n_text_state, n_text_state);

                layer.attn_v_w = ggml_new_tensor_2d(ctx, wtype,         n_text_state, n_text_state);
                layer.attn_v_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);

                layer.attn_ln_1_w = ggml_new_tensor_2d(ctx, wtype,         n_text_state, n_text_state);
                layer.attn_ln_1_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);

                layer.cross_attn_ln_0_w = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);
                layer.cross_attn_ln_0_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);

                layer.cross_attn_q_w = ggml_new_tensor_2d(ctx, wtype,         n_text_state, n_text_state);
                layer.cross_attn_q_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);

                layer.cross_attn_k_w = ggml_new_tensor_2d(ctx, wtype,         n_text_state, n_text_state);

                layer.cross_attn_v_w = ggml_new_tensor_2d(ctx, wtype,         n_text_state, n_text_state);
                layer.cross_attn_v_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);

                layer.cross_attn_ln_1_w = ggml_new_tensor_2d(ctx, wtype,         n_text_state, n_text_state);
                layer.cross_attn_ln_1_b = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, n_text_state);

                // map by name
                model.tensors["decoder.blocks." + std::to_string(i) + ".mlp_ln.weight"] = layer.mlp_ln_w;
                model.tensors["decoder.blocks." + std::to_string(i) + ".mlp_ln.bias"]   = layer.mlp_ln_b;

                model.tensors["decoder.blocks." + std::to_string(i) + ".mlp.0.weight"] = layer.mlp_0_w;
                model.tensors["decoder.blocks." + std::to_string(i) + ".mlp.0.bias"]   = layer.mlp_0_b;

                model.tensors["decoder.blocks." + std::to_string(i) + ".mlp.2.weight"] = layer.mlp_1_w;
                model.tensors["decoder.blocks." + std::to_string(i) + ".mlp.2.bias"]   = layer.mlp_1_b;

                model.tensors["decoder.blocks." + std::to_string(i) + ".attn_ln.weight"] = layer.attn_ln_0_w;
                model.tensors["decoder.blocks." + std::to_string(i) + ".attn_ln.bias"]   = layer.attn_ln_0_b;

                model.tensors["decoder.blocks." + std::to_string(i) + ".attn.query.weight"] = layer.attn_q_w;
                model.tensors["decoder.blocks." + std::to_string(i) + ".attn.query.bias"]   = layer.attn_q_b;

                model.tensors["decoder.blocks." + std::to_string(i) + ".attn.key.weight"] = layer.attn_k_w;

                model.tensors["decoder.blocks." + std::to_string(i) + ".attn.value.weight"] = layer.attn_v_w;
                model.tensors["decoder.blocks." + std::to_string(i) + ".attn.value.bias"]   = layer.attn_v_b;

                model.tensors["decoder.blocks." + std::to_string(i) + ".attn.out.weight"] = layer.attn_ln_1_w;
                model.tensors["decoder.blocks." + std::to_string(i) + ".attn.out.bias"]   = layer.attn_ln_1_b;

                model.tensors["decoder.blocks." + std::to_string(i) + ".cross_attn_ln.weight"] = layer.cross_attn_ln_0_w;
                model.tensors["decoder.blocks." + std::to_string(i) + ".cross_attn_ln.bias"]   = layer.cross_attn_ln_0_b;

                model.tensors["decoder.blocks." + std::to_string(i) + ".cross_attn.query.weight"] = layer.cross_attn_q_w;
                model.tensors["decoder.blocks." + std::to_string(i) + ".cross_attn.query.bias"]   = layer.cross_attn_q_b;

                model.tensors["decoder.blocks." + std::to_string(i) + ".cross_attn.key.weight"] = layer.cross_attn_k_w;

                model.tensors["decoder.blocks." + std::to_string(i) + ".cross_attn.value.weight"] = layer.cross_attn_v_w;
                model.tensors["decoder.blocks." + std::to_string(i) + ".cross_attn.value.bias"]   = layer.cross_attn_v_b;

                model.tensors["decoder.blocks." + std::to_string(i) + ".cross_attn.out.weight"] = layer.cross_attn_ln_1_w;
                model.tensors["decoder.blocks." + std::to_string(i) + ".cross_attn.out.bias"]   = layer.cross_attn_ln_1_b;
            }
        }
    }

    // key + value memory
    {
        const auto & hparams = model.hparams;

        const int n_text_state = hparams.n_text_state;
        const int n_text_layer = hparams.n_text_layer;
        const int n_text_ctx   = hparams.n_text_ctx;

        // key/value memory for the self-attention layer
        {
            const int n_mem      = n_text_layer*n_text_ctx;
            const int n_elements = n_text_state*n_mem;

            model.memory_k = ggml_new_tensor_1d(ctx, GGML_TYPE_F16, n_elements);
            model.memory_v = ggml_new_tensor_1d(ctx, GGML_TYPE_F16, n_elements);
        }

        // key/value memory for the cross-attention layer
        {
            const int n_audio_ctx   = hparams.n_audio_ctx;

            const int n_mem      = n_text_layer*n_audio_ctx;
            const int n_elements = n_text_state*n_mem;

            model.memory_cross_k = ggml_new_tensor_1d(ctx, GGML_TYPE_F16, n_elements);
            model.memory_cross_v = ggml_new_tensor_1d(ctx, GGML_TYPE_F16, n_elements);
        }

        const size_t memory_size =
            ggml_nbytes(model.memory_k)       + ggml_nbytes(model.memory_v) +
            ggml_nbytes(model.memory_cross_k) + ggml_nbytes(model.memory_cross_v);

        printf("%s: memory size = %8.2f MB \n", __func__, memory_size/1024.0/1024.0);
    }

    // load weights
    {
        size_t total_size = 0;

        while (true) {
            int32_t n_dims;
            int32_t length;
            int32_t ftype;

            fin.read(reinterpret_cast<char *>(&n_dims), sizeof(n_dims));
            fin.read(reinterpret_cast<char *>(&length), sizeof(length));
            fin.read(reinterpret_cast<char *>(&ftype),  sizeof(ftype));

            if (fin.eof()) {
                break;
            }

            int32_t nelements = 1;
            int32_t ne[3] = { 1, 1, 1 };
            for (int i = 0; i < n_dims; ++i) {
                fin.read(reinterpret_cast<char *>(&ne[i]), sizeof(ne[i]));
                nelements *= ne[i];
            }

            std::string name(length, 0);
            fin.read(&name[0], length);

            if (model.tensors.find(name.data()) == model.tensors.end()) {
                fprintf(stderr, "%s: unknown tensor '%s' in model file\n", __func__, name.data());
                return false;
            }

            auto tensor = model.tensors[name.data()];
            if (ggml_nelements(tensor) != nelements) {
                fprintf(stderr, "%s: tensor '%s' has wrong size in model file\n", __func__, name.data());
                return false;
            }

            if (tensor->ne[0] != ne[0] || tensor->ne[1] != ne[1] || tensor->ne[2] != ne[2]) {
                fprintf(stderr, "%s: tensor '%s' has wrong shape in model file: got [%d, %d, %d], expected [%d, %d, %d]\n",
                        __func__, name.data(), tensor->ne[0], tensor->ne[1], tensor->ne[2], ne[0], ne[1], ne[2]);
                return false;
            }

            const size_t bpe = (ftype == 0) ? sizeof(float) : sizeof(ggml_fp16_t);

            if (nelements*bpe != ggml_nbytes(tensor)) {
                fprintf(stderr, "%s: tensor '%s' has wrong size in model file: got %zu, expected %zu\n",
                        __func__, name.data(), ggml_nbytes(tensor), nelements*bpe);
                return false;
            }

            fin.read(reinterpret_cast<char *>(tensor->data), ggml_nbytes(tensor));

            //printf("%24s - [%5d, %5d], type = %6s, %6.2f MB\n", name.data(), ne[0], ne[1], ftype == 0 ? "float" : "f16", ggml_nbytes(tensor)/1024.0/1024.0);
            total_size += ggml_nbytes(tensor);
        }

        printf("%s: model size  = %8.2f MB\n", __func__, total_size/1024.0/1024.0);
    }

    fin.close();

    return true;
}

// evaluate the encoder
//
// given audio recording (more specifically, its log mel spectrogram), runs forward pass of the encoder
// part of the transformer model and returns the encoded features
//
//   - model:      the model
//   - n_threads:  number of threads to use
//   - mel_offset: offset in the mel spectrogram (i.e. audio offset)
//   - mel_inp:    input mel spectrogram
//   - features:   output encoded features
//
bool whisper_encode(
        const whisper_model & model,
        const int n_threads,
        const int mel_offset,
        const whisper_mel & mel_inp,
              std::vector<float> & features) {
    const auto & hparams = model.hparams;

    const int n_vocab = hparams.n_vocab;

    const int n_ctx   = hparams.n_audio_ctx;
    const int n_state = hparams.n_audio_state;
    const int n_head  = hparams.n_audio_head;
    const int n_layer = hparams.n_audio_layer;

    const int N = n_ctx;

    const int n_mels = hparams.n_mels;
    assert(mel_inp.n_mel == n_mels);

    struct ggml_init_params params;

    {
        static size_t buf_size = MEM_REQ_ENCODE.at(model.type);
        static void * buf = malloc(buf_size);

        params = {
            .mem_size   = buf_size,
            .mem_buffer = buf,
        };
    }

    struct ggml_context * ctx0 = ggml_init(params);

    struct ggml_tensor * mel = ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, 2*n_ctx, n_mels);
    assert(mel->type == GGML_TYPE_F32);
    {
        float * dst = (float *) mel->data;
        memset(dst, 0, ggml_nbytes(mel));

        const int i0 = std::min(mel_offset, mel_inp.n_len);
        const int i1 = std::min(mel_offset + 2*n_ctx, mel_inp.n_len);

        for (int j = 0; j < mel_inp.n_mel; ++j) {
            for (int i = i0; i < i1; ++i) {
                dst[j*2*n_ctx + (i - i0)] = mel_inp.data[j*mel_inp.n_len + i];
            }
        }
    }

    struct ggml_tensor * cur;

    // convolution + gelu
    {
        cur = ggml_conv_1d_1s(ctx0, model.e_conv_1_w, mel);
        cur = ggml_add(ctx0,
                ggml_repeat(ctx0,
                    model.e_conv_1_b,
                    cur),
                cur);

        cur = ggml_gelu(ctx0, cur);

        cur = ggml_conv_1d_2s(ctx0, model.e_conv_2_w, cur);
        cur = ggml_add(ctx0,
                ggml_repeat(ctx0,
                    model.e_conv_2_b,
                    cur),
                cur);

        cur = ggml_gelu(ctx0, cur);
    }

    cur = ggml_add(ctx0, model.e_pe, ggml_transpose(ctx0, cur));

    struct ggml_tensor * inpL = cur;

    for (int il = 0; il < n_layer; ++il) {
        const auto & layer = model.layers_encoder[il];

        // create separate context for each layer to reduce memory usage

        struct ggml_init_params paramsL;
        {
            static size_t buf_size = MEM_REQ_ENCODE_LAYER.at(model.type);
            static void * buf = malloc(buf_size);

            paramsL = {
                .mem_size   = buf_size,
                .mem_buffer = buf,
            };
        }

        struct ggml_context * ctxL = ggml_init(paramsL);

        // norm
        {
            cur = ggml_norm(ctxL, inpL);

            // cur = ln_0_w*cur + ln_0_b
            cur = ggml_add(ctxL,
                    ggml_mul(ctxL,
                        ggml_repeat(ctxL, layer.attn_ln_0_w, cur),
                        cur),
                    ggml_repeat(ctxL, layer.attn_ln_0_b, cur));
        }

        // self-attention
        {
            struct ggml_tensor * Qcur = ggml_mul_mat(ctxL,
                    layer.attn_q_w,
                    cur);

            Qcur = ggml_add(ctxL,
                    ggml_repeat(ctxL,
                        layer.attn_q_b,
                        Qcur),
                    Qcur);

            //Qcur = ggml_scale(ctxL, Qcur, ggml_new_f32(ctxL, pow(float(n_state)/n_head, -0.25)));

            // note: no bias for Key
            struct ggml_tensor * Kcur = ggml_mul_mat(ctxL,
                    layer.attn_k_w,
                    cur);

            //Kcur = ggml_scale(ctxL, Kcur, ggml_new_f32(ctxL, pow(float(n_state)/n_head, -0.25)));

            struct ggml_tensor * Vcur = ggml_mul_mat(ctxL,
                    layer.attn_v_w,
                    cur);

            Vcur = ggml_add(ctxL,
                    ggml_repeat(ctxL,
                        layer.attn_v_b,
                        Vcur),
                    Vcur);

            // ------

#ifdef USE_FLASH_ATTN
            struct ggml_tensor * Q =
                ggml_permute(ctxL,
                        ggml_cpy(ctxL,
                            Qcur,
                            ggml_new_tensor_3d(ctxL, GGML_TYPE_F16, n_state/n_head, n_head, N)),
                        0, 2, 1, 3);

            struct ggml_tensor * K =
                ggml_permute(ctxL,
                        ggml_cpy(ctxL,
                            Kcur,
                            ggml_new_tensor_3d(ctxL, GGML_TYPE_F16, n_state/n_head, n_head, N)),
                        0, 2, 1, 3);

            struct ggml_tensor * V =
                ggml_cpy(ctxL,
                        ggml_permute(ctxL,
                            ggml_reshape_3d(ctxL,
                                Vcur,
                                n_state/n_head, n_head, N),
                            1, 2, 0, 3),
                        ggml_new_tensor_3d(ctxL, GGML_TYPE_F16, N, n_state/n_head, n_head)
                        );

            struct ggml_tensor * KQV = ggml_flash_attn(ctxL, Q, K, V, false);
#else
            struct ggml_tensor * Q =
                ggml_permute(ctxL,
                        ggml_cpy(ctxL,
                            Qcur,
                            ggml_new_tensor_3d(ctxL, GGML_TYPE_F32, n_state/n_head, n_head, N)),
                        0, 2, 1, 3);

            struct ggml_tensor * K =
                ggml_permute(ctxL,
                        ggml_cpy(ctxL,
                            Kcur,
                            ggml_new_tensor_3d(ctxL, GGML_TYPE_F16, n_state/n_head, n_head, N)),
                        0, 2, 1, 3);

            // K * Q
            struct ggml_tensor * KQ = ggml_mul_mat(ctxL, K, Q);

            struct ggml_tensor * KQ_scaled =
                ggml_scale(ctxL,
                        KQ,
                        ggml_new_f32(ctxL, 1.0f/sqrt(float(n_state)/n_head))
                        );

            struct ggml_tensor * KQ_soft_max = ggml_soft_max(ctxL, KQ_scaled);

            //struct ggml_tensor * V_trans =
            //    ggml_permute(ctxL,
            //            ggml_cpy(ctxL,
            //                Vcur,
            //                ggml_new_tensor_3d(ctxL, GGML_TYPE_F16, n_state/n_head, n_head, N)),
            //            1, 2, 0, 3);

            //struct ggml_tensor * KQV = ggml_mul_mat(ctxL, V_trans, KQ_soft_max);

            struct ggml_tensor * V =
                ggml_cpy(ctxL,
                        ggml_permute(ctxL,
                            ggml_reshape_3d(ctxL,
                                Vcur,
                                n_state/n_head, n_head, N),
                            0, 2, 1, 3),
                        ggml_new_tensor_3d(ctxL, GGML_TYPE_F16, n_state/n_head, N, n_head)
                        );

            struct ggml_tensor * KQV = ggml_mul_mat(ctxL, ggml_transpose(ctxL, V), KQ_soft_max);
#endif

            struct ggml_tensor * KQV_merged = ggml_permute(ctxL, KQV, 0, 2, 1, 3);

            cur = ggml_cpy(ctxL,
                    KQV_merged,
                    ggml_new_tensor_2d(ctxL, GGML_TYPE_F32, n_state, N));
        }

        // projection
        {
            cur = ggml_mul_mat(ctxL,
                    layer.attn_ln_1_w,
                    cur);

            cur = ggml_add(ctxL,
                    ggml_repeat(ctxL, layer.attn_ln_1_b, cur),
                    cur);
        }

        // add the input
        cur = ggml_add(ctxL, cur, inpL);

        struct ggml_tensor * inpFF = cur;

        // feed-forward network
        {
            // norm
            {
                cur = ggml_norm(ctxL, inpFF);

                // cur = mlp_ln_w*cur + mlp_ln_b
                cur = ggml_add(ctxL,
                        ggml_mul(ctxL,
                            ggml_repeat(ctxL, layer.mlp_ln_w, cur),
                            cur),
                        ggml_repeat(ctxL, layer.mlp_ln_b, cur));
            }

#ifdef USE_FLASH_FF
            cur = ggml_flash_ff(ctxL,
                    ggml_cpy(ctxL, cur, ggml_new_tensor_2d(ctxL, GGML_TYPE_F16, n_state, N)),
                    layer.mlp_0_w, layer.mlp_0_b, layer.mlp_1_w, layer.mlp_1_b);
#else
            // fully connected
            cur = ggml_mul_mat(ctxL,
                    layer.mlp_0_w,
                    cur);

            cur = ggml_add(ctxL,
                    ggml_repeat(ctxL, layer.mlp_0_b, cur),
                    cur);

            // GELU activation
            cur = ggml_gelu(ctxL, cur);

            // projection
            cur = ggml_mul_mat(ctxL,
                    layer.mlp_1_w,
                    cur);

            cur = ggml_add(ctxL,
                    ggml_repeat(ctxL, layer.mlp_1_b, cur),
                    cur);
#endif
        }

        // output from this layer
        struct ggml_tensor * inpO = ggml_add(ctxL, cur, inpFF);

        {
            struct ggml_cgraph gf = { .n_threads = n_threads };

            ggml_build_forward_expand(&gf, inpO);
            ggml_graph_compute       (ctxL, &gf);

            //ggml_graph_print(&gf);
        }

        // TODO: this is a hack to have per-layer computation graphs - need to come up with something better
        // input for next layer (inpO -> inpL)
        memcpy(inpL->data, inpO->data, ggml_nbytes(inpL));
        inpL->op = GGML_OP_NONE;
        inpL->src0 = NULL;
        inpL->src1 = NULL;

        //printf("%s: - used_mem(%d) = %f MB\n", __func__, il, ggml_used_mem(ctxL)/1024.0/1024.0);

        ggml_free(ctxL);
    }

    cur = inpL;

    // norm
    {
        cur = ggml_norm(ctx0, cur);

        // cur = ln_f_g*cur + ln_f_b
        cur = ggml_add(ctx0,
                ggml_mul(ctx0,
                    ggml_repeat(ctx0, model.e_ln_w, cur),
                    cur),
                ggml_repeat(ctx0, model.e_ln_b, cur));
    }

    // run the computation
    {
        struct ggml_cgraph gf = { .n_threads = n_threads };

        ggml_build_forward_expand(&gf, cur);
        ggml_graph_compute       (ctx0, &gf);

        //ggml_graph_print(&gf);
    }

    // cur
    //{
    //    printf("ne0 = %d\n", cur->ne[0]);
    //    printf("ne1 = %d\n", cur->ne[1]);
    //    for (int i = 0; i < 10; ++i) {
    //        printf("%8.4f ", ((float *)(cur->data))[i]);
    //    }
    //    printf("... ");
    //    for (int i = cur->ne[0] - 10; i < cur->ne[0]; ++i) {
    //        printf("%8.4f ", ((float *)(cur->data))[i]);
    //    }
    //    printf("\n");
    //}

    // pre-compute cross-attention memory
    {
        struct ggml_cgraph gf = { .n_threads = n_threads };

        // TODO: hack to disconnect the encoded features from the previous graph
        cur->op = GGML_OP_NONE;
        cur->src0 = NULL;
        cur->src1 = NULL;

        for (int il = 0; il < model.hparams.n_text_layer; ++il) {
            auto & layer = model.layers_decoder[il];

            struct ggml_tensor * Kcross = ggml_mul_mat(ctx0,
                    layer.cross_attn_k_w,
                    cur);

            Kcross = ggml_scale(ctx0, Kcross, ggml_new_f32(ctx0, pow(float(n_state)/n_head, -0.25)));

            struct ggml_tensor * Vcross = ggml_mul_mat(ctx0,
                    layer.cross_attn_v_w,
                    cur);

            Vcross = ggml_add(ctx0,
                    ggml_repeat(ctx0,
                        layer.cross_attn_v_b,
                        Vcross),
                    Vcross);

            struct ggml_tensor * k = ggml_view_1d(ctx0, model.memory_cross_k, n_state*n_ctx, (ggml_element_size(model.memory_cross_k)*n_state)*(il*n_ctx));
            struct ggml_tensor * v = ggml_view_1d(ctx0, model.memory_cross_v, n_state*n_ctx, (ggml_element_size(model.memory_cross_v)*n_state)*(il*n_ctx));

            ggml_build_forward_expand(&gf, ggml_cpy(ctx0, Kcross, k));
            ggml_build_forward_expand(&gf, ggml_cpy(ctx0, Vcross, v));
        }

        ggml_graph_compute(ctx0, &gf);
    }

    ////////////////////////////////////////////////////////////////////////////

    // output the features
    assert(cur->type == GGML_TYPE_F32);
    features.resize(cur->ne[0]*cur->ne[1]);
    memcpy(features.data(), cur->data, features.size()*sizeof(float));

    //printf("%s: used_mem = %f MB\n", __func__, ggml_used_mem(ctx0)/1024.0/1024.0);

    ggml_free(ctx0);

    return true;
}

// evaluate the decoder
//
// given text prompt + audio features -> predicts the probabilities for the next token
//
//   - model:      the model
//   - n_threads:  number of threads to use
//   - n_past:     prompt length
//   - prompt:     text prompt
//   - logits_out: output logits
//   - probs_out:  output probabilities
//
bool whisper_decode(
        const whisper_model & model,
        const int n_threads,
        const int n_past,
        const std::vector<whisper_vocab::id> & prompt,
              std::vector<float> & logits_out,
              std::vector<float> & probs_out) {
    const auto & hparams = model.hparams;

    const int n_vocab = hparams.n_vocab;

    const int n_ctx   = hparams.n_text_ctx;
    const int n_state = hparams.n_text_state;
    const int n_head  = hparams.n_text_head;
    const int n_layer = hparams.n_text_layer;

    const int N = prompt.size();
    const int M = hparams.n_audio_ctx;

    struct ggml_init_params params;

    {
        static size_t buf_size = MEM_REQ_DECODE.at(model.type);
        static void * buf = malloc(buf_size);

        params = {
            .mem_size   = buf_size,
            .mem_buffer = buf,
        };
    }

    struct ggml_context * ctx0 = ggml_init(params);

    struct ggml_tensor * embd = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, N);
    memcpy(embd->data, prompt.data(), N*ggml_element_size(embd));

    struct ggml_tensor * position = ggml_new_tensor_1d(ctx0, GGML_TYPE_I32, N);
    for (int i = 0; i < N; ++i) {
        ((int32_t *) position->data)[i] = n_past + i;
    }

    // token encoding + position encoding
    struct ggml_tensor * cur =
        ggml_add(ctx0,
                ggml_get_rows(ctx0, model.d_te, embd),
                ggml_get_rows(ctx0, model.d_pe, position));

    struct ggml_tensor * inpL = cur;

    for (int il = 0; il < n_layer; ++il) {
        const auto & layer = model.layers_decoder[il];

        struct ggml_init_params paramsL;

        {
            static size_t buf_size = MEM_REQ_DECODE_LAYER.at(model.type);
            static void * buf = malloc(buf_size);

            paramsL = {
                .mem_size   = buf_size,
                .mem_buffer = buf,
            };
        }

        struct ggml_context * ctxL = ggml_init(paramsL);
        struct ggml_cgraph gf = { .n_threads = n_threads };

        // norm
        {
            cur = ggml_norm(ctxL, inpL);

            // cur = ln_0_w*cur + ln_0_b
            cur = ggml_add(ctxL,
                    ggml_mul(ctxL,
                        ggml_repeat(ctxL, layer.attn_ln_0_w, cur),
                        cur),
                    ggml_repeat(ctxL, layer.attn_ln_0_b, cur));
        }

        // self-attention
        {
            struct ggml_tensor * Qcur = ggml_mul_mat(ctxL,
                    layer.attn_q_w,
                    cur);

            Qcur = ggml_add(ctxL,
                    ggml_repeat(ctxL,
                        layer.attn_q_b,
                        Qcur),
                    Qcur);

            Qcur = ggml_scale(ctxL, Qcur, ggml_new_f32(ctxL, pow(float(n_state)/n_head, -0.25)));

            // note: no bias for Key
            struct ggml_tensor * Kcur = ggml_mul_mat(ctxL,
                    layer.attn_k_w,
                    cur);

            Kcur = ggml_scale(ctxL, Kcur, ggml_new_f32(ctxL, pow(float(n_state)/n_head, -0.25)));

            struct ggml_tensor * Vcur = ggml_mul_mat(ctxL,
                    layer.attn_v_w,
                    cur);

            Vcur = ggml_add(ctxL,
                    ggml_repeat(ctxL,
                        layer.attn_v_b,
                        Vcur),
                    Vcur);

            // store key and value to memory
            {
                struct ggml_tensor * k = ggml_view_1d(ctxL, model.memory_k, N*n_state, (ggml_element_size(model.memory_k)*n_state)*(il*n_ctx + n_past));
                struct ggml_tensor * v = ggml_view_1d(ctxL, model.memory_v, N*n_state, (ggml_element_size(model.memory_v)*n_state)*(il*n_ctx + n_past));

                ggml_build_forward_expand(&gf, ggml_cpy(ctxL, Kcur, k));
                ggml_build_forward_expand(&gf, ggml_cpy(ctxL, Vcur, v));
            }

            // ------

            struct ggml_tensor * Q =
                ggml_permute(ctxL,
                        ggml_cpy(ctxL,
                            Qcur,
                            ggml_new_tensor_3d(ctxL, GGML_TYPE_F32, n_state/n_head, n_head, N)),
                        0, 2, 1, 3);

            struct ggml_tensor * K =
                ggml_permute(ctxL,
                        ggml_reshape_3d(ctxL,
                            ggml_view_1d(ctxL, model.memory_k, (n_past + N)*n_state, il*n_ctx*ggml_element_size(model.memory_k)*n_state),
                            n_state/n_head, n_head, n_past + N),
                        0, 2, 1, 3);

            // K * Q
            struct ggml_tensor * KQ = ggml_mul_mat(ctxL, K, Q);

            //struct ggml_tensor * KQ_scaled =
            //    ggml_scale(ctxL,
            //            KQ,
            //            ggml_new_f32(ctxL, 1.0f/sqrt(float(n_state)/n_head))
            //            );

            struct ggml_tensor * KQ_masked = ggml_diag_mask_inf(ctxL, KQ, n_past);

            struct ggml_tensor * KQ_soft_max = ggml_soft_max(ctxL, KQ_masked);

            struct ggml_tensor * V_trans =
                ggml_permute(ctxL,
                        ggml_reshape_3d(ctxL,
                            ggml_view_1d(ctxL, model.memory_v, (n_past + N)*n_state, il*n_ctx*ggml_element_size(model.memory_v)*n_state),
                            n_state/n_head, n_head, n_past + N),
                        1, 2, 0, 3);

            struct ggml_tensor * KQV = ggml_mul_mat(ctxL, V_trans, KQ_soft_max);

            struct ggml_tensor * KQV_merged = ggml_permute(ctxL, KQV, 0, 2, 1, 3);

            cur = ggml_cpy(ctxL,
                    KQV_merged,
                    ggml_new_tensor_2d(ctxL, GGML_TYPE_F32, n_state, N));
        }

        {
            cur = ggml_mul_mat(ctxL,
                    layer.attn_ln_1_w,
                    cur);

            cur = ggml_add(ctxL,
                    ggml_repeat(ctxL, layer.attn_ln_1_b, cur),
                    cur);
        }

        // add the input
        struct ggml_tensor * inpCA = ggml_add(ctxL, cur, inpL);

        // norm
        {
            cur = ggml_norm(ctxL, inpCA); // note: we use inpCA here

            // cur = ln_0_w*cur + ln_0_b
            cur = ggml_add(ctxL,
                    ggml_mul(ctxL,
                        ggml_repeat(ctxL, layer.cross_attn_ln_0_w, cur),
                        cur),
                    ggml_repeat(ctxL, layer.cross_attn_ln_0_b, cur));
        }

        // cross-attention
        {
            struct ggml_tensor * Qcur = ggml_mul_mat(ctxL,
                    layer.cross_attn_q_w,
                    cur);

            Qcur = ggml_add(ctxL,
                    ggml_repeat(ctxL,
                        layer.cross_attn_q_b,
                        Qcur),
                    Qcur);

            Qcur = ggml_scale(ctxL, Qcur, ggml_new_f32(ctxL, pow(float(n_state)/n_head, -0.25)));

            // Kcross is already scaled
            struct ggml_tensor * Kcross =
                ggml_reshape_3d(ctxL,
                        ggml_view_1d(ctxL, model.memory_cross_k, M*n_state, il*M*ggml_element_size(model.memory_cross_k)*n_state),
                        n_state/n_head, n_head, M);

            struct ggml_tensor * Vcross =
                ggml_reshape_3d(ctxL,
                        ggml_view_1d(ctxL, model.memory_cross_v, M*n_state, il*M*ggml_element_size(model.memory_cross_v)*n_state),
                        n_state/n_head, n_head, M);

            // ------

            struct ggml_tensor * Q =
                ggml_permute(ctxL,
                        ggml_cpy(ctxL,
                            Qcur,
                            ggml_new_tensor_3d(ctxL, GGML_TYPE_F32, n_state/n_head, n_head, N)),
                        0, 2, 1, 3);

            struct ggml_tensor * K = ggml_permute(ctxL, Kcross, 0, 2, 1, 3);

            // K * Q
            struct ggml_tensor * KQ = ggml_mul_mat(ctxL, K, Q);

            //struct ggml_tensor * KQ_scaled =
            //    ggml_scale(ctxL,
            //            KQ,
            //            ggml_new_f32(ctxL, 1.0f/sqrt(float(n_state)/n_head))
            //            );

            // no masking for cross-attention
            //struct ggml_tensor * KQ_masked = ggml_diag_mask_inf(ctxL, KQ_scaled, n_past);

            struct ggml_tensor * KQ_soft_max = ggml_soft_max(ctxL, KQ);

            struct ggml_tensor * V_trans = ggml_permute(ctxL, Vcross, 1, 2, 0, 3);

            struct ggml_tensor * KQV = ggml_mul_mat(ctxL, V_trans, KQ_soft_max);

            struct ggml_tensor * KQV_merged = ggml_permute(ctxL, KQV, 0, 2, 1, 3);

            // cur = KQV_merged.contiguous().view(n_state, N)
            cur = ggml_cpy(ctxL,
                    KQV_merged,
                    ggml_new_tensor_2d(ctxL, GGML_TYPE_F32, n_state, N));
        }

        // projection
        {
            cur = ggml_mul_mat(ctxL,
                    layer.cross_attn_ln_1_w,
                    cur);

            cur = ggml_add(ctxL,
                    ggml_repeat(ctxL, layer.cross_attn_ln_1_b, cur),
                    cur);
        }

        // add the input
        cur = ggml_add(ctxL, cur, inpCA);

        struct ggml_tensor * inpFF = cur;

        // feed-forward network
        {
            // norm
            {
                cur = ggml_norm(ctxL, inpFF);

                // cur = mlp_ln_w*cur + mlp_ln_b
                cur = ggml_add(ctxL,
                        ggml_mul(ctxL,
                            ggml_repeat(ctxL, layer.mlp_ln_w, cur),
                            cur),
                        ggml_repeat(ctxL, layer.mlp_ln_b, cur));
            }

            // fully connected
            cur = ggml_mul_mat(ctxL,
                    layer.mlp_0_w,
                    cur);

            cur = ggml_add(ctxL,
                    ggml_repeat(ctxL, layer.mlp_0_b, cur),
                    cur);

            // GELU activation
            cur = ggml_gelu(ctxL, cur);

            // projection
            cur = ggml_mul_mat(ctxL,
                    layer.mlp_1_w,
                    cur);

            cur = ggml_add(ctxL,
                    ggml_repeat(ctxL, layer.mlp_1_b, cur),
                    cur);
        }

        // output from this layer
        struct ggml_tensor * inpO = ggml_add(ctxL, cur, inpFF);

        {
            ggml_build_forward_expand(&gf, inpO);
            ggml_graph_compute       (ctxL, &gf);

            //ggml_graph_print(&gf);
        }

        // TODO: this is a hack to have per-layer computation graphs - need to come up with something better
        // input for next layer (inpO -> inpL)
        memcpy(inpL->data, inpO->data, ggml_nbytes(inpL));
        inpL->op = GGML_OP_NONE;
        inpL->src0 = NULL;
        inpL->src1 = NULL;

        if (N > 1) {
            //printf("%s: - used_mem(%d) = %f MB\n", __func__, il, ggml_used_mem(ctxL)/1024.0/1024.0);
        }

        ggml_free(ctxL);
    }

    cur = inpL;

    // norm
    {
        cur = ggml_norm(ctx0, cur);

        cur = ggml_add(ctx0,
                ggml_mul(ctx0,
                    ggml_repeat(ctx0, model.d_ln_w, cur),
                    cur),
                ggml_repeat(ctx0, model.d_ln_b, cur));
    }

    struct ggml_tensor * logits = ggml_mul_mat(ctx0, model.d_te, cur);

    // logits -> probs
    cur = ggml_dup(ctx0, logits);
    cur = ggml_soft_max(ctx0, cur); // in-place

    // run the computation
    {
        struct ggml_cgraph gf = { .n_threads = n_threads };

        ggml_build_forward_expand(&gf, cur);
        ggml_graph_compute       (ctx0, &gf);
    }

    logits_out.resize(N*n_vocab);
    memcpy(logits_out.data(), ggml_get_data(logits), sizeof(float)*N*n_vocab);

    probs_out.resize(N*n_vocab);
    memcpy(probs_out.data(), ggml_get_data(cur), sizeof(float)*N*n_vocab);

    if (N > 1) {
        //const float mem_per_token = ggml_used_mem(ctx0)/1024.0/1024.0/N;
        //printf("%s: used_mem = %f MB / %f per token\n", __func__, ggml_used_mem(ctx0)/1024.0/1024.0, mem_per_token);
        //printf("%s: max mem = %f MB\n", __func__, mem_per_token*model.hparams.n_text_ctx);
    }

    ggml_free(ctx0);

    return true;
}

// the most basic sampling scheme - select the top token
// TODO: beam search
// TODO: temperature
whisper_vocab::id whisper_sample_best(
        const whisper_vocab & vocab,
        const float * probs,
        double temp,
        int offset = 0) {
    int n_logits = vocab.id_to_token.size();

    std::vector<std::pair<double, whisper_vocab::id>> probs_id;
    probs_id.reserve(n_logits);

    for (int i = offset; i < n_logits; i++) {
        probs_id.push_back(std::make_pair(probs[i], i));
    }

    const int top_k = 10;

    // find the top K tokens
    std::partial_sort(
            probs_id.begin(),
            probs_id.begin() + top_k, probs_id.end(),
            [](const std::pair<double, whisper_vocab::id> & a, const std::pair<double, whisper_vocab::id> & b) {
        return a.first > b.first;
    });

    probs_id.resize(top_k);

    //printf("\n");
    //for (int i = 0; i < (int) probs_id.size(); i++) {
    //    printf("%d: '%s' %f, %d\n", i, vocab.id_to_token.at(probs_id[i].second).c_str(), probs_id[i].first, probs_id[i].second);
    //}

    int res = 0;
    while (probs_id[res].second == vocab.token_solm && res < (int) probs_id.size() - 1) {
        res++;
    }

    return probs_id[res].second;
}

// Cooley-Tukey FFT
// poor man's implmentation - use something better
// input is real-valued
// output is complex-valued
void fft(const std::vector<float> & in, std::vector<float> & out) {
    out.resize(in.size()*2);

    int N = in.size();

    if (N == 1) {
        out[0] = in[0];
        out[1] = 0;
        return;
    }

    std::vector<float> even;
    std::vector<float> odd;

    for (int i = 0; i < N; i++) {
        if (i % 2 == 0) {
            even.push_back(in[i]);
        } else {
            odd.push_back(in[i]);
        }
    }

    std::vector<float> even_fft;
    std::vector<float> odd_fft;

    fft(even, even_fft);
    fft(odd, odd_fft);

    for (int k = 0; k < N/2; k++) {
        float theta = 2*M_PI*k/N;

        float re = cos(theta);
        float im = -sin(theta);

        float re_odd = odd_fft[2*k + 0];
        float im_odd = odd_fft[2*k + 1];

        out[2*k + 0] = even_fft[2*k + 0] + re*re_odd - im*im_odd;
        out[2*k + 1] = even_fft[2*k + 1] + re*im_odd + im*re_odd;

        out[2*(k + N/2) + 0] = even_fft[2*k + 0] - re*re_odd + im*im_odd;
        out[2*(k + N/2) + 1] = even_fft[2*k + 1] - re*im_odd - im*re_odd;
    }
}

// ref: https://github.com/openai/whisper/blob/main/whisper/audio.py#L92-L124
bool log_mel_spectrogram(
    const std::vector<float> sf32,
    const int sample_rate,
    const int fft_size,
    const int fft_step,
    const int n_mel,
    const int n_threads,
    const whisper_filters & filters,
    whisper_mel & mel) {
    const int n_sample = sf32.size();
    const float * samples = sf32.data();

    // Hanning window
    std::vector<float> hann;
    hann.resize(fft_size);
    for (int i = 0; i < fft_size; i++) {
        hann[i] = 0.5*(1.0 - cos((2.0*M_PI*i)/(fft_size)));
    }

    mel.n_mel = n_mel;
    mel.n_len = (n_sample)/fft_step;
    mel.data.resize(mel.n_mel*mel.n_len);

    const int n_fft = 1 + fft_size/2;

    printf("%s: n_sample = %d, n_len = %d\n", __func__, n_sample, mel.n_len);
    printf("%s: recording length: %f s\n", __func__, (float) n_sample/sample_rate);

    std::vector<std::thread> workers(n_threads);
    for (int iw = 0; iw < n_threads; ++iw) {
        workers[iw] = std::thread([&](int ith) {
            std::vector<float> fft_in;
            fft_in.resize(fft_size);
            for (int i = 0; i < fft_size; i++) {
                fft_in[i] = 0.0;
            }

            std::vector<float> fft_out;
            fft_out.resize(2*fft_size);

            for (int i = ith; i < mel.n_len; i += n_threads) {
                const int offset = i*fft_step;

                // apply Hanning window
                for (int j = 0; j < fft_size; j++) {
                    if (offset + j < n_sample) {
                        fft_in[j] = hann[j]*samples[offset + j];
                    } else {
                        fft_in[j] = 0.0;
                    }
                }

                // FFT -> mag^2
                fft(fft_in, fft_out);

                for (int j = 0; j < n_fft; j++) {
                    fft_out[j] = (fft_out[2*j + 0]*fft_out[2*j + 0] + fft_out[2*j + 1]*fft_out[2*j + 1]);
                }

                // mel spectrogram
                for (int j = 0; j < mel.n_mel; j++) {
                    double sum = 0.0;

                    for (int k = 0; k < n_fft; k++) {
                        sum += fft_out[k]*filters.data[j*n_fft + k];
                    }
                    if (sum < 1e-10) {
                        sum = 1e-10;
                    }

                    sum = log10(sum);

                    mel.data[j*mel.n_len + i] = sum;
                }
            }
        }, iw);
    }

    for (int iw = 0; iw < n_threads; ++iw) {
        workers[iw].join();
    }

    // clamping and normalization
    double mmax = -1e20;
    for (int i = 0; i < mel.n_mel*mel.n_len; i++) {
        if (mel.data[i] > mmax) {
            mmax = mel.data[i];
        }
    }

    mmax -= 8.0;

    for (int i = 0; i < mel.n_mel*mel.n_len; i++) {
        if (mel.data[i] < mmax) {
            mel.data[i] = mmax;
        }

        mel.data[i] = (mel.data[i] + 4.0)/4.0;
    }

    return true;
}

int main(int argc, char ** argv) {
    const int64_t t_main_start_us = ggml_time_us();

    whisper_params params;

    if (whisper_params_parse(argc, argv, params) == false) {
        return 1;
    }

    if (params.seed < 0) {
        params.seed = time(NULL);
    }

    // Model loading

    //printf("%s: seed = %d\n", __func__, params.seed);

    int64_t t_load_us   = 0;
    int64_t t_mel_us    = 0;
    int64_t t_sample_us  = 0;
    int64_t t_encode_us = 0;
    int64_t t_decode_us = 0;

    whisper_vocab vocab;
    whisper_model model;

    // load the model
    {
        const int64_t t_start_us = ggml_time_us();

        if (!whisper_model_load(params.model, model, vocab)) {
            fprintf(stderr, "%s: failed to load model from '%s'\n", __func__, params.model.c_str());
            whisper_print_usage(argc, argv, {});
            return 1;
        }

        t_load_us = ggml_time_us() - t_start_us;
    }

    // WAV input
    std::vector<float> pcmf32;
    {
        drwav wav;
        if (!drwav_init_file(&wav, params.fname_inp.c_str(), NULL)) {
            fprintf(stderr, "%s: failed to open WAV file '%s' - check your input\n", argv[0], params.fname_inp.c_str());
            whisper_print_usage(argc, argv, {});
            return 2;
        }

        if (wav.channels != 1) {
            fprintf(stderr, "%s: WAV file '%s' must be mono\n", argv[0], params.fname_inp.c_str());
            return 3;
        }

        if (wav.sampleRate != SAMPLE_RATE) {
            fprintf(stderr, "%s: WAV file '%s' must be 16 kHz\n", argv[0], params.fname_inp.c_str());
            return 4;
        }

        if (wav.bitsPerSample != 16) {
            fprintf(stderr, "%s: WAV file '%s' must be 16-bit\n", argv[0], params.fname_inp.c_str());
            return 5;
        }

        std::vector<int16_t> pcm16;
        pcm16.resize(wav.totalPCMFrameCount);
        drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, pcm16.data());
        drwav_uninit(&wav);

        // convert to float
        pcmf32.resize(pcm16.size());
        for (size_t i = 0; i < pcm16.size(); i++) {
            pcmf32[i] = float(pcm16[i])/32768.0f;
        }
    }

    // compute log mel spectrogram
    whisper_mel mel_inp;
    {
        const int64_t t_start_us = ggml_time_us();

        log_mel_spectrogram(pcmf32, SAMPLE_RATE, N_FFT, HOP_LENGTH, N_MEL, params.n_threads, model.filters, mel_inp);

        t_mel_us = ggml_time_us() - t_start_us;
    }

    // print some info about the processing
    {
        printf("\n");
        if (!vocab.is_multilingual()) {
            if (params.language != "en" || params.translate) {
                params.language = "en";
                params.translate = false;
                printf("%s: WARNING: model is not multilingual, ignoring language and translation options\n", __func__);
            }
        }
        printf("%s: processing %d samples (%.1f sec), %d threads, lang = %s, task = %s ...\n",
                __func__, int(pcmf32.size()), float(pcmf32.size())/SAMPLE_RATE, params.n_threads,
                g_lang.at(params.language).second.c_str(),
                params.translate ? "translate" : "transcribe");
    }

    // the accumulated text context so far
    std::vector<whisper_vocab::id> prompt_past = { };

    // these tokens determine the task that will be performed
    std::vector<whisper_vocab::id> prompt_init = { vocab.token_sot };
    if (vocab.is_multilingual()) {
        prompt_init.push_back(vocab.token_sot + 1 + g_lang.at(params.language).first);
        if (params.translate) {
            prompt_init.push_back(vocab.token_translate);
        } else {
            prompt_init.push_back(vocab.token_transcribe);
        }
    }

    // main loop
    int seek = 0;
    while (true) {
        if (seek >= mel_inp.n_len) {
            break;
        }

        // encode audio features starting at offset seek
        std::vector<float> features;
        {
            const int64_t t_start_us = ggml_time_us();

            if (!whisper_encode(model, params.n_threads, seek, mel_inp, features)) {
                fprintf(stderr, "%s: failed to eval\n", __func__);
                return 1;
            }

            t_encode_us = ggml_time_us() - t_start_us;
        }

        std::vector<float> probs;
        std::vector<float> logits;

        std::vector<whisper_vocab::id> prompt;

        int n_past = 0;

        // if we have already generated some text, use it as a prompt to condition the next generation
        if (prompt_past.size() > 0) {
            int n_take = std::min(model.hparams.n_text_ctx/2, int(prompt_past.size()));

            prompt = { vocab.token_prev };
            prompt.insert(prompt.begin() + 1, prompt_past.end() - n_take, prompt_past.end());

            prompt_past.clear();
            prompt_past.insert(prompt_past.end(), prompt.begin() + 1, prompt.end());
        }

        prompt.insert(prompt.end(), prompt_init.begin(), prompt_init.end());

        bool done = false;
        int seek_delta = 100*CHUNK_SIZE;
        whisper_vocab::id last_id = 0;

        //for (int i = 0; i < prompt.size(); i++) {
        //    printf("%s: prompt[%d] = %s\n", __func__, i, vocab.id_to_token[prompt[i]].c_str());
        //}

        printf("\n");
        for (int i = 0; i < model.hparams.n_text_ctx/2; ++i) {
            // decode
            if (prompt.size() > 0) {
                const int64_t t_start_us = ggml_time_us();

                if (!whisper_decode(model, params.n_threads, n_past, prompt, logits, probs)) {
                    fprintf(stderr, "%s: failed to eval\n", __func__);
                    return 1;
                }

                t_decode_us += ggml_time_us() - t_start_us;
            }

            n_past += prompt.size();
            prompt.clear();

            // very basic greedy sampling strategy:
            //
            //   - always take the most probable token
            //   - if we have accumulated more than 'params.max_tokens_per_iter' -> pick most probable timestamp token
            //     and advance the sliding window by that amount
            //   - in the meantime, if we encounter 2 consecutive timestamp tokens, we advance the sliding window too
            //
            // more sophisticated sampling strategies could be implemented here, but we keep it simple
            // feel free to experiment!
            //
            {
                // sample next token
                const float temp  = 1.0; // TODO

                const int n_vocab = model.hparams.n_vocab;

                whisper_vocab::id id = 0;

                {
                    const int64_t t_start_sample_us = ggml_time_us();

                    id = whisper_sample_best(vocab, probs.data() + (probs.size() - n_vocab), temp, i > params.max_tokens_per_iter ? vocab.token_beg : 0);

                    t_sample_us += ggml_time_us() - t_start_sample_us;
                }

                // end of text token
                if (id == vocab.token_eot) {
                    break;
                }

                // 2 consecutive time tokens
                if (id > vocab.token_beg && last_id > vocab.token_beg) {
                    seek_delta = 2*(id - vocab.token_beg);
                    done = true;
                }
                last_id = id;

                // add it to the context
                prompt.push_back(id);
                prompt_past.push_back(id);
            }

            // display text
            for (auto id : prompt) {
                if (params.print_special_tokens == false && id >= vocab.token_eot) {
                    continue;
                }
                printf("%s", vocab.id_to_token[id].c_str());
            }
            fflush(stdout);

            if (done) {
                break;
            }
        }

        seek += seek_delta;
    }

    // report timing
    {
        const int64_t t_main_end_us = ggml_time_us();

        printf("\n\n");
        printf("%s:     load time = %8.2f ms\n", __func__, t_load_us/1000.0f);
        printf("%s:      mel time = %8.2f ms\n", __func__, t_mel_us/1000.0f);
        printf("%s:   sample time = %8.2f ms\n", __func__, t_sample_us/1000.0f);
        printf("%s:   encode time = %8.2f ms / %.2f ms per layer\n", __func__, t_encode_us/1000.0f, t_encode_us/1000.0f/model.hparams.n_audio_layer);
        printf("%s:   decode time = %8.2f ms\n", __func__, t_decode_us/1000.0f);
        printf("%s:    total time = %8.2f ms\n", __func__, (t_main_end_us - t_main_start_us)/1000.0f);
    }

    ggml_free(model.ctx);

    return 0;
}
