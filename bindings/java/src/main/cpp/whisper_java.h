#define WHISPER_BUILD
#include <whisper.h>

#ifdef __cplusplus
extern "C" {
#endif

struct whisper_java_params {
};

WHISPER_API void whisper_java_default_params(enum whisper_sampling_strategy strategy);

WHISPER_API void whisper_java_init_from_file(const char * path_model);

WHISPER_API int whisper_java_full(
          struct whisper_context * ctx,
//      struct whisper_java_params   params,
                     const float * samples,
                             int   n_samples);


#ifdef __cplusplus
}
#endif
