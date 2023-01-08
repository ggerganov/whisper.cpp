// ggml SIMD/AVX test
// based on https://gist.github.com/belltailjp/4653695
// belltailjp/simd.cpp
//
// https://github.com/ggerganov/whisper.cpp
//
// gcc -O3 -std=c++11 -mavx -mf16c -o test-ggml test-ggml.cpp -lstdc++ -lm
// gcc -O3 -std=c++11 -mavx -mf16c -mavx2 -mfma -o test-ggml test-ggml.cpp -lstdc++ -lm
// gcc -Ofast -std=c++11 -mavx -mf16c -o test-ggml test-ggml.cpp -lstdc++ -lm
// gcc -Ofast -std=c++11 -mavx -mf16c -mavx2 -mfma -o test-ggml test-ggml.cpp -lstdc++ -lm

#include <random>
#include <algorithm>
#include <chrono>
#include <stdio.h>
#include <string.h>
#include <float.h>

#include <xmmintrin.h>
#include <immintrin.h>

#ifdef  __cplusplus
extern "C" {
#endif

// from ggml.h
typedef uint16_t ggml_fp16_t;

// from ggml.c
#define GGML_ASSERT(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "GGML_ASSERT: %s:%d: %s\n", __FILE__, __LINE__, #x); \
            abort(); \
        } \
    } while (0)

typedef double ggml_float;


// FP16 <-> FP32
// ref: https://github.com/Maratyszcza/FP16

static inline float fp32_from_bits(uint32_t w) {
    union {
        uint32_t as_bits;
        float as_value;
    } fp32 = { w };
    return fp32.as_value;
}

static inline uint32_t fp32_to_bits(float f) {
	union {
		float as_value;
		uint32_t as_bits;
	} fp32 = { f };
	return fp32.as_bits;
}

inline float ggml_fp16_to_fp32(ggml_fp16_t h) {
    const uint32_t w = (uint32_t) h << 16;
    const uint32_t sign = w & UINT32_C(0x80000000);
    const uint32_t two_w = w + w;

    const uint32_t exp_offset = UINT32_C(0xE0) << 23;
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) || defined(__GNUC__) && !defined(__STRICT_ANSI__)
    const float exp_scale = 0x1.0p-112f;
#else
    const float exp_scale = fp32_from_bits(UINT32_C(0x7800000));
#endif
    const float normalized_value = fp32_from_bits((two_w >> 4) + exp_offset) * exp_scale;

    const uint32_t magic_mask = UINT32_C(126) << 23;
    const float magic_bias = 0.5f;
    const float denormalized_value = fp32_from_bits((two_w >> 17) | magic_mask) - magic_bias;

    const uint32_t denormalized_cutoff = UINT32_C(1) << 27;
    const uint32_t result = sign |
        (two_w < denormalized_cutoff ? fp32_to_bits(denormalized_value) : fp32_to_bits(normalized_value));
    return fp32_from_bits(result);
}

inline ggml_fp16_t ggml_fp32_to_fp16(float f) {
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) || defined(__GNUC__) && !defined(__STRICT_ANSI__)
    const float scale_to_inf = 0x1.0p+112f;
    const float scale_to_zero = 0x1.0p-110f;
#else
    const float scale_to_inf = fp32_from_bits(UINT32_C(0x77800000));
    const float scale_to_zero = fp32_from_bits(UINT32_C(0x08800000));
#endif
    float base = (fabsf(f) * scale_to_inf) * scale_to_zero;

    const uint32_t w = fp32_to_bits(f);
    const uint32_t shl1_w = w + w;
    const uint32_t sign = w & UINT32_C(0x80000000);
    uint32_t bias = shl1_w & UINT32_C(0xFF000000);
    if (bias < UINT32_C(0x71000000)) {
        bias = UINT32_C(0x71000000);
    }

    base = fp32_from_bits((bias >> 1) + UINT32_C(0x07800000)) + base;
    const uint32_t bits = fp32_to_bits(base);
    const uint32_t exp_bits = (bits >> 13) & UINT32_C(0x00007C00);
    const uint32_t mantissa_bits = bits & UINT32_C(0x00000FFF);
    const uint32_t nonsign = exp_bits + mantissa_bits;
    return (sign >> 16) | (shl1_w > UINT32_C(0xFF000000) ? UINT16_C(0x7E00) : nonsign);
}


#ifdef __F16C__
inline float ggml_fp16_to_fp32_f16c(ggml_fp16_t h) {
    return _cvtsh_ss(h);
}
inline ggml_fp16_t ggml_fp32_to_fp16_f16c(float f) {
    return _cvtss_sh(f, 0);
}
#endif


//inline static void ggml_vec_set_i8(const int n, int8_t * x, const int8_t v) { for (int i = 0; i < n; ++i) x[i] = v; }

//inline static void ggml_vec_set_i16(const int n, int16_t * x, const int16_t v) { for (int i = 0; i < n; ++i) x[i] = v; }

//inline static void ggml_vec_set_i32(const int n, int32_t * x, const int32_t v) { for (int i = 0; i < n; ++i) x[i] = v; }

//inline static void ggml_vec_set_f16(const int n, ggml_fp16_t * x, const int32_t v) { for (int i = 0; i < n; ++i) x[i] = v; }

//inline static void ggml_vec_add_f32 (const int n, float * z, const float * x, const float * y) { for (int i = 0; i < n; ++i) z[i]  = x[i] + y[i]; }
#ifdef __AVX2__
void ggml_vec_add_f32_avx2 (const int n, float * z, const float * x, const float * y)
{
    // AVX 256-bit
    const int n32 = (n & ~31);
    __m256 x0, x1, x2, x3;
    __m256 y0, y1, y2, y3;

    for (int i = 0; i < n32; i += 32) {
        x0 = _mm256_loadu_ps(x + i + 0);
        x1 = _mm256_loadu_ps(x + i + 8);
        x2 = _mm256_loadu_ps(x + i + 16);
        x3 = _mm256_loadu_ps(x + i + 24);

        y0 = _mm256_loadu_ps(y + i + 0);
        y1 = _mm256_loadu_ps(y + i + 8);
        y2 = _mm256_loadu_ps(y + i + 16);
        y3 = _mm256_loadu_ps(y + i + 24);

	y0 = _mm256_add_ps(x0, y0);
	y1 = _mm256_add_ps(x1, y1);
	y2 = _mm256_add_ps(x2, y2);
	y3 = _mm256_add_ps(x3, y3);

	_mm256_storeu_ps(z + i + 0, y0);
	_mm256_storeu_ps(z + i + 8, y1);
	_mm256_storeu_ps(z + i + 16, y2);
	_mm256_storeu_ps(z + i + 24, y3);
    }
    // leftovers
    for (int i = n32; i < n; ++i) {
	z[i]  = x[i] + y[i];
    }
}
#endif

void ggml_vec_add_f32_avx (const int n, float * z, const float * x, const float * y)
{
    // AVX 256-bit
    const int n32 = (n & ~31);
    __m256 x0, x1, x2, x3;
    __m256 y0, y1, y2, y3;

    for (int i = 0; i < n32; i += 32) {
        x0 = _mm256_loadu_ps(x + i + 0);
        x1 = _mm256_loadu_ps(x + i + 8);
        x2 = _mm256_loadu_ps(x + i + 16);
        x3 = _mm256_loadu_ps(x + i + 24);

        y0 = _mm256_loadu_ps(y + i + 0);
        y1 = _mm256_loadu_ps(y + i + 8);
        y2 = _mm256_loadu_ps(y + i + 16);
        y3 = _mm256_loadu_ps(y + i + 24);

	y0 = _mm256_add_ps(x0, y0);
	y1 = _mm256_add_ps(x1, y1);
	y2 = _mm256_add_ps(x2, y2);
	y3 = _mm256_add_ps(x3, y3);

	_mm256_storeu_ps(z + i + 0, y0);
	_mm256_storeu_ps(z + i + 8, y1);
	_mm256_storeu_ps(z + i + 16, y2);
	_mm256_storeu_ps(z + i + 24, y3);
    }
    // leftovers
    for (int i = n32; i < n; ++i) {
	z[i]  = x[i] + y[i];
    }
}

void ggml_vec_add_f32_normal (const int n, float * z, const float * x, const float * y)
{
    for (int i = 0; i < n; ++i) {
	z[i]  = x[i] + y[i];
    }
}

//inline static void ggml_vec_acc_f32 (const int n, float * y, const float * x)                  { for (int i = 0; i < n; ++i) y[i] += x[i];        }
//inline static void ggml_vec_acc1_f32(const int n, float * y, const float   v)                  { for (int i = 0; i < n; ++i) y[i] += v;           }
//inline static void ggml_vec_sub_f32 (const int n, float * z, const float * x, const float * y) { for (int i = 0; i < n; ++i) z[i]  = x[i] - y[i]; }
//inline static void ggml_vec_set_f32 (const int n, float * x, const float   v)                  { for (int i = 0; i < n; ++i) x[i]  = v;           }
//inline static void ggml_vec_cpy_f32 (const int n, float * y, const float * x)                  { for (int i = 0; i < n; ++i) y[i]  = x[i];        }
//inline static void ggml_vec_neg_f32 (const int n, float * y, const float * x)                  { for (int i = 0; i < n; ++i) y[i]  = -x[i];       }
//inline static void ggml_vec_mul_f32 (const int n, float * z, const float * x, const float * y) { for (int i = 0; i < n; ++i) z[i]  = x[i]*y[i];   }
//inline static void ggml_vec_div_f32 (const int n, float * z, const float * x, const float * y) { for (int i = 0; i < n; ++i) z[i]  = x[i]/y[i];   }


// inline static void ggml_vec_dot_f32(const int n, float * restrict s, const float * restrict x, const float * restrict y)
#ifdef __AVX2__
void ggml_vec_dot_f32_avx2(const int n, float *s, const float *x, const float *y)
{
    ggml_float sumf = 0.0;
    // AVX 256-bit
    const int n32 = (n & ~31);

    __m256 sum0 = _mm256_setzero_ps();
    __m256 sum1 = _mm256_setzero_ps();
    __m256 sum2 = _mm256_setzero_ps();
    __m256 sum3 = _mm256_setzero_ps();

    __m256 x0, x1, x2, x3;
    __m256 y0, y1, y2, y3;

    for (int i = 0; i < n32; i += 32) {
        x0 = _mm256_loadu_ps(x + i + 0);
        x1 = _mm256_loadu_ps(x + i + 8);
        x2 = _mm256_loadu_ps(x + i + 16);
        x3 = _mm256_loadu_ps(x + i + 24);

        y0 = _mm256_loadu_ps(y + i + 0);
        y1 = _mm256_loadu_ps(y + i + 8);
        y2 = _mm256_loadu_ps(y + i + 16);
        y3 = _mm256_loadu_ps(y + i + 24);

        sum0 = _mm256_fmadd_ps(x0, y0, sum0);
        sum1 = _mm256_fmadd_ps(x1, y1, sum1);
        sum2 = _mm256_fmadd_ps(x2, y2, sum2);
        sum3 = _mm256_fmadd_ps(x3, y3, sum3);
    }

    sum0 = _mm256_add_ps(sum0, sum1);
    sum2 = _mm256_add_ps(sum2, sum3);
    sum0 = _mm256_add_ps(sum0, sum2);

    const __m128 r4 = _mm_add_ps(_mm256_castps256_ps128(sum0), _mm256_extractf128_ps(sum0, 1));
    const __m128 r2 = _mm_add_ps(r4, _mm_movehl_ps(r4, r4));
    const __m128 r1 = _mm_add_ss(r2, _mm_movehdup_ps(r2));

    sumf = _mm_cvtss_f32(r1);

    // leftovers
    for (int i = n32; i < n; ++i) {
        sumf += x[i]*y[i];
    }
    *s = sumf;
}
#endif

void ggml_vec_dot_f32_avx(const int n, float *s, const float *x, const float *y)
{
    ggml_float sumf = 0.0;
    // AVX 256-bit
    const int n32 = (n & ~31);

    __m256 sum0 = _mm256_setzero_ps();
    __m256 sum1 = _mm256_setzero_ps();
    __m256 sum2 = _mm256_setzero_ps();
    __m256 sum3 = _mm256_setzero_ps();

    __m256 x0, x1, x2, x3;
    __m256 y0, y1, y2, y3;

    for (int i = 0; i < n32; i += 32) {
        x0 = _mm256_loadu_ps(x + i + 0);
        x1 = _mm256_loadu_ps(x + i + 8);
        x2 = _mm256_loadu_ps(x + i + 16);
        x3 = _mm256_loadu_ps(x + i + 24);

        y0 = _mm256_loadu_ps(y + i + 0);
        y1 = _mm256_loadu_ps(y + i + 8);
        y2 = _mm256_loadu_ps(y + i + 16);
        y3 = _mm256_loadu_ps(y + i + 24);

	sum0 = _mm256_add_ps(_mm256_mul_ps(x0, y0), sum0);
	sum1 = _mm256_add_ps(_mm256_mul_ps(x1, y1), sum1);
	sum2 = _mm256_add_ps(_mm256_mul_ps(x2, y2), sum2);
	sum3 = _mm256_add_ps(_mm256_mul_ps(x3, y3), sum3);
    }

    sum0 = _mm256_add_ps(sum0, sum1);
    sum2 = _mm256_add_ps(sum2, sum3);
    sum0 = _mm256_add_ps(sum0, sum2);

    const __m128 r4 = _mm_add_ps(_mm256_castps256_ps128(sum0), _mm256_extractf128_ps(sum0, 1));
    const __m128 r2 = _mm_add_ps(r4, _mm_movehl_ps(r4, r4));
    const __m128 r1 = _mm_add_ss(r2, _mm_movehdup_ps(r2));

    sumf = _mm_cvtss_f32(r1);

    // leftovers
    for (int i = n32; i < n; ++i) {
        sumf += x[i]*y[i];
    }
    *s = sumf;
}

void ggml_vec_dot_f32_normal(const int n, float *s, const float *x, const float *y)
{
    ggml_float sumf = 0.0;
    // scalar
    for (int i = 0; i < n; ++i) {
        sumf += x[i]*y[i];
    }
    *s = sumf;
}


// inline static void ggml_vec_dot_f16(const int n, float * restrict s, ggml_fp16_t * restrict x, ggml_fp16_t * restrict y)
#ifdef __AVX2__
void ggml_vec_dot_f16_avx2(const int n, float *s, ggml_fp16_t *x, ggml_fp16_t *y)
{
    ggml_float sumf = 0.0;
    // AVX 256-bit
    const int n32 = (n & ~31);

    __m256 sum0 = _mm256_setzero_ps();
    __m256 sum1 = _mm256_setzero_ps();
    __m256 sum2 = _mm256_setzero_ps();
    __m256 sum3 = _mm256_setzero_ps();

    __m256 x0, x1, x2, x3;
    __m256 y0, y1, y2, y3;

    for (int i = 0; i < n32; i += 32) {
        x0 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 0 )));
        x1 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 8 )));
        x2 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 16)));
        x3 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 24)));

        y0 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 0 )));
        y1 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 8 )));
        y2 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 16)));
        y3 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 24)));

        sum0 = _mm256_fmadd_ps(x0, y0, sum0);
        sum1 = _mm256_fmadd_ps(x1, y1, sum1);
        sum2 = _mm256_fmadd_ps(x2, y2, sum2);
        sum3 = _mm256_fmadd_ps(x3, y3, sum3);
    }

    const __m256 sum01 = _mm256_add_ps(sum0, sum1);
    const __m256 sum23 = _mm256_add_ps(sum2, sum3);
    const __m256 sum0123 = _mm256_add_ps(sum01, sum23);

    const __m128 r4 = _mm_add_ps(_mm256_castps256_ps128(sum0123), _mm256_extractf128_ps(sum0123, 1));
    const __m128 r2 = _mm_add_ps(r4, _mm_movehl_ps(r4, r4));
    const __m128 r1 = _mm_add_ss(r2, _mm_movehdup_ps(r2));

    sumf = _mm_cvtss_f32(r1);

    // leftovers
    for (int i = n32; i < n; ++i) {
        //GGML_ASSERT(false);
        sumf += ggml_fp16_to_fp32(x[i])*ggml_fp16_to_fp32(y[i]);
    }
    *s = sumf;
}
#endif

void ggml_vec_dot_f16_avx(const int n, float *s, ggml_fp16_t *x, ggml_fp16_t *y)
{
    ggml_float sumf = 0.0;
    // AVX 256-bit
    const int n32 = (n & ~31);

    __m256 sum0 = _mm256_setzero_ps();
    __m256 sum1 = _mm256_setzero_ps();
    __m256 sum2 = _mm256_setzero_ps();
    __m256 sum3 = _mm256_setzero_ps();

    __m256 x0, x1, x2, x3;
    __m256 y0, y1, y2, y3;

    for (int i = 0; i < n32; i += 32) {
        x0 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 0 )));
        x1 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 8 )));
        x2 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 16)));
        x3 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 24)));

        y0 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 0 )));
        y1 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 8 )));
        y2 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 16)));
        y3 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 24)));

	sum0 = _mm256_add_ps(_mm256_mul_ps(x0, y0), sum0);
	sum1 = _mm256_add_ps(_mm256_mul_ps(x1, y1), sum1);
	sum2 = _mm256_add_ps(_mm256_mul_ps(x2, y2), sum2);
	sum3 = _mm256_add_ps(_mm256_mul_ps(x3, y3), sum3);
    }

    const __m256 sum01 = _mm256_add_ps(sum0, sum1);
    const __m256 sum23 = _mm256_add_ps(sum2, sum3);
    const __m256 sum0123 = _mm256_add_ps(sum01, sum23);

    const __m128 r4 = _mm_add_ps(_mm256_castps256_ps128(sum0123), _mm256_extractf128_ps(sum0123, 1));
    const __m128 r2 = _mm_add_ps(r4, _mm_movehl_ps(r4, r4));
    const __m128 r1 = _mm_add_ss(r2, _mm_movehdup_ps(r2));

    sumf = _mm_cvtss_f32(r1);

    // leftovers
    for (int i = n32; i < n; ++i) {
        //GGML_ASSERT(false);
        sumf += ggml_fp16_to_fp32(x[i])*ggml_fp16_to_fp32(y[i]);
    }
    *s = sumf;
}

void ggml_vec_dot_f16_normal(const int n, float *s, ggml_fp16_t *x, ggml_fp16_t *y)
{
    ggml_float sumf = 0.0;
    for (int i = 0; i < n; ++i) {
        sumf += ggml_fp16_to_fp32(x[i])*ggml_fp16_to_fp32(y[i]);
    }
    *s = sumf;
}


// inline static void ggml_vec_mad_f32(const int n, float * restrict y, const float * restrict x, const float v)
#ifdef __AVX2__
void ggml_vec_mad_f32_avx2(const int n, float *y, const float *x, const float v)
{
    // AVX 256-bit
    const int n32 = (n & ~31);

    const __m256 v4 = _mm256_set1_ps(v);

    __m256 x0, x1, x2, x3;
    __m256 y0, y1, y2, y3;

    for (int i = 0; i < n32; i += 32) {
        x0 = _mm256_loadu_ps(x + i + 0);
        x1 = _mm256_loadu_ps(x + i + 8);
        x2 = _mm256_loadu_ps(x + i + 16);
        x3 = _mm256_loadu_ps(x + i + 24);

        y0 = _mm256_loadu_ps(y + i + 0);
        y1 = _mm256_loadu_ps(y + i + 8);
        y2 = _mm256_loadu_ps(y + i + 16);
        y3 = _mm256_loadu_ps(y + i + 24);

        y0 = _mm256_fmadd_ps(x0, v4, y0);
        y1 = _mm256_fmadd_ps(x1, v4, y1);
        y2 = _mm256_fmadd_ps(x2, v4, y2);
        y3 = _mm256_fmadd_ps(x3, v4, y3);

        _mm256_storeu_ps(y + i + 0, y0);
        _mm256_storeu_ps(y + i + 8, y1);
        _mm256_storeu_ps(y + i + 16, y2);
        _mm256_storeu_ps(y + i + 24, y3);
    }

    // leftovers
    for (int i = n32; i < n; ++i) {
        y[i] += x[i]*v;
    }
}
#endif

void ggml_vec_mad_f32_avx(const int n, float *y, const float *x, const float v)
{
    // AVX 256-bit
    const int n32 = (n & ~31);

    const __m256 v4 = _mm256_set1_ps(v);

    __m256 x0, x1, x2, x3;
    __m256 y0, y1, y2, y3;

    for (int i = 0; i < n32; i += 32) {
        x0 = _mm256_loadu_ps(x + i + 0);
        x1 = _mm256_loadu_ps(x + i + 8);
        x2 = _mm256_loadu_ps(x + i + 16);
        x3 = _mm256_loadu_ps(x + i + 24);

        y0 = _mm256_loadu_ps(y + i + 0);
        y1 = _mm256_loadu_ps(y + i + 8);
        y2 = _mm256_loadu_ps(y + i + 16);
        y3 = _mm256_loadu_ps(y + i + 24);

	y0 = _mm256_add_ps(_mm256_mul_ps(x0, v4), y0);
	y1 = _mm256_add_ps(_mm256_mul_ps(x1, v4), y1);
	y2 = _mm256_add_ps(_mm256_mul_ps(x2, v4), y2);
	y3 = _mm256_add_ps(_mm256_mul_ps(x3, v4), y3);

        _mm256_storeu_ps(y + i + 0, y0);
        _mm256_storeu_ps(y + i + 8, y1);
        _mm256_storeu_ps(y + i + 16, y2);
        _mm256_storeu_ps(y + i + 24, y3);
    }

    // leftovers
    for (int i = n32; i < n; ++i) {
        y[i] += x[i]*v;
    }
}

void ggml_vec_mad_f32_normal(const int n, float *y, const float *x, const float v)
{
    // scalar
    for (int i = 0; i < n; ++i) {
        y[i] += x[i]*v;
    }
}


// inline static void ggml_vec_mad_f16(const int n, ggml_fp16_t * restrict y, ggml_fp16_t * restrict x, const float v)
#ifdef __AVX2__
void ggml_vec_mad_f16_avx2(const int n, ggml_fp16_t *y, ggml_fp16_t *x, const float v)
{
    // AVX 256-bit
    const int n32 = (n & ~31);

    const __m256 v8 = _mm256_set1_ps(v);

    __m256 x0, x1, x2, x3;
    __m256 y0, y1, y2, y3;

    for (int i = 0; i < n32; i += 32) {
        y0 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 0 )));
        y1 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 8 )));
        y2 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 16)));
        y3 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 24)));

        x0 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 0 )));
        x1 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 8 )));
        x2 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 16)));
        x3 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 24)));

        y0 = _mm256_fmadd_ps(x0, v8, y0);
        y1 = _mm256_fmadd_ps(x1, v8, y1);
        y2 = _mm256_fmadd_ps(x2, v8, y2);
        y3 = _mm256_fmadd_ps(x3, v8, y3);

        _mm_storeu_si128((__m128i*)(y + i + 0 ), _mm256_cvtps_ph(y0, 0));
        _mm_storeu_si128((__m128i*)(y + i + 8 ), _mm256_cvtps_ph(y1, 0));
        _mm_storeu_si128((__m128i*)(y + i + 16), _mm256_cvtps_ph(y2, 0));
        _mm_storeu_si128((__m128i*)(y + i + 24), _mm256_cvtps_ph(y3, 0));
    }

    // leftovers
    for (int i = n32; i < n; ++i) {
        GGML_ASSERT(false);
        y[i] = ggml_fp32_to_fp16(ggml_fp16_to_fp32(y[i]) + ggml_fp16_to_fp32(x[i])*v);
    }
}
#endif

void ggml_vec_mad_f16_avx(const int n, ggml_fp16_t *y, ggml_fp16_t *x, const float v)
{
    // AVX 256-bit
    const int n32 = (n & ~31);

    const __m256 v8 = _mm256_set1_ps(v);

    __m256 x0, x1, x2, x3;
    __m256 y0, y1, y2, y3;

    for (int i = 0; i < n32; i += 32) {
        y0 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 0 )));
        y1 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 8 )));
        y2 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 16)));
        y3 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(y + i + 24)));

        x0 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 0 )));
        x1 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 8 )));
        x2 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 16)));
        x3 = _mm256_cvtph_ps(_mm_loadu_si128((__m128i*)(x + i + 24)));

	y0 = _mm256_add_ps(_mm256_mul_ps(x0, v8), y0);
	y1 = _mm256_add_ps(_mm256_mul_ps(x1, v8), y1);
	y2 = _mm256_add_ps(_mm256_mul_ps(x2, v8), y2);
	y3 = _mm256_add_ps(_mm256_mul_ps(x3, v8), y3);

        _mm_storeu_si128((__m128i*)(y + i + 0 ), _mm256_cvtps_ph(y0, 0));
        _mm_storeu_si128((__m128i*)(y + i + 8 ), _mm256_cvtps_ph(y1, 0));
        _mm_storeu_si128((__m128i*)(y + i + 16), _mm256_cvtps_ph(y2, 0));
        _mm_storeu_si128((__m128i*)(y + i + 24), _mm256_cvtps_ph(y3, 0));
    }

    // leftovers
    for (int i = n32; i < n; ++i) {
        //GGML_ASSERT(false);
        y[i] = ggml_fp32_to_fp16(ggml_fp16_to_fp32(y[i]) + ggml_fp16_to_fp32(x[i])*v);
    }
}

void ggml_vec_mad_f16_normal(const int n, ggml_fp16_t *y, ggml_fp16_t *x, const float v)
{
    for (int i = 0; i < n; ++i) {
        y[i] = ggml_fp32_to_fp16(ggml_fp16_to_fp32(y[i]) + ggml_fp16_to_fp32(x[i])*v);
    }
}


//inline static void ggml_vec_scale_f32(const int n, float * y, const float   v) { for (int i = 0; i < n; ++i) y[i] *= v;          }
#ifdef __AVX__
inline static void ggml_vec_scale_f32_avx(const int n, float * y, const float   v)
{
    // AVX 256-bit
    const int n32 = (n & ~31);

    const __m256 v4 = _mm256_set1_ps(v);

    __m256 y0, y1, y2, y3;

    for (int i = 0; i < n32; i += 32) {
        y0 = _mm256_loadu_ps(y + i + 0);
        y1 = _mm256_loadu_ps(y + i + 8);
        y2 = _mm256_loadu_ps(y + i + 16);
        y3 = _mm256_loadu_ps(y + i + 24);

	y0 = _mm256_mul_ps(y0, v4);
	y1 = _mm256_mul_ps(y1, v4);
	y2 = _mm256_mul_ps(y2, v4);
	y3 = _mm256_mul_ps(y3, v4);

        _mm256_storeu_ps(y + i + 0, y0);
        _mm256_storeu_ps(y + i + 8, y1);
        _mm256_storeu_ps(y + i + 16, y2);
        _mm256_storeu_ps(y + i + 24, y3);
    }

    // leftovers
    for (int i = n32; i < n; ++i) {
        y[i] *= v;
    }
}
#endif

inline static void ggml_vec_scale_f32_normal(const int n, float * y, const float   v)
{
    for (int i = 0; i < n; ++i) y[i] *= v;
}

//inline static void ggml_vec_norm_f32 (const int n, float * s, const float * x) { ggml_vec_dot_f32(n, s, x, x); *s = sqrt(*s);   }
//inline static void ggml_vec_sqr_f32  (const int n, float * y, const float * x) { for (int i = 0; i < n; ++i) y[i] = x[i]*x[i];   }
//inline static void ggml_vec_sqrt_f32 (const int n, float * y, const float * x) { for (int i = 0; i < n; ++i) y[i] = sqrt(x[i]); }
//inline static void ggml_vec_abs_f32  (const int n, float * y, const float * x) { for (int i = 0; i < n; ++i) y[i] = fabsf(x[i]); }
//inline static void ggml_vec_sgn_f32  (const int n, float * y, const float * x) { for (int i = 0; i < n; ++i) y[i] = (x[i] > 0.f) ? 1.f : ((x[i] < 0.f) ? -1.f : 0.f); }
//inline static void ggml_vec_step_f32 (const int n, float * y, const float * x) { for (int i = 0; i < n; ++i) y[i] = (x[i] > 0.f) ? 1.f : 0.f; }
//inline static void ggml_vec_relu_f32 (const int n, float * y, const float * x) { for (int i = 0; i < n; ++i) y[i] = (x[i] > 0.f) ? x[i] : 0.f; }

//inline static void ggml_vec_sum_f32     (const int n, float * s, const float * x) { ggml_float sum = 0.0; for (int i = 0; i < n; ++i) sum += x[i]; *s += sum; }
//inline static void ggml_vec_norm_inv_f32(const int n, float * s, const float * x) { ggml_vec_norm_f32(n, s, x); *s = 1./(*s); }

#ifdef  __cplusplus
}
#endif


// stop watch class
class stopwatch {
public:
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point now;

stopwatch()
{
    start = std::chrono::system_clock::now();
}

double elapsed_ms()
{
    now = std::chrono::system_clock::now();
    double elapsed = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(now - start).count() / 1000.0);
    return elapsed;
}

};


// copy from z to Z
void copy(const int n, float *Z, const float *z)
{
    for (int i = 0; i < n; ++i)
    {
        Z[i] = z[i];
    }
}

// converted copy from z to Z 
void copy_fp32_to_fp16(const int n, ggml_fp16_t *Z, const float *z)
{
    for (int i = 0; i < n; ++i)
    {
        Z[i] = ggml_fp32_to_fp16(z[i]);
    }
}

// calc elapsed time in msec
template<class T>
double calc_for_ggml(T t, unsigned ms)
{
    stopwatch sw;
    int cnt = 0;
    volatile float sum = 0; //最適化で消されるの防止
    while(sw.elapsed_ms() < ms)
    {
	try
	{
            t();
            ++cnt;
	}
	catch (const char *s)
	{
	    printf("catch: %s\n", s);
	    return 0;
	}
    }
#if 1
    return sw.elapsed_ms() / cnt;
#else
    return cnt;
#endif
}

// compare
template<class T1, class T2>
bool compare(const char *func, T1 t1, T2 t2, const int n, const float *z, const float *w)
{
    t1();
    t2();

    int max_diff = 5; 
    int n_diff = 0;
    for (int i = 0; i < n; ++i)
    {
        if (std::fabs(z[i]- w[i]) > FLT_EPSILON)
        {
            printf("%s: different: i:%d z:%lf, w:%lf\n", func, i, z[i], w[i]);
            ++n_diff;
            if (n_diff == max_diff)
            {
                break;
            }
        }
    }

    if (n_diff == 0)
    {
        printf("%s: same:%lf\n", func, z[0]);
        return true;
    }

    return false;
}

template<class T1, class T2>
bool compare(const char *func, T1 t1, T2 t2, const int n, const ggml_fp16_t *Z, const ggml_fp16_t *W)
{
    t1();
    t2();

    int max_diff = 5; 
    int n_diff = 0;
    for (int i = 0; i < n; ++i)
    {
        if (Z[i] != W[i])
        {
            printf("%s: different: i:%d Z:%u, W:%u\n", func, i, Z[i], W[i]);
            ++n_diff;
            if (n_diff == max_diff)
            {
                break;
            }
        }
    }

    if (n_diff == 0)
    {
        printf("%s: same:%u\n", func, Z[0]);
        return true;
    }

    return false;
}


int main(int argc, char** argv)
{
    const unsigned len_begin = 8;
    const unsigned len_end   = 512 * 1024;
    const unsigned len_fact  = 2;
    //const unsigned run_ms    = 250;
    const unsigned run_ms    = 500;

    // option
    int verbose = 0;
    long noalign = 0; // no align 32bytes address
    int nocomp = 0; // no compare
    int noperf = 0; // no performance test

    if (argc > 1)
    {
	for(int i = 1; i < argc; i++)
	{
	    printf("arg:%d %s\n", i, argv[i]);
	    if (strcmp(argv[i], "-h") == 0)
	    {
		printf("test-ggml: test performace and compare result of ggml AVX, AVX2 code\n");
		printf("usage: test-ggml [-h][-v][-na|--noalign][-l|--useload][-np|--noperf][-nc|--nocompare]\n");
		printf("  -h ... this message\n");
		printf("  -v ... verbose message\n");
		printf("  -na|--noalign ... use not-32bytes-aligned data\n");
		printf("  -np|--noperf ... no performance test\n");
		printf("  -nc|--nocompare ... no comparing normal results with AVX results\n");
		return 0;
	    }
	    if (strcmp(argv[i], "-v") == 0)
	    {
		verbose++;
	    }
	    if ((strcmp(argv[i], "-na") == 0) || (strcmp(argv[i], "--noalign") == 0))
	    {
		noalign = 1;
	    }
	    if ((strcmp(argv[i], "-np") == 0) || (strcmp(argv[i], "--noperf") == 0))
	    {
		noperf = 1;
	    }
	    if ((strcmp(argv[i], "-nc") == 0) || (strcmp(argv[i], "--nocompare") == 0))
	    {
		nocomp = 1;
	    }
	}
    }

    // calc
    std::mt19937 rng;
    std::uniform_real_distribution<> dst(-1, 1);

    // label
    printf("# noalign:%s\n", noalign ? "Y" : "N");
    if (!noperf)
    {
#ifdef __AVX2__
        printf("func\tlen\tnormal\tavx\tavx2\tnormal/avx\tnormal/avx2\n");
#else
        printf("func\tlen\tnormal\tavx\tnormal/avx\n");
#endif
    }

    for(unsigned len = len_begin; len <= len_end; len *= len_fact)
    {
	// float
        float *p1 = new __attribute__((aligned(32))) float[len + 9];
        float *p2 = new __attribute__((aligned(32))) float[len + 9];
        float *p3 = new __attribute__((aligned(32))) float[len + 9];
        float *p4 = new __attribute__((aligned(32))) float[len + 9];
        float *x = p1;
        float *y = p2;
        float *z = p3;
        float *w = p4; // for compare with normal result
        while(reinterpret_cast<long>(x) % 32) ++x;
        while(reinterpret_cast<long>(y) % 32) ++y;
        while(reinterpret_cast<long>(z) % 32) ++z;
        while(reinterpret_cast<long>(w) % 32) ++w;
	// unaligned
	if (noalign != 0)
	{
	    x = reinterpret_cast<float *>((reinterpret_cast<char *>(x) + 1));
	    y = reinterpret_cast<float *>((reinterpret_cast<char *>(y) + 1));
	    z = reinterpret_cast<float *>((reinterpret_cast<char *>(z) + 1));
	    w = reinterpret_cast<float *>((reinterpret_cast<char *>(w) + 1));
	}
	if (verbose > 0) printf("# float x:%p y:%p z:%p\n", x, y, z);

        std::generate(x, x + len, [&rng, &dst](){ return dst(rng); });
        std::generate(y, y + len, [&rng, &dst](){ return dst(rng); });
	

	// fp16
        ggml_fp16_t *p5 = new __attribute__((aligned(32))) ggml_fp16_t[len + 9];
        ggml_fp16_t *p6 = new __attribute__((aligned(32))) ggml_fp16_t[len + 9];
        ggml_fp16_t *p7 = new __attribute__((aligned(32))) ggml_fp16_t[len + 9];
        ggml_fp16_t *p8 = new __attribute__((aligned(32))) ggml_fp16_t[len + 9];
        ggml_fp16_t *X = p5;
        ggml_fp16_t *Y = p6;
        ggml_fp16_t *Z = p7;
        ggml_fp16_t *W = p8; // for compare with normal result
        while(reinterpret_cast<long>(X) % 32) ++X;
        while(reinterpret_cast<long>(Y) % 32) ++Y;
        while(reinterpret_cast<long>(Z) % 32) ++Z;
        while(reinterpret_cast<long>(W) % 32) ++W;
	// unaligned
	if (noalign != 0)
	{
	    X = reinterpret_cast<ggml_fp16_t *>((reinterpret_cast<char *>(X) + 1));
	    Y = reinterpret_cast<ggml_fp16_t *>((reinterpret_cast<char *>(Y) + 1));
	    Z = reinterpret_cast<ggml_fp16_t *>((reinterpret_cast<char *>(Z) + 1));
	    W = reinterpret_cast<ggml_fp16_t *>((reinterpret_cast<char *>(W) + 1));
	}
	if (verbose > 0) printf("# ggml_fp16_t X:%p Y:%p Z:%p\n", X, Y, Z);

	copy_fp32_to_fp16(len, X, x);
	copy_fp32_to_fp16(len, Y, y);

	float v = y[0];
	float s;
	float S;
	int n = len;

	double tnormal = 0;
	double tavx = 0;
#ifdef __F16C__
	double tf16c = 0;
#endif
#ifdef __AVX2__
	double tavx2 = 0;
#endif

	std::string func;

	// inline static void ggml_vec_dot_f32(const int n, float * restrict s, const float * restrict x, const float * restrict y)
	func = "ggml_vec_dot_f32";
        if (nocomp == 0)
	{
	    compare(func.c_str(), [n, &S, x, y](){ return ggml_vec_dot_f32_normal(n, &S, x, y); }, [n, &s, x, y](){ return ggml_vec_dot_f32_avx(n, &s, x, y); }, 1, &s, &S);
#ifdef __AVX2__
            compare(func.c_str(), [n, &S, x, y](){ return ggml_vec_dot_f32_normal(n, &S, x, y); }, [n, &s, x, y](){ return ggml_vec_dot_f32_avx2(n, &s, #endif
#endif
        }
	if (noperf == 0)
	{
	    if (verbose > 0) printf("# start calc normal\n");
	    tnormal = calc_for_ggml([n, &S, x, y](){ return ggml_vec_dot_f32_normal(n, &S, x, y); }, run_ms);
	    if (verbose > 0) printf("# start calc avx\n");
	    tavx    = calc_for_ggml([n, &s, x, y](){ return ggml_vec_dot_f32_avx   (n, &s, x, y); }, run_ms);
#ifdef __AVX2__
	    if (verbose > 0) printf("# start calc avx2\n");
x, y); }, 1, &s, &S);
	    tavx2   = calc_for_ggml([n, &s, x, y](){ return ggml_vec_dot_f32_avx2  (n, &s, x, y); }, run_ms);
#endif
#ifdef __AVX2__
	    printf("%s\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tavx, tavx2, tnormal/tavx, tnormal/tavx2);
#else
	    printf("%s\t%d\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tavx, tnormal/tavx);
#endif
        }

	// inline static void ggml_vec_dot_f16(const int n, float * restrict s, ggml_fp16_t * restrict x, ggml_fp16_t * restrict y)
	func = "ggml_vec_dot_f16";
        if (nocomp == 0)
	{
	    compare(func.c_str(), [n, &S, X, Y](){ return ggml_vec_dot_f16_normal(n, &S, X, Y); }, [n, &s, X, Y](){ return ggml_vec_dot_f16_avx(n, &s, X, Y); }, 1, &s, &S);
#ifdef __AVX2__
            compare(func.c_str(), [n, &S, X, Y](){ return ggml_vec_dot_f16_normal(n, &S, X, Y); }, [n, &s, X, Y](){ return ggml_vec_dot_f16_avx2(n, &s, X, Y); }, 1, &s, &S);
#endif
	}
	if (noperf == 0) 
	{
	    if (verbose > 0) printf("# start calc normal\n");
	    tnormal = calc_for_ggml([n, &S, X, Y](){ return ggml_vec_dot_f16_normal(n, &S, X, Y); }, run_ms);
	    if (verbose > 0) printf("# start calc avx\n");
	    tavx    = calc_for_ggml([n, &s, X, Y](){ return ggml_vec_dot_f16_avx   (n, &s, X, Y); }, run_ms);
#ifdef __AVX2__
	    if (verbose > 0) printf("# start calc avx2\n");
	    tavx2   = calc_for_ggml([n, &s, X, Y](){ return ggml_vec_dot_f16_avx2  (n, &s, X, Y); }, run_ms);
#endif
#ifdef __AVX2__
	    printf("%s\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tavx, tavx2, tnormal/tavx, tnormal/tavx2);
#else
	    printf("%s\t%d\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tavx, tnormal/tavx);
#endif
	}

	// inline static void ggml_vec_mad_f32(const int n, float * restrict y, const float * restrict x, const float v)
	func = "ggml_vec_mad_f32";
	copy(n, w, y);
	copy(n, z, y);
        if (nocomp == 0)
	{
            compare(func.c_str(), [n, w, x, v](){ return ggml_vec_mad_f32_normal(n, w, x, v); }, [n, z, x, v](){ return ggml_vec_mad_f32_avx(n, z, x, v); }, n, z, w);
#ifdef __AVX2__
	    copy(n, w, y);
	    copy(n, z, y);
            if (nocomp == 0) compare(func.c_str(), [n, w, x, v](){ return ggml_vec_mad_f32_normal(n, w, x, v); }, [n, z, x, v](){ return ggml_vec_mad_f32_avx2(n, z, x, v); }, n, z, w);
#endif
	}
	if (noperf == 0) 
	{
	    if (verbose > 0) printf("# start calc normal\n");
	    tnormal = calc_for_ggml([n, w, x, v](){ return ggml_vec_mad_f32_normal(n, w, x, v); }, run_ms);
	    if (verbose > 0) printf("# start calc avx\n");
	    tavx    = calc_for_ggml([n, z, x, v](){ return ggml_vec_mad_f32_avx   (n, z, x, v); }, run_ms);
#ifdef __AVX2__
	    if (verbose > 0) printf("# start calc avx2\n");
	    tavx2   = calc_for_ggml([n, z, x, v](){ return ggml_vec_mad_f32_avx2  (n, z, x, v); }, run_ms);
#endif
#ifdef __AVX2__
	    printf("%s\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tavx, tavx2, tnormal/tavx, tnormal/tavx2);
#else
	    printf("%s\t%d\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tavx, tnormal/tavx);
#endif
	}

	// inline static void ggml_vec_mad_f16(const int n, ggml_fp16_t * restrict y, ggml_fp16_t * restrict x, const float v)
	func = "ggml_vec_mad_f16";
	copy_fp32_to_fp16(n, W, y);
	copy_fp32_to_fp16(n, Z, y);
        if (nocomp == 0)
	{
            compare(func.c_str(), [n, W, X, v](){ return ggml_vec_mad_f16_normal(n, W, X, v); }, [n, Z, X, v](){ return ggml_vec_mad_f16_avx(n, Z, X, v); }, n, Z, W);
#ifdef __AVX2__
	    copy_fp32_to_fp16(n, W, y);
	    copy_fp32_to_fp16(n, Z, y);
            compare(func.c_str(), [n, W, X, v](){ return ggml_vec_mad_f16_normal(n, W, X, v); }, [n, Z, X, v](){ return ggml_vec_mad_f16_avx2(n, Z, X, v); }, n, Z, W);
#endif
	}
	if (noperf == 0) 
	{
	    if (verbose > 0) printf("# start calc normal\n");
	    //copy_fp32_to_fp16(n, W, y);
	    tnormal = calc_for_ggml([n, W, X, v](){ return ggml_vec_mad_f16_normal(n, W, X, v); }, run_ms);
	    if (verbose > 0) printf("# start calc avx\n");
	    //copy_fp32_to_fp16(n, Z, y);
	    tavx    = calc_for_ggml([n, Z, X, v](){ return ggml_vec_mad_f16_avx   (n, Z, X, v); }, run_ms);
#ifdef __AVX2__
	    if (verbose > 0) printf("# start calc avx2\n");
	    //copy_fp32_to_fp16(n, Z, y);
	    tavx2   = calc_for_ggml([n, Z, X, v](){ return ggml_vec_mad_f16_avx2  (n, Z, X, v); }, run_ms);
#endif
#ifdef __AVX2__
	    printf("%s\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tavx, tavx2, tnormal/tavx, tnormal/tavx2);
#else
	    printf("%s\t%d\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tavx, tnormal/tavx);
#endif
	}

	//inline static void ggml_vec_add_f32 (const int n, float * z, const float * x, const float * y) { for (int i = 0; i < n; ++i) z[i]  = x[i] + y[i]; }
	func = "ggml_vec_add_f32";
        if (nocomp == 0)
	{
            compare(func.c_str(), [n, w, x, y](){ return ggml_vec_add_f32_normal(n, w, x, y); }, [n, z, x, y](){ return ggml_vec_add_f32_avx(n, z, x, y); }, n, z, w);
#ifdef __AVX2__
            compare(func.c_str(), [n, w, x, y](){ return ggml_vec_add_f32_normal(n, w, x, y); }, [n, z, x, y](){ return ggml_vec_add_f32_avx2(n, z, x, y); }, n, z, w);
#endif
	}
	if (noperf == 0) 
	{
	    if (verbose > 0) printf("# start calc normal\n");
	    tnormal = calc_for_ggml([n, w, x, y](){ return ggml_vec_add_f32_normal(n, w, x, y); }, run_ms);
	    if (verbose > 0) printf("# start calc avx\n");
	    tavx    = calc_for_ggml([n, z, x, y](){ return ggml_vec_add_f32_avx   (n, z, x, y); }, run_ms);
#ifdef __AVX2__
	    if (verbose > 0) printf("# start calc avx2\n");
	    tavx2   = calc_for_ggml([n, z, x, y](){ return ggml_vec_add_f32_avx2  (n, z, x, y); }, run_ms);
#endif
#ifdef __AVX2__
	    printf("%s\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tavx, tavx2, tnormal/tavx, tnormal/tavx2);
#else
	    printf("%s\t%d\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tavx, tnormal/tavx);
#endif
	}

	//inline static void ggml_vec_scale_f32(const int n, float * y, const float   v) { for (int i = 0; i < n; ++i) y[i] *= v;          }
	func = "ggml_vec_scale_f32";
	copy(n, w, y);
	copy(n, z, y);
        if (nocomp == 0)
	{
            compare(func.c_str(), [n, w, v](){ return ggml_vec_scale_f32_normal(n, w, v); }, [n, z, v](){ return ggml_vec_scale_f32_avx(n, z, v); }, n, w, z);
#ifdef __AVX2__
	    copy(n, w, y);
	    copy(n, z, y);
            compare(func.c_str(), [n, w, v](){ return ggml_vec_scale_f32_normal(n, w, v); }, [n, z, v](){ return ggml_vec_scale_f32_avx2(n, z, v); }, n, w, z);
#endif
	}
	if (noperf == 0) 
	{
	    if (verbose > 0) printf("# start calc normal\n");
	    tnormal = calc_for_ggml([n, w, v](){ return ggml_vec_scale_f32_normal(n, w, v); }, run_ms);
	    if (verbose > 0) printf("# start calc avx\n");
	    tavx    = calc_for_ggml([n, z, v](){ return ggml_vec_scale_f32_avx   (n, z, v); }, run_ms);
#ifdef __AVX2__
	    if (verbose > 0) printf("# start calc avx2\n");
	    tavx2   = calc_for_ggml([n, z, v](){ return ggml_vec_scale_f32_avx2  (n, z, v); }, run_ms);
#endif
#ifdef __AVX2__
	    printf("%s\t%d\t%lf\t%lf\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tavx, tavx2, tnormal/tavx, tnormal/tavx2);
#else
	    printf("%s\t%d\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tavx, tnormal/tavx);
#endif
	}


	//inline float ggml_fp16_to_fp32(ggml_fp16_t h)
	//inline ggml_fp16_t ggml_fp32_to_fp16(float f)
	func = "ggml_fp16_to_fp32";
        if (nocomp == 0)
	{
#ifdef  __F16C__
            compare(func.c_str(), [n, w, X](){ for (int i = 0; i < n; ++i) w[i] = ggml_fp16_to_fp32(X[i]); }, [n, z, X](){ for (int i = 0; i < n; ++i) z[i] = ggml_fp16_to_fp32_f16c(X[i]); }, n, z, w);
#endif
	}
	if (noperf == 0) 
	{
	    if (verbose > 0) printf("# start calc normal\n");
	    tnormal = calc_for_ggml([n, w, X](){ for (int i = 0; i < n; ++i) w[i] = ggml_fp16_to_fp32(X[i]); }, run_ms);
#ifdef  __F16C__
	    if (verbose > 0) printf("# start calc f16c\n");
	    tf16c   = calc_for_ggml([n, z, X](){ for (int i = 0; i < n; ++i) z[i] = ggml_fp16_to_fp32_f16c(X[i]); }, run_ms);
#endif
#ifdef  __F16C__
	    printf("%s\t%d\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tf16c, tnormal/tf16c);
#else
	    printf("%s\t%d\t%lf\n", func.c_str(), n, tnormal);
#endif
	}

	//inline float ggml_fp16_to_fp32(ggml_fp16_t h)
	//inline ggml_fp16_t ggml_fp32_to_fp16(float f)
	func = "ggml_fp32_to_fp16";
        if (nocomp == 0)
	{
#ifdef  __F16C__
            compare(func.c_str(), [n, W, x](){ for (int i = 0; i < n; ++i) W[i] = ggml_fp32_to_fp16(x[i]); }, [n, Z, x](){ for (int i = 0; i < n; ++i) Z[i] = ggml_fp32_to_fp16_f16c(x[i]); }, n, Z, W);
#endif
	}
	if (noperf == 0) 
	{
	    if (verbose > 0) printf("# start calc normal\n");
	    tnormal = calc_for_ggml([n, W, x](){ for (int i = 0; i < n; ++i) W[i] = ggml_fp32_to_fp16(x[i]); }, run_ms);
#ifdef  __F16C__
	    if (verbose > 0) printf("# start calc f16c\n");
	    tf16c   = calc_for_ggml([n, Z, x](){ for (int i = 0; i < n; ++i) Z[i] = ggml_fp32_to_fp16_f16c(x[i]); }, run_ms);
#endif
#ifdef  __F16C__
	    printf("%s\t%d\t%lf\t%lf\t%lf\n", func.c_str(), n, tnormal, tf16c, tnormal/tf16c);
#else
	    printf("%s\t%d\t%lf\n", func.c_str(), n, tnormal);
#endif
	}

        delete[] p1;
        delete[] p2;
        delete[] p3;
        delete[] p4;
        delete[] p5;
        delete[] p6;
        delete[] p7;
        delete[] p8;
    }

    return 0;
}

