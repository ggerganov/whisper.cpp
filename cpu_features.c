#ifdef __x86_64__
#include <stdint.h>
#include <stdbool.h>
#include <cpuid.h>

#ifndef bit_AVX512VNNI
#define bit_AVX512VNNI        0x800
#endif
#ifndef bit_AVXVNNIINT8
#define bit_AVXVNNIINT8       0x10
#endif

bool cpu_has_sse3 = false;
bool cpu_has_ssse3 = false;
bool cpu_has_f16c = false;
bool cpu_has_fma = false;
bool cpu_has_avx = false;
bool cpu_has_avx2 = false;
bool cpu_has_avx512f = false;
bool cpu_has_avx512vbmi = false;
bool cpu_has_avx512vnni = false;
bool cpu_has_avx512vnniint8 = false;
bool cpu_has_avx512bf16 = false;

#ifdef XSAVE_SUPPORT

#define XCR0_SSE_STATE        0x2    // SSE 128-bit state (XMM bit)
#define XCR0_AVX_STATE        0x4    // AVX 256-bit state (YMM bit)
#define XCR0_AVX_MASK         (XCR0_SSE_STATE | XCR0_AVX_STATE)
#define XCR0_AVX512_STATE     0xE0   // AVX-512 state (combines ZMM hi256, Hi16 ZMM, PKRU)

unsigned long long get_xcr0() {
    unsigned int eax, edx;
    // use xgetbv to get XCR0
    __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
    return ((unsigned long long)edx << 32) | eax;
}
#endif

bool _cpuid(unsigned int level, unsigned int count, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
    __cpuid_count(1, 0, *eax, *ebx, *ecx, *edx);
    if (*eax < level)
        return false;
    __cpuid_count(level, count, *eax, *ebx, *ecx, *edx);
    return true;
}

bool check_cpu_sse3(void) {
    unsigned int eax, ebx, ecx, edx;
    return _cpuid(1, 0, &eax, &ebx, &ecx, &edx) && (ecx & bit_SSE3);
}

bool check_cpu_ssse3(void) {
    unsigned int eax, ebx, ecx, edx;
    return _cpuid(1, 0, &eax, &ebx, &ecx, &edx) && (ecx & bit_SSSE3);
}

bool check_cpu_f16c(void) {
    unsigned eax, ebx, ecx, edx;
    return _cpuid(1, 0, &eax, &ebx, &ecx, &edx) && (ecx & bit_F16C);
}

bool check_cpu_fma(void) {
    unsigned eax, ebx, ecx, edx;
    return _cpuid(1, 0, &eax, &ebx, &ecx, &edx) && (ecx & bit_FMA);
}

bool check_cpu_avx(void) {
    unsigned int eax, ebx, ecx, edx;
    return _cpuid(1, 0, &eax, &ebx, &ecx, &edx) && (ecx & bit_AVX)
#ifdef XSAVE_SUPPORT
        && (ecx & bit_OSXSAVE) && ((get_xcr0() & XCR0_AVX_MASK) == XCR0_AVX_MASK)
#endif
        ;
}

bool check_cpu_avx2(void) {
    unsigned int eax, ebx, ecx, edx;
    return check_cpu_avx() && _cpuid(7, 0, &eax, &ebx, &ecx, &edx) && (ebx & bit_AVX2);
}

bool check_cpu_avx512f(void) {
    unsigned eax, ebx, ecx, edx;
    return check_cpu_avx2() && _cpuid(7, 0, &eax, &ebx, &ecx, &edx) && (ebx & bit_AVX512F)
#ifdef XSAVE_SUPPORT
        && (get_xcr0() & XCR0_AVX512_STATE) == XCR0_AVX512_STATE
#endif
        ;
}

bool check_cpu_avx512vbmi(void) {
    unsigned eax, ebx, ecx, edx;
    return check_cpu_avx512f() && _cpuid(7, 0, &eax, &ebx, &ecx, &edx) && (ecx & bit_AVX512VBMI);
}

bool check_cpu_avx512vnni(void) {
    unsigned eax, ebx, ecx, edx;
    return check_cpu_avx512f() && _cpuid(7, 0, &eax, &ebx, &ecx, &edx) && (ecx & bit_AVX512VNNI);
}

bool check_cpu_avx512vnniint8(void) {
    unsigned eax, ebx, ecx, edx;
    return check_cpu_avx512f() && _cpuid(7, 1, &eax, &ebx, &ecx, &edx) && (edx & bit_AVXVNNIINT8);
}

bool check_cpu_avx512bf16(void) {
    unsigned eax, ebx, ecx, edx;
    return check_cpu_avx512f() && _cpuid(7, 1, &eax, &ebx, &ecx, &edx) && (ebx & (1 << 5)) != 0; // EBX, bit 5
}

void check_cpu_features(void) {
    cpu_has_sse3 = check_cpu_sse3();
    cpu_has_ssse3 = check_cpu_ssse3();
    cpu_has_f16c = check_cpu_f16c();
    cpu_has_fma = check_cpu_fma();
    cpu_has_avx = check_cpu_avx();
    cpu_has_avx2 = check_cpu_avx2();
    cpu_has_avx512f = check_cpu_avx512f();
    cpu_has_avx512vbmi = check_cpu_avx512vbmi();
    cpu_has_avx512vnni = check_cpu_avx512vnni();
    cpu_has_avx512vnniint8 = check_cpu_avx512vnniint8();
    cpu_has_avx512bf16 = check_cpu_avx512bf16();
}
#else
#pragma GCC diagnostic ignored "-Wempty-translation-unit"
#endif
