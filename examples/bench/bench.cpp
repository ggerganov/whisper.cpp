#include "ggml.h"
#include "whisper.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

// command-line parameters
struct whisper_params {
    int32_t n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t what = 0; // what to benchmark: 0 - whisper ecoder, 1 - memcpy, 2 - ggml_mul_mat

    std::string model = "models/ggml-base.en.bin";
};

void whisper_print_usage(int argc, char ** argv, const whisper_params & params);

bool whisper_params_parse(int argc, char ** argv, whisper_params & params) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
        else if (arg == "-t" || arg == "--threads") { params.n_threads = std::stoi(argv[++i]); }
        else if (arg == "-m" || arg == "--model")   { params.model     = argv[++i]; }
        else if (arg == "-w" || arg == "--what")    { params.what     = atoi(argv[++i]); }
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
    fprintf(stderr, "  -h,       --help        [default] show this help message and exit\n");
    fprintf(stderr, "  -t N,     --threads N   [%-7d] number of threads to use during computation\n", params.n_threads);
    fprintf(stderr, "  -m FNAME, --model FNAME [%-7s] model path\n",                                  params.model.c_str());
    fprintf(stderr, "  -w N,     --what N      [%-7d] what to benchmark:\n",                          params.what);
    fprintf(stderr, "                           %-7s  0 - whisper encoder\n",                         "");
    fprintf(stderr, "                           %-7s  1 - memcpy\n",                                  "");
    fprintf(stderr, "                           %-7s  2 - ggml_mul_mat\n",                            "");
    fprintf(stderr, "\n");
}

int bench_whisper_encoder(const whisper_params & params) {
    // whisper init

    struct whisper_context * ctx = whisper_init_from_file(params.model.c_str());

    {
        fprintf(stderr, "\n");
        fprintf(stderr, "system_info: n_threads = %d / %d | %s\n", params.n_threads, std::thread::hardware_concurrency(), whisper_print_system_info());
    }

    if (ctx == nullptr) {
        fprintf(stderr, "error: failed to initialize whisper context\n");
        return 2;
    }

    if (int ret = whisper_set_mel(ctx, nullptr, 0, WHISPER_N_MEL)) {
        fprintf(stderr, "error: failed to set mel: %d\n", ret);
        return 3;
    }

    if (int ret = whisper_encode(ctx, 0, params.n_threads) != 0) {
        fprintf(stderr, "error: failed to encode model: %d\n", ret);
        return 4;
    }

    whisper_print_timings(ctx);
    whisper_free(ctx);

    fprintf(stderr, "\n");
    fprintf(stderr, "If you wish, you can submit these results here:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  https://github.com/ggerganov/whisper.cpp/issues/89\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Please include the following information:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  - CPU model\n");
    fprintf(stderr, "  - Operating system\n");
    fprintf(stderr, "  - Compiler\n");
    fprintf(stderr, "\n");

    return 0;
}

int bench_memcpy(const whisper_params & params) {
    size_t n    = 50;
    size_t arr  = params.what > 0 ? 1024 : params.what; // trick to avoid compiler optimizations

    // 1 GB array
    const size_t size = arr*1024llu*1024llu;

    char * src = (char *) malloc(size);
    char * dst = (char *) malloc(size);

    for (size_t i = 0; i < size; i++) src[i] = i;

    memcpy(dst, src, size); // heat-up

    double tsum = 0.0;

    for (size_t i = 0; i < n; i++) {
        const int64_t t0 = ggml_time_us();

        memcpy(dst, src, size);

        const int64_t t1 = ggml_time_us();

        tsum += (t1 - t0)*1e-6;

        src[0] = rand();
    }

    fprintf(stderr, "memcpy: %.2f GB/s\n", (double) (n*size)/(tsum*1024llu*1024llu*1024llu));

    // needed to prevent the compile from optimizing the memcpy away
    {
        double sum = 0.0;

        for (size_t i = 0; i < size; i++) sum += dst[i];

        fprintf(stderr, "sum:    %s\n", sum == -536870910.00 ? "ok" : "error");
    }

    free(src);
    free(dst);

    return 0;
}

int bench_ggml_mul_mat(const whisper_params & params) {
    const int n_max = 128;

    const std::vector<size_t> sizes = {
        64, 128, 256, 512, 1024, 2048, 4096,
    };

    const size_t N_max = sizes.back();

    // a: N*N*sizeof(float)
    // b: N*N*sizeof(float)
    // c: N*N*sizeof(float)
    // when F16 is used, there is an extra work buffer of size N*N*sizeof(float)
    std::vector<char> buf(4llu*N_max*N_max*sizeof(float) + 4*256);

    for (size_t i = 0; i < buf.size(); i++) buf[i] = i;

    for (int j = 0; j < (int) sizes.size(); j++) {
        int n_fp16 = 0;
        int n_fp32 = 0;

        // GFLOPS/s
        double s_fp16 = 0.0;
        double s_fp32 = 0.0;

        const size_t N = sizes[j];

        for (int k = 0; k < 2; ++k) {
            const ggml_type wtype = k == 0 ? GGML_TYPE_F16 : GGML_TYPE_F32;

            double & s = k == 0 ? s_fp16 : s_fp32;
            int    & n = k == 0 ? n_fp16   : n_fp32;

            struct ggml_init_params gparams = {
                /*.mem_size   =*/ buf.size(),
                /*.mem_buffer =*/ buf.data(),
            };

            struct ggml_context * ctx0 = ggml_init(gparams);

            struct ggml_tensor * a = ggml_new_tensor_2d(ctx0, wtype,         N, N);
            struct ggml_tensor * b = ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, N, N);

            struct ggml_tensor * c = ggml_mul_mat(ctx0, a, b);

            struct ggml_cgraph gf = ggml_build_forward(c);

            gf.n_threads = params.n_threads;

            double tsum = 0.0;

            // heat-up
            ggml_graph_compute(ctx0, &gf);

            for (int i = 0; i < n_max; ++i) {
                const int64_t t0 = ggml_time_us();

                ggml_graph_compute(ctx0, &gf);

                const int64_t t1 = ggml_time_us();

                tsum += (t1 - t0)*1e-6;
                n++;

                if (tsum > 1.0 && n >= 3) {
                    break;
                }
            }

            ggml_free(ctx0);

            s = ((2.0*N*N*N*n)/tsum)*1e-9;
        }

        fprintf(stderr, "ggml_mul_mat: %5zu x %5zu: F16 %8.1f GFLOPS (%3d runs) / F32 %8.1f GFLOPS (%3d runs)\n",
            N, N, s_fp16, n_fp16, s_fp32, n_fp32);
    }

    return 0;
}

int main(int argc, char ** argv) {
    whisper_params params;

    if (whisper_params_parse(argc, argv, params) == false) {
        return 1;
    }

    ggml_time_init();

    int ret = -1;

    switch (params.what) {
        case 0: ret = bench_whisper_encoder(params); break;
        case 1: ret = bench_memcpy(params);          break;
        case 2: ret = bench_ggml_mul_mat(params);    break;
        default: fprintf(stderr, "error: unknown benchmark: %d\n", params.what); break;
    }

    return ret;
}
