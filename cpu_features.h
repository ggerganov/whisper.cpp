#ifndef __CPU_FEATURES_H__
#define __CPU_FEATURES_H__
#ifdef __x86_64__

#include <stdbool.h>

extern bool cpu_has_sse3;
extern bool cpu_has_ssse3;
extern bool cpu_has_f16c;
extern bool cpu_has_fma;
extern bool cpu_has_avx;
extern bool cpu_has_avx2;
extern bool cpu_has_avx512f;
extern bool cpu_has_avx512vbmi;
extern bool cpu_has_avx512vnni;
extern bool cpu_has_avx512vnniint8;
extern bool cpu_has_avx512bf16;

void check_cpu_features(void);

bool check_cpu_sse3(void);
bool check_cpu_ssse3(void);
bool check_cpu_f16c(void);
bool check_cpu_fma(void);
bool check_cpu_avx(void);
bool check_cpu_avx2(void);
bool check_cpu_avx512f(void);
bool check_cpu_avx512vbmi(void);
bool check_cpu_avx512vnni(void);
bool check_cpu_avx512vnniint8(void);
bool check_cpu_avx512bf16(void);

#endif
#endif
