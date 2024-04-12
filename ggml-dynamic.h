

#if !defined(DYNAMIC_ARCH_STAGE) || DYNAMIC_ARCH_STAGE == 0


#ifdef DYNAMIC_ARCH
__attribute__((target("f16c")))
void ggml_fp32_to_fp16_row_f16c(const float * x, ggml_fp16_t * y, int64_t n) {
    int64_t i = 0;
    for (; i + 7 < n; i += 8) {
        __m256 x_vec = _mm256_loadu_ps(x + i);
        __m128i y_vec = _mm256_cvtps_ph(x_vec, _MM_FROUND_TO_NEAREST_INT);
        _mm_storeu_si128((__m128i *)(y + i), y_vec);
    }
    for(; i + 3 < n; i += 4) {
        __m128 x_vec = _mm_loadu_ps(x + i);
        __m128i y_vec = _mm_cvtps_ph(x_vec, _MM_FROUND_TO_NEAREST_INT);
        _mm_storel_epi64((__m128i *)(y + i), y_vec);
    }
    for (; i < n; i++) {
        y[i] = GGML_FP32_TO_FP16(x[i]);
    }
}

__attribute__((target("avx512f")))
void ggml_bf16_to_fp32_row_avx512(const ggml_bf16_t * x, float * y, int64_t n) {
    int64_t i = 0;
    for (; i + 16 <= n; i += 16) {
        _mm512_storeu_ps(y + i,
                         _mm512_castsi512_ps(
                             _mm512_slli_epi32(
                                 _mm512_cvtepu16_epi32(
                                     _mm256_loadu_si256(
                                         (const __m256i *)(x + i))),
                                 16)));
    }
    for (; i < n; i++) {
        y[i] = GGML_BF16_TO_FP32(x[i]);
    }
}

__attribute__((target("avx2")))
void ggml_bf16_to_fp32_row_avx2(const ggml_bf16_t * x, float * y, int64_t n) {
    int64_t i = 0;
    for (; i + 8 <= n; i += 8) {
        _mm256_storeu_ps(y + i,
                         _mm256_castsi256_ps(
                             _mm256_slli_epi32(
                                 _mm256_cvtepu16_epi32(
                                     _mm_loadu_si128(
                                         (const __m128i *)(x + i))),
                                 16)));
    }
    for (; i < n; i++) {
        y[i] = GGML_BF16_TO_FP32(x[i]);
    }
}

__attribute__((target("avx512bf16")))
void ggml_fp32_to_bf16_row_avx512bf16(const float * x, ggml_bf16_t * y, int64_t n) {
  int i = 0;
  for (; i + 32 <= n; i += 32) {
        _mm512_storeu_ps(
            (__m512 *)(y + i),
            (__m512)_mm512_cvtne2ps_pbh(_mm512_loadu_ps(x + i + 16),
                                        _mm512_loadu_ps(x + i)));
  }
    for (; i < n; i++) {
        y[i] = GGML_FP32_TO_BF16(x[i]);
    }
}
#endif


#elif DYNAMIC_ARCH_STAGE == 1


#if defined(__AVX512F__) || defined(DYNAMIC_ARCH)

#define GGML_SIMD

// F32 AVX512

#define GGML_F32_AVX512_STEP 64
#define GGML_F32_AVX512_EPR  16

#define GGML_F32x16         __m512
#define GGML_F32x16_ZERO    _mm512_setzero_ps()
#define GGML_F32x16_SET1(x) _mm512_set1_ps(x)
#define GGML_F32x16_LOAD    _mm512_loadu_ps
#define GGML_F32x16_STORE   _mm512_storeu_ps
// _mm512_fmadd_ps is defined in AVX512F so no guard is required
#define GGML_F32x16_FMA(a, b, c) _mm512_fmadd_ps(b, c, a)
#define GGML_F32x16_ADD     _mm512_add_ps
#define GGML_F32x16_MUL     _mm512_mul_ps
#define GGML_F32x16_REDUCE(res, x)                                    \
do {                                                                  \
    int offset = GGML_F32_AVX512_ARR >> 1;                                   \
    for (int i = 0; i < offset; ++i) {                                \
        x[i] = _mm512_add_ps(x[i], x[offset+i]);                      \
    }                                                                 \
    offset >>= 1;                                                     \
    for (int i = 0; i < offset; ++i) {                                \
        x[i] = _mm512_add_ps(x[i], x[offset+i]);                      \
    }                                                                 \
    offset >>= 1;                                                     \
    for (int i = 0; i < offset; ++i) {                                \
        x[i] = _mm512_add_ps(x[i], x[offset+i]);                      \
    }                                                                 \
    res = _mm512_reduce_add_ps(x[0]);                                 \
} while (0)

// TODO: is this optimal ?

#define GGML_F32_AVX512_VEC        GGML_F32x16
#define GGML_F32_AVX512_VEC_ZERO   GGML_F32x16_ZERO
#define GGML_F32_AVX512_VEC_SET1   GGML_F32x16_SET1
#define GGML_F32_AVX512_VEC_LOAD   GGML_F32x16_LOAD
#define GGML_F32_AVX512_VEC_STORE  GGML_F32x16_STORE
#define GGML_F32_AVX512_VEC_FMA    GGML_F32x16_FMA
#define GGML_F32_AVX512_VEC_ADD    GGML_F32x16_ADD
#define GGML_F32_AVX512_VEC_MUL    GGML_F32x16_MUL
#define GGML_F32_AVX512_VEC_REDUCE GGML_F32x16_REDUCE

// F16 AVX512

// F16 AVX

#define GGML_F16_AVX512_STEP 64
#define GGML_F16_AVX512_EPR  16

// AVX512 has FP16 extension (AVX512_FP16) but I don't have it on my machine so I use FP32 instead

#define GGML_F32Cx16             __m512
#define GGML_F32Cx16_ZERO        _mm512_setzero_ps()
#define GGML_F32Cx16_SET1(x)     _mm512_set1_ps(x)

// unlike  _mm256_cvt intrinsics that require F16C, _mm512_cvt is defined in AVX512F
// so F16C guard isn't required
#define GGML_F32Cx16_LOAD(x)     _mm512_cvtph_ps(_mm256_loadu_si256((const __m256i *)(x)))
#define GGML_F32Cx16_STORE(x, y) _mm256_storeu_si256((__m256i *)(x), _mm512_cvtps_ph(y, 0))

#define GGML_F32Cx16_FMA(a, b, c) _mm512_fmadd_ps(b, c, a)
#define GGML_F32Cx16_ADD         _mm512_add_ps
#define GGML_F32Cx16_MUL         _mm512_mul_ps
#define GGML_F32Cx16_REDUCE(res, x)                               \
do {                                                              \
    int offset = GGML_F32_AVX512_ARR >> 1;                        \
    for (int i = 0; i < offset; ++i) {                            \
        x[i] = _mm512_add_ps(x[i], x[offset+i]);                  \
    }                                                             \
    offset >>= 1;                                                 \
    for (int i = 0; i < offset; ++i) {                            \
        x[i] = _mm512_add_ps(x[i], x[offset+i]);                  \
    }                                                             \
    offset >>= 1;                                                 \
    for (int i = 0; i < offset; ++i) {                            \
        x[i] = _mm512_add_ps(x[i], x[offset+i]);                  \
    }                                                             \
    res = _mm512_reduce_add_ps(x[0]);                             \
} while (0)

#define GGML_F16_AVX512_VEC                GGML_F32Cx16
#define GGML_F16_AVX512_VEC_ZERO           GGML_F32Cx16_ZERO
#define GGML_F16_AVX512_VEC_SET1           GGML_F32Cx16_SET1
#define GGML_F16_AVX512_VEC_LOAD(p, i)     GGML_F32Cx16_LOAD(p)
#define GGML_F16_AVX512_VEC_STORE(p, r, i) GGML_F32Cx16_STORE(p, r[i])
#define GGML_F16_AVX512_VEC_FMA            GGML_F32Cx16_FMA
#define GGML_F16_AVX512_VEC_ADD            GGML_F32Cx16_ADD
#define GGML_F16_AVX512_VEC_MUL            GGML_F32Cx16_MUL
#define GGML_F16_AVX512_VEC_REDUCE         GGML_F32Cx16_REDUCE

#endif


#if defined(__AVX__) || defined(DYNAMIC_ARCH)

// F32 AVX

#define GGML_F32_AVX_STEP 32
#define GGML_F32_AVX_EPR  8

#define GGML_F32x8         __m256
#define GGML_F32x8_ZERO    _mm256_setzero_ps()
#define GGML_F32x8_SET1(x) _mm256_set1_ps(x)
#define GGML_F32x8_LOAD    _mm256_loadu_ps
#define GGML_F32x8_STORE   _mm256_storeu_ps
#ifdef DYNAMIC_ARCH
// __attribute__((target("fma")))
// static inline __m256 __avx_f32x8_fma(__m256 a, __m256 b, __m256 c) {
//     return _mm256_fmadd_ps(b, c, a);
// }
// #define GGML_F32x8_FMA2(a, b, c) (cpu_has_fma ? __avx_f32x8_fma(a, b, c) : _mm256_add_ps(_mm256_mul_ps(b, c), a))
#define GGML_F32x8_FMA(a, b, c) _mm256_add_ps(_mm256_mul_ps(b, c), a)
#define GGML_F32x8_FMA2(a, b, c) _mm256_fmadd_ps(b, c, a)
#elif defined(__FMA__)
    #define GGML_F32x8_FMA(a, b, c) _mm256_fmadd_ps(b, c, a)
#else
    #define GGML_F32x8_FMA(a, b, c) _mm256_add_ps(_mm256_mul_ps(b, c), a)
#endif
#define GGML_F32x8_ADD     _mm256_add_ps
#define GGML_F32x8_MUL     _mm256_mul_ps
#define GGML_F32x8_REDUCE(res, x)                                 \
do {                                                              \
    int offset = GGML_F32_AVX_ARR >> 1;                           \
    for (int i = 0; i < offset; ++i) {                            \
        x[i] = _mm256_add_ps(x[i], x[offset+i]);                  \
    }                                                             \
    offset >>= 1;                                                 \
    for (int i = 0; i < offset; ++i) {                            \
        x[i] = _mm256_add_ps(x[i], x[offset+i]);                  \
    }                                                             \
    offset >>= 1;                                                 \
    for (int i = 0; i < offset; ++i) {                            \
        x[i] = _mm256_add_ps(x[i], x[offset+i]);                  \
    }                                                             \
    const __m128 t0 = _mm_add_ps(_mm256_castps256_ps128(x[0]),    \
                                 _mm256_extractf128_ps(x[0], 1)); \
    const __m128 t1 = _mm_hadd_ps(t0, t0);                        \
    res = _mm_cvtss_f32(_mm_hadd_ps(t1, t1));                     \
} while (0)
// TODO: is this optimal ?

#define GGML_F32_AVX_VEC        GGML_F32x8
#define GGML_F32_AVX_VEC_ZERO   GGML_F32x8_ZERO
#define GGML_F32_AVX_VEC_SET1   GGML_F32x8_SET1
#define GGML_F32_AVX_VEC_LOAD   GGML_F32x8_LOAD
#define GGML_F32_AVX_VEC_STORE  GGML_F32x8_STORE
#define GGML_F32_AVX_VEC_FMA    GGML_F32x8_FMA
#define GGML_F32_AVX_VEC_FMA2   GGML_F32x8_FMA2
#define GGML_F32_AVX_VEC_ADD    GGML_F32x8_ADD
#define GGML_F32_AVX_VEC_MUL    GGML_F32x8_MUL
#define GGML_F32_AVX_VEC_REDUCE GGML_F32x8_REDUCE

// F16 AVX

#define GGML_F16_AVX_STEP 32
#define GGML_F16_AVX_EPR  8

// F16 arithmetic is not supported by AVX, so we use F32 instead

#define GGML_F32Cx8             __m256
#define GGML_F32Cx8_ZERO        _mm256_setzero_ps()
#define GGML_F32Cx8_SET1(x)     _mm256_set1_ps(x)

#if defined(__F16C__)
// the  _mm256_cvt intrinsics require F16C
#define GGML_F32Cx8_LOAD(x)     _mm256_cvtph_ps(_mm_loadu_si128((const __m128i *)(x)))
#define GGML_F32Cx8_STORE(x, y) _mm_storeu_si128((__m128i *)(x), _mm256_cvtps_ph(y, 0))
#define GGML_F32Cx8_LOAD2(x)      GGML_F32Cx8_LOAD(x)
#define GGML_F32Cx8_STORE2(x, y)  GGML_F32Cx8_STORE(x, y)
#else
__attribute__((target("avx")))
static inline __m256 __avx_f32cx8_load(const ggml_fp16_t *x) {
    float tmp[8];

    for (int i = 0; i < 8; i++) {
        tmp[i] = GGML_FP16_TO_FP32(x[i]);
    }

    return _mm256_loadu_ps(tmp);
}
__attribute__((target("avx")))
static inline void __avx_f32cx8_store(ggml_fp16_t *x, __m256 y) {
    float arr[8];

    _mm256_storeu_ps(arr, y);

    for (int i = 0; i < 8; i++)
        x[i] = GGML_FP32_TO_FP16(arr[i]);
}
#if defined(DYNAMIC_ARCH)
__attribute__((target("avx,f16c")))
static inline __m256 __avx_f32cx8_load_f16c(const ggml_fp16_t *x) {
    return _mm256_cvtph_ps(_mm_loadu_si128((const __m128i *)(x)));
}
__attribute__((target("avx,f16c")))
static inline void __avx_f32cx8_store_f16c(ggml_fp16_t *x, __m256 y) {
    _mm_storeu_si128((__m128i *)(x), _mm256_cvtps_ph(y, 0));
}
#define GGML_F32Cx8_LOAD(x)     (cpu_has_f16c ? __avx_f32cx8_load_f16c(x) : __avx_f32cx8_load(x))
#define GGML_F32Cx8_STORE(x, y) (cpu_has_f16c ? __avx_f32cx8_store_f16c(x, y) : __avx_f32cx8_store(x, y))
#define GGML_F32Cx8_LOAD2(x)     _mm256_cvtph_ps(_mm_loadu_si128((__m128i *)(x)))
#define GGML_F32Cx8_STORE2(x, y) _mm_storeu_si128((__m128i *)(x), _mm256_cvtps_ph(y, 0))
#else
#define GGML_F32Cx8_LOAD(x)     __avx_f32cx8_load(x)
#define GGML_F32Cx8_STORE(x, y) __avx_f32cx8_store(x, y)
#endif
#endif

#define GGML_F32Cx8_FMA         GGML_F32x8_FMA
#define GGML_F32Cx8_FMA2        GGML_F32x8_FMA2
#define GGML_F32Cx8_ADD         _mm256_add_ps
#define GGML_F32Cx8_MUL         _mm256_mul_ps
#define GGML_F32Cx8_REDUCE      GGML_F32x8_REDUCE

#define GGML_F16_AVX_VEC                GGML_F32Cx8
#define GGML_F16_AVX_VEC_ZERO           GGML_F32Cx8_ZERO
#define GGML_F16_AVX_VEC_SET1           GGML_F32Cx8_SET1
#define GGML_F16_AVX_VEC_LOAD(p, i)     GGML_F32Cx8_LOAD(p)
#define GGML_F16_AVX_VEC_STORE(p, r, i) GGML_F32Cx8_STORE(p, r[i])
#define GGML_F16_AVX_VEC_LOAD2(p, i)     GGML_F32Cx8_LOAD2(p)
#define GGML_F16_AVX_VEC_STORE2(p, r, i) GGML_F32Cx8_STORE2(p, r[i])
#define GGML_F16_AVX_VEC_FMA            GGML_F32Cx8_FMA
#define GGML_F16_AVX_VEC_FMA2           GGML_F32Cx8_FMA2
#define GGML_F16_AVX_VEC_ADD            GGML_F32Cx8_ADD
#define GGML_F16_AVX_VEC_MUL            GGML_F32Cx8_MUL
#define GGML_F16_AVX_VEC_REDUCE         GGML_F32Cx8_REDUCE

#endif  // __AVX__


#if defined(__SSE3__) || defined(DYNAMIC_ARCH)

// F32 SSE

#define GGML_F32_SSE_STEP 32
#define GGML_F32_SSE_EPR  4

#define GGML_F32x4         __m128
#define GGML_F32x4_ZERO    _mm_setzero_ps()
#define GGML_F32x4_SET1(x) _mm_set1_ps(x)
#define GGML_F32x4_LOAD    _mm_loadu_ps
#define GGML_F32x4_STORE   _mm_storeu_ps
#ifdef DYNAMIC_ARCH
// __attribute__((target("sse3,fma")))
// static inline __m128 __sse_f32x4_fma(__m128 a, __m128 b, __m128 c) {
//     return _mm_fmadd_ps(b, c, a);
// }
// #define GGML_F32x4_FMA(a, b, c) (cpu_has_fma ? __sse_f32x4_fma(a, b, c) : _mm_add_ps(_mm_mul_ps(b, c), a))
#define GGML_F32x4_FMA(a, b, c) _mm_add_ps(_mm_mul_ps(b, c), a)
#define GGML_F32x4_FMA2(a, b, c) _mm_fmadd_ps(b, c, a)
#elif defined(__FMA__)
    // TODO: Does this work?
    #define GGML_F32x4_FMA(a, b, c) _mm_fmadd_ps(b, c, a)
#else
    #define GGML_F32x4_FMA(a, b, c) _mm_add_ps(_mm_mul_ps(b, c), a)
#endif
#define GGML_F32x4_ADD     _mm_add_ps
#define GGML_F32x4_MUL     _mm_mul_ps
#define GGML_F32x4_REDUCE(res, x)                                 \
{                                                                 \
    int offset = GGML_F32_SSE_ARR >> 1;                           \
    for (int i = 0; i < offset; ++i) {                            \
        x[i] = _mm_add_ps(x[i], x[offset+i]);                     \
    }                                                             \
    offset >>= 1;                                                 \
    for (int i = 0; i < offset; ++i) {                            \
        x[i] = _mm_add_ps(x[i], x[offset+i]);                     \
    }                                                             \
    offset >>= 1;                                                 \
    for (int i = 0; i < offset; ++i) {                            \
        x[i] = _mm_add_ps(x[i], x[offset+i]);                     \
    }                                                             \
    const __m128 t0 = _mm_hadd_ps(x[0], x[0]);                    \
    res = _mm_cvtss_f32(_mm_hadd_ps(t0, t0));                     \
}
// TODO: is this optimal ?

#define GGML_F32_SSE_VEC        GGML_F32x4
#define GGML_F32_SSE_VEC_ZERO   GGML_F32x4_ZERO
#define GGML_F32_SSE_VEC_SET1   GGML_F32x4_SET1
#define GGML_F32_SSE_VEC_LOAD   GGML_F32x4_LOAD
#define GGML_F32_SSE_VEC_STORE  GGML_F32x4_STORE
#define GGML_F32_SSE_VEC_FMA    GGML_F32x4_FMA
#define GGML_F32_SSE_VEC_FMA2   GGML_F32x4_FMA2
#define GGML_F32_SSE_VEC_ADD    GGML_F32x4_ADD
#define GGML_F32_SSE_VEC_MUL    GGML_F32x4_MUL
#define GGML_F32_SSE_VEC_REDUCE GGML_F32x4_REDUCE

// F16 SSE

#define GGML_F16_SSE_STEP 32
#define GGML_F16_SSE_EPR  4

static inline __m128 __sse_f16x4_load(const ggml_fp16_t *x) {
    float tmp[4];

    tmp[0] = GGML_FP16_TO_FP32(x[0]);
    tmp[1] = GGML_FP16_TO_FP32(x[1]);
    tmp[2] = GGML_FP16_TO_FP32(x[2]);
    tmp[3] = GGML_FP16_TO_FP32(x[3]);

    return _mm_loadu_ps(tmp);
}

static inline void __sse_f16x4_store(ggml_fp16_t *x, __m128 y) {
    float arr[4];

    _mm_storeu_ps(arr, y);

    x[0] = GGML_FP32_TO_FP16(arr[0]);
    x[1] = GGML_FP32_TO_FP16(arr[1]);
    x[2] = GGML_FP32_TO_FP16(arr[2]);
    x[3] = GGML_FP32_TO_FP16(arr[3]);
}

#define GGML_F32Cx4             __m128
#define GGML_F32Cx4_ZERO        _mm_setzero_ps()
#define GGML_F32Cx4_SET1(x)     _mm_set1_ps(x)
#define GGML_F32Cx4_LOAD(x)     __sse_f16x4_load(x)
#define GGML_F32Cx4_STORE(x, y) __sse_f16x4_store(x, y)
#define GGML_F32Cx4_FMA         GGML_F32x4_FMA
#define GGML_F32Cx4_FMA2        GGML_F32x4_FMA2
#define GGML_F32Cx4_ADD         _mm_add_ps
#define GGML_F32Cx4_MUL         _mm_mul_ps
#define GGML_F32Cx4_REDUCE      GGML_F32x4_REDUCE

#define GGML_F16_SSE_VEC                 GGML_F32Cx4
#define GGML_F16_SSE_VEC_ZERO            GGML_F32Cx4_ZERO
#define GGML_F16_SSE_VEC_SET1            GGML_F32Cx4_SET1
#define GGML_F16_SSE_VEC_LOAD(p, i)      GGML_F32Cx4_LOAD(p)
#define GGML_F16_SSE_VEC_STORE(p, r, i)  GGML_F32Cx4_STORE(p, r[i])
#define GGML_F16_SSE_VEC_FMA             GGML_F32Cx4_FMA
#define GGML_F16_SSE_VEC_FMA2            GGML_F32Cx4_FMA2
#define GGML_F16_SSE_VEC_ADD             GGML_F32Cx4_ADD
#define GGML_F16_SSE_VEC_MUL             GGML_F32Cx4_MUL
#define GGML_F16_SSE_VEC_REDUCE          GGML_F32Cx4_REDUCE

#endif // __SSE3__


#elif DYNAMIC_ARCH_STAGE == 2


#ifdef DYNAMIC_ARCH
__attribute__((target("avx512f")))
static void ggml_vec_dot_f32_avx512(int n, float * restrict s, size_t bs, const float * restrict x, size_t bx, const float * restrict y, size_t by, int nrc) {
   assert(nrc == 1);
   UNUSED(nrc);
   UNUSED(bx);
   UNUSED(by);
   UNUSED(bs);

    float sumf = 0.0f;
    const int np = (n & ~(GGML_F32_AVX512_STEP - 1));

    GGML_F32_AVX512_VEC sum[GGML_F32_AVX512_ARR] = { GGML_F32_AVX512_VEC_ZERO };

    GGML_F32_AVX512_VEC ax[GGML_F32_AVX512_ARR];
    GGML_F32_AVX512_VEC ay[GGML_F32_AVX512_ARR];

    for (int i = 0; i < np; i += GGML_F32_AVX512_STEP) {
        for (int j = 0; j < GGML_F32_AVX512_ARR; j++) {
            ax[j] = GGML_F32_AVX512_VEC_LOAD(x + i + j*GGML_F32_AVX512_EPR);
            ay[j] = GGML_F32_AVX512_VEC_LOAD(y + i + j*GGML_F32_AVX512_EPR);

            sum[j] = GGML_F32_AVX512_VEC_FMA(sum[j], ax[j], ay[j]);
        }
    }

    // reduce sum0..sum3 to sum0
    GGML_F32_AVX512_VEC_REDUCE(sumf, sum);

    // leftovers
    for (int i = np; i < n; ++i) {
        sumf += x[i]*y[i];
    }

    *s = sumf;
}

__attribute__((target("avx,fma")))
static void ggml_vec_dot_f32_avx_fma(int n, float * restrict s, size_t bs, const float * restrict x, size_t bx, const float * restrict y, size_t by, int nrc) {
   assert(nrc == 1);
   UNUSED(nrc);
   UNUSED(bx);
   UNUSED(by);
   UNUSED(bs);

    float sumf = 0.0f;
    const int np = (n & ~(GGML_F32_AVX_STEP - 1));

    GGML_F32_AVX_VEC sum[GGML_F32_AVX_ARR] = { GGML_F32_AVX_VEC_ZERO };

    GGML_F32_AVX_VEC ax[GGML_F32_AVX_ARR];
    GGML_F32_AVX_VEC ay[GGML_F32_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F32_AVX_STEP) {
        for (int j = 0; j < GGML_F32_AVX_ARR; j++) {
            ax[j] = GGML_F32_AVX_VEC_LOAD(x + i + j*GGML_F32_AVX_EPR);
            ay[j] = GGML_F32_AVX_VEC_LOAD(y + i + j*GGML_F32_AVX_EPR);

            sum[j] = GGML_F32_AVX_VEC_FMA2(sum[j], ax[j], ay[j]);
        }
    }

    // reduce sum0..sum3 to sum0
    GGML_F32_AVX_VEC_REDUCE(sumf, sum);

    // leftovers
    for (int i = np; i < n; ++i) {
        sumf += x[i]*y[i];
    }

    *s = sumf;
}

__attribute__((target("avx")))
static void ggml_vec_dot_f32_avx(int n, float * restrict s, size_t bs, const float * restrict x, size_t bx, const float * restrict y, size_t by, int nrc) {
   assert(nrc == 1);
   UNUSED(nrc);
   UNUSED(bx);
   UNUSED(by);
   UNUSED(bs);

    float sumf = 0.0f;
    const int np = (n & ~(GGML_F32_AVX_STEP - 1));

    GGML_F32_AVX_VEC sum[GGML_F32_AVX_ARR] = { GGML_F32_AVX_VEC_ZERO };

    GGML_F32_AVX_VEC ax[GGML_F32_AVX_ARR];
    GGML_F32_AVX_VEC ay[GGML_F32_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F32_AVX_STEP) {
        for (int j = 0; j < GGML_F32_AVX_ARR; j++) {
            ax[j] = GGML_F32_AVX_VEC_LOAD(x + i + j*GGML_F32_AVX_EPR);
            ay[j] = GGML_F32_AVX_VEC_LOAD(y + i + j*GGML_F32_AVX_EPR);

            sum[j] = GGML_F32_AVX_VEC_FMA(sum[j], ax[j], ay[j]);
        }
    }

    // reduce sum0..sum3 to sum0
    GGML_F32_AVX_VEC_REDUCE(sumf, sum);

    // leftovers
    for (int i = np; i < n; ++i) {
        sumf += x[i]*y[i];
    }

    *s = sumf;
}

__attribute__((target("sse3,fma")))
static void ggml_vec_dot_f32_sse3_fma(int n, float * restrict s, size_t bs, const float * restrict x, size_t bx, const float * restrict y, size_t by, int nrc) {
   assert(nrc == 1);
   UNUSED(nrc);
   UNUSED(bx);
   UNUSED(by);
   UNUSED(bs);

    float sumf = 0.0f;
    const int np = (n & ~(GGML_F32_SSE_STEP - 1));

    GGML_F32_SSE_VEC sum[GGML_F32_SSE_ARR] = { GGML_F32_SSE_VEC_ZERO };

    GGML_F32_SSE_VEC ax[GGML_F32_SSE_ARR];
    GGML_F32_SSE_VEC ay[GGML_F32_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F32_SSE_STEP) {
        for (int j = 0; j < GGML_F32_SSE_ARR; j++) {
            ax[j] = GGML_F32_SSE_VEC_LOAD(x + i + j*GGML_F32_SSE_EPR);
            ay[j] = GGML_F32_SSE_VEC_LOAD(y + i + j*GGML_F32_SSE_EPR);

            sum[j] = GGML_F32_SSE_VEC_FMA2(sum[j], ax[j], ay[j]);
        }
    }

    // reduce sum0..sum3 to sum0
    GGML_F32_SSE_VEC_REDUCE(sumf, sum);

    // leftovers
    for (int i = np; i < n; ++i) {
        sumf += x[i]*y[i];
    }

    *s = sumf;
}

__attribute__((target("sse3")))
static void ggml_vec_dot_f32_sse3(int n, float * restrict s, size_t bs, const float * restrict x, size_t bx, const float * restrict y, size_t by, int nrc) {
   assert(nrc == 1);
   UNUSED(nrc);
   UNUSED(bx);
   UNUSED(by);
   UNUSED(bs);

    float sumf = 0.0f;
    const int np = (n & ~(GGML_F32_SSE_STEP - 1));

    GGML_F32_SSE_VEC sum[GGML_F32_SSE_ARR] = { GGML_F32_SSE_VEC_ZERO };

    GGML_F32_SSE_VEC ax[GGML_F32_SSE_ARR];
    GGML_F32_SSE_VEC ay[GGML_F32_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F32_SSE_STEP) {
        for (int j = 0; j < GGML_F32_SSE_ARR; j++) {
            ax[j] = GGML_F32_SSE_VEC_LOAD(x + i + j*GGML_F32_SSE_EPR);
            ay[j] = GGML_F32_SSE_VEC_LOAD(y + i + j*GGML_F32_SSE_EPR);

            sum[j] = GGML_F32_SSE_VEC_FMA(sum[j], ax[j], ay[j]);
        }
    }

    // reduce sum0..sum3 to sum0
    GGML_F32_SSE_VEC_REDUCE(sumf, sum);

    // leftovers
    for (int i = np; i < n; ++i) {
        sumf += x[i]*y[i];
    }

    *s = sumf;
}
#endif



#ifdef DYNAMIC_ARCH
__attribute__((target("avx512f")))
static void ggml_vec_dot_f16_avx512(int n, float * restrict s, size_t bs, ggml_fp16_t * restrict x, size_t bx, ggml_fp16_t * restrict y, size_t by, int nrc) {
    assert(nrc == 1);
    UNUSED(nrc);
    UNUSED(bx);
    UNUSED(by);
    UNUSED(bs);

    ggml_float sumf = 0.0;

    const int np = (n & ~(GGML_F16_AVX512_STEP - 1));

    GGML_F16_AVX512_VEC sum[GGML_F16_AVX512_ARR] = { GGML_F16_AVX512_VEC_ZERO };

    GGML_F16_AVX512_VEC ax[GGML_F16_AVX512_ARR];
    GGML_F16_AVX512_VEC ay[GGML_F16_AVX512_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX512_STEP) {
        for (int j = 0; j < GGML_F16_AVX512_ARR; j++) {
            ax[j] = GGML_F16_AVX512_VEC_LOAD(x + i + j*GGML_F16_AVX512_EPR, j);
            ay[j] = GGML_F16_AVX512_VEC_LOAD(y + i + j*GGML_F16_AVX512_EPR, j);

            sum[j] = GGML_F16_AVX512_VEC_FMA(sum[j], ax[j], ay[j]);
        }
    }

    // reduce sum0..sum3 to sum0
    GGML_F16_AVX512_VEC_REDUCE(sumf, sum);

    // leftovers
    for (int i = np; i < n; ++i) {
        sumf += (ggml_float)(GGML_FP16_TO_FP32(x[i])*GGML_FP16_TO_FP32(y[i]));
    }

    *s = sumf;
}

__attribute__((target("avx,fma,f16c")))
static void ggml_vec_dot_f16_avx_fma_f16c(int n, float * restrict s, size_t bs, ggml_fp16_t * restrict x, size_t bx, ggml_fp16_t * restrict y, size_t by, int nrc) {
    assert(nrc == 1);
    UNUSED(nrc);
    UNUSED(bx);
    UNUSED(by);
    UNUSED(bs);

    ggml_float sumf = 0.0;

    const int np = (n & ~(GGML_F16_AVX_STEP - 1));

    GGML_F16_AVX_VEC sum[GGML_F16_AVX_ARR] = { GGML_F16_AVX_VEC_ZERO };

    GGML_F16_AVX_VEC ax[GGML_F16_AVX_ARR];
    GGML_F16_AVX_VEC ay[GGML_F16_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX_STEP) {
        for (int j = 0; j < GGML_F16_AVX_ARR; j++) {
            ax[j] = GGML_F16_AVX_VEC_LOAD2(x + i + j*GGML_F16_AVX_EPR, j);
            ay[j] = GGML_F16_AVX_VEC_LOAD2(y + i + j*GGML_F16_AVX_EPR, j);

            sum[j] = GGML_F16_AVX_VEC_FMA2(sum[j], ax[j], ay[j]);
        }
    }

    // reduce sum0..sum3 to sum0
    GGML_F16_AVX_VEC_REDUCE(sumf, sum);

    // leftovers
    for (int i = np; i < n; ++i) {
        sumf += (ggml_float)(GGML_FP16_TO_FP32(x[i])*GGML_FP16_TO_FP32(y[i]));
    }

    *s = sumf;
}

__attribute__((target("avx,f16c")))
static void ggml_vec_dot_f16_avx_f16c(int n, float * restrict s, size_t bs, ggml_fp16_t * restrict x, size_t bx, ggml_fp16_t * restrict y, size_t by, int nrc) {
    assert(nrc == 1);
    UNUSED(nrc);
    UNUSED(bx);
    UNUSED(by);
    UNUSED(bs);

    ggml_float sumf = 0.0;

    const int np = (n & ~(GGML_F16_AVX_STEP - 1));

    GGML_F16_AVX_VEC sum[GGML_F16_AVX_ARR] = { GGML_F16_AVX_VEC_ZERO };

    GGML_F16_AVX_VEC ax[GGML_F16_AVX_ARR];
    GGML_F16_AVX_VEC ay[GGML_F16_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX_STEP) {
        for (int j = 0; j < GGML_F16_AVX_ARR; j++) {
            ax[j] = GGML_F16_AVX_VEC_LOAD2(x + i + j*GGML_F16_AVX_EPR, j);
            ay[j] = GGML_F16_AVX_VEC_LOAD2(y + i + j*GGML_F16_AVX_EPR, j);

            sum[j] = GGML_F16_AVX_VEC_FMA(sum[j], ax[j], ay[j]);
        }
    }

    // reduce sum0..sum3 to sum0
    GGML_F16_AVX_VEC_REDUCE(sumf, sum);

    // leftovers
    for (int i = np; i < n; ++i) {
        sumf += (ggml_float)(GGML_FP16_TO_FP32(x[i])*GGML_FP16_TO_FP32(y[i]));
    }

    *s = sumf;
}

__attribute__((target("avx,fma")))
static void ggml_vec_dot_f16_avx_fma(int n, float * restrict s, size_t bs, ggml_fp16_t * restrict x, size_t bx, ggml_fp16_t * restrict y, size_t by, int nrc) {
    assert(nrc == 1);
    UNUSED(nrc);
    UNUSED(bx);
    UNUSED(by);
    UNUSED(bs);

    ggml_float sumf = 0.0;

    const int np = (n & ~(GGML_F16_AVX_STEP - 1));

    GGML_F16_AVX_VEC sum[GGML_F16_AVX_ARR] = { GGML_F16_AVX_VEC_ZERO };

    GGML_F16_AVX_VEC ax[GGML_F16_AVX_ARR];
    GGML_F16_AVX_VEC ay[GGML_F16_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX_STEP) {
        for (int j = 0; j < GGML_F16_AVX_ARR; j++) {
            ax[j] = GGML_F16_AVX_VEC_LOAD(x + i + j*GGML_F16_AVX_EPR, j);
            ay[j] = GGML_F16_AVX_VEC_LOAD(y + i + j*GGML_F16_AVX_EPR, j);

            sum[j] = GGML_F16_AVX_VEC_FMA2(sum[j], ax[j], ay[j]);
        }
    }

    // reduce sum0..sum3 to sum0
    GGML_F16_AVX_VEC_REDUCE(sumf, sum);

    // leftovers
    for (int i = np; i < n; ++i) {
        sumf += (ggml_float)(GGML_FP16_TO_FP32(x[i])*GGML_FP16_TO_FP32(y[i]));
    }

    *s = sumf;
}

__attribute__((target("avx")))
static void ggml_vec_dot_f16_avx(int n, float * restrict s, size_t bs, ggml_fp16_t * restrict x, size_t bx, ggml_fp16_t * restrict y, size_t by, int nrc) {
    assert(nrc == 1);
    UNUSED(nrc);
    UNUSED(bx);
    UNUSED(by);
    UNUSED(bs);

    ggml_float sumf = 0.0;

    const int np = (n & ~(GGML_F16_AVX_STEP - 1));

    GGML_F16_AVX_VEC sum[GGML_F16_AVX_ARR] = { GGML_F16_AVX_VEC_ZERO };

    GGML_F16_AVX_VEC ax[GGML_F16_AVX_ARR];
    GGML_F16_AVX_VEC ay[GGML_F16_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX_STEP) {
        for (int j = 0; j < GGML_F16_AVX_ARR; j++) {
            ax[j] = GGML_F16_AVX_VEC_LOAD(x + i + j*GGML_F16_AVX_EPR, j);
            ay[j] = GGML_F16_AVX_VEC_LOAD(y + i + j*GGML_F16_AVX_EPR, j);

            sum[j] = GGML_F16_AVX_VEC_FMA(sum[j], ax[j], ay[j]);
        }
    }

    // reduce sum0..sum3 to sum0
    GGML_F16_AVX_VEC_REDUCE(sumf, sum);

    // leftovers
    for (int i = np; i < n; ++i) {
        sumf += (ggml_float)(GGML_FP16_TO_FP32(x[i])*GGML_FP16_TO_FP32(y[i]));
    }

    *s = sumf;
}

__attribute__((target("sse3,fma")))
static void ggml_vec_dot_f16_sse3_fma(int n, float * restrict s, size_t bs, ggml_fp16_t * restrict x, size_t bx, ggml_fp16_t * restrict y, size_t by, int nrc) {
    assert(nrc == 1);
    UNUSED(nrc);
    UNUSED(bx);
    UNUSED(by);
    UNUSED(bs);

    ggml_float sumf = 0.0;

    const int np = (n & ~(GGML_F16_SSE_STEP - 1));

    GGML_F16_SSE_VEC sum[GGML_F16_SSE_ARR] = { GGML_F16_SSE_VEC_ZERO };

    GGML_F16_SSE_VEC ax[GGML_F16_SSE_ARR];
    GGML_F16_SSE_VEC ay[GGML_F16_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F16_SSE_STEP) {
        for (int j = 0; j < GGML_F16_SSE_ARR; j++) {
            ax[j] = GGML_F16_SSE_VEC_LOAD(x + i + j*GGML_F16_SSE_EPR, j);
            ay[j] = GGML_F16_SSE_VEC_LOAD(y + i + j*GGML_F16_SSE_EPR, j);

            sum[j] = GGML_F16_SSE_VEC_FMA2(sum[j], ax[j], ay[j]);
        }
    }

    // reduce sum0..sum3 to sum0
    GGML_F16_SSE_VEC_REDUCE(sumf, sum);

    // leftovers
    for (int i = np; i < n; ++i) {
        sumf += (ggml_float)(GGML_FP16_TO_FP32(x[i])*GGML_FP16_TO_FP32(y[i]));
    }

    *s = sumf;
}

__attribute__((target("sse3")))
static void ggml_vec_dot_f16_sse3(int n, float * restrict s, size_t bs, ggml_fp16_t * restrict x, size_t bx, ggml_fp16_t * restrict y, size_t by, int nrc) {
    assert(nrc == 1);
    UNUSED(nrc);
    UNUSED(bx);
    UNUSED(by);
    UNUSED(bs);

    ggml_float sumf = 0.0;

    const int np = (n & ~(GGML_F16_SSE_STEP - 1));

    GGML_F16_SSE_VEC sum[GGML_F16_SSE_ARR] = { GGML_F16_SSE_VEC_ZERO };

    GGML_F16_SSE_VEC ax[GGML_F16_SSE_ARR];
    GGML_F16_SSE_VEC ay[GGML_F16_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F16_SSE_STEP) {
        for (int j = 0; j < GGML_F16_SSE_ARR; j++) {
            ax[j] = GGML_F16_SSE_VEC_LOAD(x + i + j*GGML_F16_SSE_EPR, j);
            ay[j] = GGML_F16_SSE_VEC_LOAD(y + i + j*GGML_F16_SSE_EPR, j);

            sum[j] = GGML_F16_SSE_VEC_FMA(sum[j], ax[j], ay[j]);
        }
    }

    // reduce sum0..sum3 to sum0
    GGML_F16_SSE_VEC_REDUCE(sumf, sum);

    // leftovers
    for (int i = np; i < n; ++i) {
        sumf += (ggml_float)(GGML_FP16_TO_FP32(x[i])*GGML_FP16_TO_FP32(y[i]));
    }

    *s = sumf;
}

#endif




#ifdef DYNAMIC_ARCH
__attribute__((target("avx512f")))
inline static void ggml_vec_dot_f16_unroll_avx512(const int n, const int xs, float * restrict s, void * restrict xv, ggml_fp16_t * restrict y) {
    ggml_float sumf[GGML_VEC_DOT_UNROLL] = { 0.0 };

    ggml_fp16_t * restrict x[GGML_VEC_DOT_UNROLL];

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        x[i] = (ggml_fp16_t *) ((char *) xv + i*xs);
    }

    const int np = (n & ~(GGML_F16_AVX512_STEP - 1));

    GGML_F16_AVX512_VEC sum[GGML_VEC_DOT_UNROLL][GGML_F16_AVX512_ARR] = { { GGML_F16_AVX512_VEC_ZERO } };

    GGML_F16_AVX512_VEC ax[GGML_F16_AVX512_ARR];
    GGML_F16_AVX512_VEC ay[GGML_F16_AVX512_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX512_STEP) {
        for (int j = 0; j < GGML_F16_AVX512_ARR; j++) {
            ay[j] = GGML_F16_AVX512_VEC_LOAD(y + i + j*GGML_F16_AVX512_EPR, j);

            for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
                ax[j] = GGML_F16_AVX512_VEC_LOAD(x[k] + i + j*GGML_F16_AVX512_EPR, j);

                sum[k][j] = GGML_F16_AVX512_VEC_FMA(sum[k][j], ax[j], ay[j]);
            }
        }
    }

    // reduce sum0..sum3 to sum0
    for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
        GGML_F16_AVX512_VEC_REDUCE(sumf[k], sum[k]);
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        for (int j = 0; j < GGML_VEC_DOT_UNROLL; ++j) {
            sumf[j] += (ggml_float)(GGML_FP16_TO_FP32(x[j][i])*GGML_FP16_TO_FP32(y[i]));
        }
    }

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        s[i] = sumf[i];
    }
}

__attribute__((target("avx,fma,f16c")))
inline static void ggml_vec_dot_f16_unroll_avx_fma_f16c(const int n, const int xs, float * restrict s, void * restrict xv, ggml_fp16_t * restrict y) {
    ggml_float sumf[GGML_VEC_DOT_UNROLL] = { 0.0 };

    ggml_fp16_t * restrict x[GGML_VEC_DOT_UNROLL];

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        x[i] = (ggml_fp16_t *) ((char *) xv + i*xs);
    }

    const int np = (n & ~(GGML_F16_AVX_STEP - 1));

    GGML_F16_AVX_VEC sum[GGML_VEC_DOT_UNROLL][GGML_F16_AVX_ARR] = { { GGML_F16_AVX_VEC_ZERO } };

    GGML_F16_AVX_VEC ax[GGML_F16_AVX_ARR];
    GGML_F16_AVX_VEC ay[GGML_F16_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX_STEP) {
        for (int j = 0; j < GGML_F16_AVX_ARR; j++) {
            ay[j] = GGML_F16_AVX_VEC_LOAD2(y + i + j*GGML_F16_AVX_EPR, j);

            for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
                ax[j] = GGML_F16_AVX_VEC_LOAD2(x[k] + i + j*GGML_F16_AVX_EPR, j);

                sum[k][j] = GGML_F16_AVX_VEC_FMA2(sum[k][j], ax[j], ay[j]);
            }
        }
    }

    // reduce sum0..sum3 to sum0
    for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
        GGML_F16_AVX_VEC_REDUCE(sumf[k], sum[k]);
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        for (int j = 0; j < GGML_VEC_DOT_UNROLL; ++j) {
            sumf[j] += (ggml_float)(GGML_FP16_TO_FP32(x[j][i])*GGML_FP16_TO_FP32(y[i]));
        }
    }

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        s[i] = sumf[i];
    }
}

__attribute__((target("avx,fma")))
inline static void ggml_vec_dot_f16_unroll_avx_fma(const int n, const int xs, float * restrict s, void * restrict xv, ggml_fp16_t * restrict y) {
    ggml_float sumf[GGML_VEC_DOT_UNROLL] = { 0.0 };

    ggml_fp16_t * restrict x[GGML_VEC_DOT_UNROLL];

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        x[i] = (ggml_fp16_t *) ((char *) xv + i*xs);
    }

    const int np = (n & ~(GGML_F16_AVX_STEP - 1));

    GGML_F16_AVX_VEC sum[GGML_VEC_DOT_UNROLL][GGML_F16_AVX_ARR] = { { GGML_F16_AVX_VEC_ZERO } };

    GGML_F16_AVX_VEC ax[GGML_F16_AVX_ARR];
    GGML_F16_AVX_VEC ay[GGML_F16_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX_STEP) {
        for (int j = 0; j < GGML_F16_AVX_ARR; j++) {
            ay[j] = GGML_F16_AVX_VEC_LOAD(y + i + j*GGML_F16_AVX_EPR, j);

            for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
                ax[j] = GGML_F16_AVX_VEC_LOAD(x[k] + i + j*GGML_F16_AVX_EPR, j);

                sum[k][j] = GGML_F16_AVX_VEC_FMA2(sum[k][j], ax[j], ay[j]);
            }
        }
    }

    // reduce sum0..sum3 to sum0
    for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
        GGML_F16_AVX_VEC_REDUCE(sumf[k], sum[k]);
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        for (int j = 0; j < GGML_VEC_DOT_UNROLL; ++j) {
            sumf[j] += (ggml_float)(GGML_FP16_TO_FP32(x[j][i])*GGML_FP16_TO_FP32(y[i]));
        }
    }

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        s[i] = sumf[i];
    }
}

__attribute__((target("avx,f16c")))
inline static void ggml_vec_dot_f16_unroll_avx_f16c(const int n, const int xs, float * restrict s, void * restrict xv, ggml_fp16_t * restrict y) {
    ggml_float sumf[GGML_VEC_DOT_UNROLL] = { 0.0 };

    ggml_fp16_t * restrict x[GGML_VEC_DOT_UNROLL];

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        x[i] = (ggml_fp16_t *) ((char *) xv + i*xs);
    }

    const int np = (n & ~(GGML_F16_AVX_STEP - 1));

    GGML_F16_AVX_VEC sum[GGML_VEC_DOT_UNROLL][GGML_F16_AVX_ARR] = { { GGML_F16_AVX_VEC_ZERO } };

    GGML_F16_AVX_VEC ax[GGML_F16_AVX_ARR];
    GGML_F16_AVX_VEC ay[GGML_F16_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX_STEP) {
        for (int j = 0; j < GGML_F16_AVX_ARR; j++) {
            ay[j] = GGML_F16_AVX_VEC_LOAD2(y + i + j*GGML_F16_AVX_EPR, j);

            for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
                ax[j] = GGML_F16_AVX_VEC_LOAD2(x[k] + i + j*GGML_F16_AVX_EPR, j);

                sum[k][j] = GGML_F16_AVX_VEC_FMA(sum[k][j], ax[j], ay[j]);
            }
        }
    }

    // reduce sum0..sum3 to sum0
    for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
        GGML_F16_AVX_VEC_REDUCE(sumf[k], sum[k]);
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        for (int j = 0; j < GGML_VEC_DOT_UNROLL; ++j) {
            sumf[j] += (ggml_float)(GGML_FP16_TO_FP32(x[j][i])*GGML_FP16_TO_FP32(y[i]));
        }
    }

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        s[i] = sumf[i];
    }
}

__attribute__((target("avx")))
inline static void ggml_vec_dot_f16_unroll_avx(const int n, const int xs, float * restrict s, void * restrict xv, ggml_fp16_t * restrict y) {
    ggml_float sumf[GGML_VEC_DOT_UNROLL] = { 0.0 };

    ggml_fp16_t * restrict x[GGML_VEC_DOT_UNROLL];

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        x[i] = (ggml_fp16_t *) ((char *) xv + i*xs);
    }

    const int np = (n & ~(GGML_F16_AVX_STEP - 1));

    GGML_F16_AVX_VEC sum[GGML_VEC_DOT_UNROLL][GGML_F16_AVX_ARR] = { { GGML_F16_AVX_VEC_ZERO } };

    GGML_F16_AVX_VEC ax[GGML_F16_AVX_ARR];
    GGML_F16_AVX_VEC ay[GGML_F16_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX_STEP) {
        for (int j = 0; j < GGML_F16_AVX_ARR; j++) {
            ay[j] = GGML_F16_AVX_VEC_LOAD(y + i + j*GGML_F16_AVX_EPR, j);

            for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
                ax[j] = GGML_F16_AVX_VEC_LOAD(x[k] + i + j*GGML_F16_AVX_EPR, j);

                sum[k][j] = GGML_F16_AVX_VEC_FMA(sum[k][j], ax[j], ay[j]);
            }
        }
    }

    // reduce sum0..sum3 to sum0
    for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
        GGML_F16_AVX_VEC_REDUCE(sumf[k], sum[k]);
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        for (int j = 0; j < GGML_VEC_DOT_UNROLL; ++j) {
            sumf[j] += (ggml_float)(GGML_FP16_TO_FP32(x[j][i])*GGML_FP16_TO_FP32(y[i]));
        }
    }

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        s[i] = sumf[i];
    }
}

__attribute__((target("sse3,fma")))
inline static void ggml_vec_dot_f16_unroll_sse3_fma(const int n, const int xs, float * restrict s, void * restrict xv, ggml_fp16_t * restrict y) {
    ggml_float sumf[GGML_VEC_DOT_UNROLL] = { 0.0 };

    ggml_fp16_t * restrict x[GGML_VEC_DOT_UNROLL];

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        x[i] = (ggml_fp16_t *) ((char *) xv + i*xs);
    }

    const int np = (n & ~(GGML_F16_SSE_STEP - 1));

    GGML_F16_SSE_VEC sum[GGML_VEC_DOT_UNROLL][GGML_F16_SSE_ARR] = { { GGML_F16_SSE_VEC_ZERO } };

    GGML_F16_SSE_VEC ax[GGML_F16_SSE_ARR];
    GGML_F16_SSE_VEC ay[GGML_F16_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F16_SSE_STEP) {
        for (int j = 0; j < GGML_F16_SSE_ARR; j++) {
            ay[j] = GGML_F16_SSE_VEC_LOAD(y + i + j*GGML_F16_SSE_EPR, j);

            for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
                ax[j] = GGML_F16_SSE_VEC_LOAD(x[k] + i + j*GGML_F16_SSE_EPR, j);

                sum[k][j] = GGML_F16_SSE_VEC_FMA2(sum[k][j], ax[j], ay[j]);
            }
        }
    }

    // reduce sum0..sum3 to sum0
    for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
        GGML_F16_SSE_VEC_REDUCE(sumf[k], sum[k]);
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        for (int j = 0; j < GGML_VEC_DOT_UNROLL; ++j) {
            sumf[j] += (ggml_float)(GGML_FP16_TO_FP32(x[j][i])*GGML_FP16_TO_FP32(y[i]));
        }
    }

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        s[i] = sumf[i];
    }
}

__attribute__((target("sse3")))
inline static void ggml_vec_dot_f16_unroll_sse3(const int n, const int xs, float * restrict s, void * restrict xv, ggml_fp16_t * restrict y) {
    ggml_float sumf[GGML_VEC_DOT_UNROLL] = { 0.0 };

    ggml_fp16_t * restrict x[GGML_VEC_DOT_UNROLL];

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        x[i] = (ggml_fp16_t *) ((char *) xv + i*xs);
    }

    const int np = (n & ~(GGML_F16_SSE_STEP - 1));

    GGML_F16_SSE_VEC sum[GGML_VEC_DOT_UNROLL][GGML_F16_SSE_ARR] = { { GGML_F16_SSE_VEC_ZERO } };

    GGML_F16_SSE_VEC ax[GGML_F16_SSE_ARR];
    GGML_F16_SSE_VEC ay[GGML_F16_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F16_SSE_STEP) {
        for (int j = 0; j < GGML_F16_SSE_ARR; j++) {
            ay[j] = GGML_F16_SSE_VEC_LOAD(y + i + j*GGML_F16_SSE_EPR, j);

            for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
                ax[j] = GGML_F16_SSE_VEC_LOAD(x[k] + i + j*GGML_F16_SSE_EPR, j);

                sum[k][j] = GGML_F16_SSE_VEC_FMA(sum[k][j], ax[j], ay[j]);
            }
        }
    }

    // reduce sum0..sum3 to sum0
    for (int k = 0; k < GGML_VEC_DOT_UNROLL; ++k) {
        GGML_F16_SSE_VEC_REDUCE(sumf[k], sum[k]);
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        for (int j = 0; j < GGML_VEC_DOT_UNROLL; ++j) {
            sumf[j] += (ggml_float)(GGML_FP16_TO_FP32(x[j][i])*GGML_FP16_TO_FP32(y[i]));
        }
    }

    for (int i = 0; i < GGML_VEC_DOT_UNROLL; ++i) {
        s[i] = sumf[i];
    }
}
#endif



#ifdef DYNAMIC_ARCH
__attribute__((target("avx512f")))
inline static void ggml_vec_mad_f32_avx512(const int n, float * restrict y, const float * restrict x, const float v) {
    const int np = (n & ~(GGML_F32_AVX512_STEP - 1));

    GGML_F32_AVX512_VEC vx = GGML_F32_AVX512_VEC_SET1(v);

    GGML_F32_AVX512_VEC ax[GGML_F32_AVX512_ARR];
    GGML_F32_AVX512_VEC ay[GGML_F32_AVX512_ARR];

    for (int i = 0; i < np; i += GGML_F32_AVX512_STEP) {
        for (int j = 0; j < GGML_F32_AVX512_ARR; j++) {
            ax[j] = GGML_F32_AVX512_VEC_LOAD(x + i + j*GGML_F32_AVX512_EPR);
            ay[j] = GGML_F32_AVX512_VEC_LOAD(y + i + j*GGML_F32_AVX512_EPR);
            ay[j] = GGML_F32_AVX512_VEC_FMA(ay[j], ax[j], vx);

            GGML_F32_AVX512_VEC_STORE(y + i + j*GGML_F32_AVX512_EPR, ay[j]);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] += x[i]*v;
    }
}

__attribute__((target("avx,fma")))
inline static void ggml_vec_mad_f32_avx_fma(const int n, float * restrict y, const float * restrict x, const float v) {
    const int np = (n & ~(GGML_F32_AVX_STEP - 1));

    GGML_F32_AVX_VEC vx = GGML_F32_AVX_VEC_SET1(v);

    GGML_F32_AVX_VEC ax[GGML_F32_AVX_ARR];
    GGML_F32_AVX_VEC ay[GGML_F32_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F32_AVX_STEP) {
        for (int j = 0; j < GGML_F32_AVX_ARR; j++) {
            ax[j] = GGML_F32_AVX_VEC_LOAD(x + i + j*GGML_F32_AVX_EPR);
            ay[j] = GGML_F32_AVX_VEC_LOAD(y + i + j*GGML_F32_AVX_EPR);
            ay[j] = GGML_F32_AVX_VEC_FMA2(ay[j], ax[j], vx);

            GGML_F32_AVX_VEC_STORE(y + i + j*GGML_F32_AVX_EPR, ay[j]);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] += x[i]*v;
    }
}

__attribute__((target("avx")))
inline static void ggml_vec_mad_f32_avx(const int n, float * restrict y, const float * restrict x, const float v) {
    const int np = (n & ~(GGML_F32_AVX_STEP - 1));

    GGML_F32_AVX_VEC vx = GGML_F32_AVX_VEC_SET1(v);

    GGML_F32_AVX_VEC ax[GGML_F32_AVX_ARR];
    GGML_F32_AVX_VEC ay[GGML_F32_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F32_AVX_STEP) {
        for (int j = 0; j < GGML_F32_AVX_ARR; j++) {
            ax[j] = GGML_F32_AVX_VEC_LOAD(x + i + j*GGML_F32_AVX_EPR);
            ay[j] = GGML_F32_AVX_VEC_LOAD(y + i + j*GGML_F32_AVX_EPR);
            ay[j] = GGML_F32_AVX_VEC_FMA(ay[j], ax[j], vx);

            GGML_F32_AVX_VEC_STORE(y + i + j*GGML_F32_AVX_EPR, ay[j]);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] += x[i]*v;
    }
}

__attribute__((target("sse3,fma")))
inline static void ggml_vec_mad_f32_sse3_fma(const int n, float * restrict y, const float * restrict x, const float v) {
    const int np = (n & ~(GGML_F32_SSE_STEP - 1));

    GGML_F32_SSE_VEC vx = GGML_F32_SSE_VEC_SET1(v);

    GGML_F32_SSE_VEC ax[GGML_F32_SSE_ARR];
    GGML_F32_SSE_VEC ay[GGML_F32_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F32_SSE_STEP) {
        for (int j = 0; j < GGML_F32_SSE_ARR; j++) {
            ax[j] = GGML_F32_SSE_VEC_LOAD(x + i + j*GGML_F32_SSE_EPR);
            ay[j] = GGML_F32_SSE_VEC_LOAD(y + i + j*GGML_F32_SSE_EPR);
            ay[j] = GGML_F32_SSE_VEC_FMA2(ay[j], ax[j], vx);

            GGML_F32_SSE_VEC_STORE(y + i + j*GGML_F32_SSE_EPR, ay[j]);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] += x[i]*v;
    }
}

__attribute__((target("sse3")))
inline static void ggml_vec_mad_f32_sse3(const int n, float * restrict y, const float * restrict x, const float v) {
    const int np = (n & ~(GGML_F32_SSE_STEP - 1));

    GGML_F32_SSE_VEC vx = GGML_F32_SSE_VEC_SET1(v);

    GGML_F32_SSE_VEC ax[GGML_F32_SSE_ARR];
    GGML_F32_SSE_VEC ay[GGML_F32_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F32_SSE_STEP) {
        for (int j = 0; j < GGML_F32_SSE_ARR; j++) {
            ax[j] = GGML_F32_SSE_VEC_LOAD(x + i + j*GGML_F32_SSE_EPR);
            ay[j] = GGML_F32_SSE_VEC_LOAD(y + i + j*GGML_F32_SSE_EPR);
            ay[j] = GGML_F32_SSE_VEC_FMA(ay[j], ax[j], vx);

            GGML_F32_SSE_VEC_STORE(y + i + j*GGML_F32_SSE_EPR, ay[j]);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] += x[i]*v;
    }
}
#endif



#ifdef DYNAMIC_ARCH
__attribute__((target("avx512f")))
inline static void ggml_vec_mad_f32_unroll_avx512(const int n, const int xs, const int vs, float * restrict y, const float * restrict xv, const float * restrict vv) {

    const float * restrict x[GGML_VEC_MAD_UNROLL];
    const float * restrict v[GGML_VEC_MAD_UNROLL];

    for (int i = 0; i < GGML_VEC_MAD_UNROLL; ++i) {
        x[i] = (const float *) ((const char *) xv + i*xs);
        v[i] = (const float *) ((const char *) vv + i*vs);
    }

    const int np = (n & ~(GGML_F32_AVX512_STEP - 1));

    GGML_F32_AVX512_VEC vx[GGML_VEC_MAD_UNROLL];

    for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
        vx[k] = GGML_F32_AVX512_VEC_SET1(v[k][0]);
    }

    GGML_F32_AVX512_VEC ax[GGML_VEC_MAD_UNROLL][GGML_F32_AVX512_ARR];
    GGML_F32_AVX512_VEC ay[GGML_F32_AVX512_ARR];

    for (int i = 0; i < np; i += GGML_F32_AVX512_STEP) {
        for (int j = 0; j < GGML_F32_AVX512_ARR; j++) {
            ay[j] = GGML_F32_AVX512_VEC_LOAD(y + i + j*GGML_F32_AVX512_EPR);

            for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
                ax[k][j] = GGML_F32_AVX512_VEC_LOAD(x[k] + i + j*GGML_F32_AVX512_EPR);
                ay[j] = GGML_F32_AVX512_VEC_FMA(ay[j], ax[k][j], vx[k]);
            }

            GGML_F32_AVX512_VEC_STORE(y + i + j*GGML_F32_AVX512_EPR, ay[j]);
        }
    }

    // leftovers
    for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
        for (int i = np; i < n; ++i) {
            y[i] += x[k][i]*v[k][0];
        }
    }
}

__attribute__((target("avx,fma")))
inline static void ggml_vec_mad_f32_unroll_avx_fma(const int n, const int xs, const int vs, float * restrict y, const float * restrict xv, const float * restrict vv) {

    const float * restrict x[GGML_VEC_MAD_UNROLL];
    const float * restrict v[GGML_VEC_MAD_UNROLL];

    for (int i = 0; i < GGML_VEC_MAD_UNROLL; ++i) {
        x[i] = (const float *) ((const char *) xv + i*xs);
        v[i] = (const float *) ((const char *) vv + i*vs);
    }

    const int np = (n & ~(GGML_F32_AVX_STEP - 1));

    GGML_F32_AVX_VEC vx[GGML_VEC_MAD_UNROLL];

    for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
        vx[k] = GGML_F32_AVX_VEC_SET1(v[k][0]);
    }

    GGML_F32_AVX_VEC ax[GGML_VEC_MAD_UNROLL][GGML_F32_AVX_ARR];
    GGML_F32_AVX_VEC ay[GGML_F32_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F32_AVX_STEP) {
        for (int j = 0; j < GGML_F32_AVX_ARR; j++) {
            ay[j] = GGML_F32_AVX_VEC_LOAD(y + i + j*GGML_F32_AVX_EPR);

            for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
                ax[k][j] = GGML_F32_AVX_VEC_LOAD(x[k] + i + j*GGML_F32_AVX_EPR);
                ay[j] = GGML_F32_AVX_VEC_FMA2(ay[j], ax[k][j], vx[k]);
            }

            GGML_F32_AVX_VEC_STORE(y + i + j*GGML_F32_AVX_EPR, ay[j]);
        }
    }

    // leftovers
    for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
        for (int i = np; i < n; ++i) {
            y[i] += x[k][i]*v[k][0];
        }
    }
}

__attribute__((target("avx")))
inline static void ggml_vec_mad_f32_unroll_avx(const int n, const int xs, const int vs, float * restrict y, const float * restrict xv, const float * restrict vv) {

    const float * restrict x[GGML_VEC_MAD_UNROLL];
    const float * restrict v[GGML_VEC_MAD_UNROLL];

    for (int i = 0; i < GGML_VEC_MAD_UNROLL; ++i) {
        x[i] = (const float *) ((const char *) xv + i*xs);
        v[i] = (const float *) ((const char *) vv + i*vs);
    }

    const int np = (n & ~(GGML_F32_AVX_STEP - 1));

    GGML_F32_AVX_VEC vx[GGML_VEC_MAD_UNROLL];

    for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
        vx[k] = GGML_F32_AVX_VEC_SET1(v[k][0]);
    }

    GGML_F32_AVX_VEC ax[GGML_VEC_MAD_UNROLL][GGML_F32_AVX_ARR];
    GGML_F32_AVX_VEC ay[GGML_F32_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F32_AVX_STEP) {
        for (int j = 0; j < GGML_F32_AVX_ARR; j++) {
            ay[j] = GGML_F32_AVX_VEC_LOAD(y + i + j*GGML_F32_AVX_EPR);

            for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
                ax[k][j] = GGML_F32_AVX_VEC_LOAD(x[k] + i + j*GGML_F32_AVX_EPR);
                ay[j] = GGML_F32_AVX_VEC_FMA(ay[j], ax[k][j], vx[k]);
            }

            GGML_F32_AVX_VEC_STORE(y + i + j*GGML_F32_AVX_EPR, ay[j]);
        }
    }

    // leftovers
    for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
        for (int i = np; i < n; ++i) {
            y[i] += x[k][i]*v[k][0];
        }
    }
}

__attribute__((target("sse3,fma")))
inline static void ggml_vec_mad_f32_unroll_sse3_fma(const int n, const int xs, const int vs, float * restrict y, const float * restrict xv, const float * restrict vv) {

    const float * restrict x[GGML_VEC_MAD_UNROLL];
    const float * restrict v[GGML_VEC_MAD_UNROLL];

    for (int i = 0; i < GGML_VEC_MAD_UNROLL; ++i) {
        x[i] = (const float *) ((const char *) xv + i*xs);
        v[i] = (const float *) ((const char *) vv + i*vs);
    }

    const int np = (n & ~(GGML_F32_SSE_STEP - 1));

    GGML_F32_SSE_VEC vx[GGML_VEC_MAD_UNROLL];

    for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
        vx[k] = GGML_F32_SSE_VEC_SET1(v[k][0]);
    }

    GGML_F32_SSE_VEC ax[GGML_VEC_MAD_UNROLL][GGML_F32_SSE_ARR];
    GGML_F32_SSE_VEC ay[GGML_F32_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F32_SSE_STEP) {
        for (int j = 0; j < GGML_F32_SSE_ARR; j++) {
            ay[j] = GGML_F32_SSE_VEC_LOAD(y + i + j*GGML_F32_SSE_EPR);

            for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
                ax[k][j] = GGML_F32_SSE_VEC_LOAD(x[k] + i + j*GGML_F32_SSE_EPR);
                ay[j] = GGML_F32_SSE_VEC_FMA2(ay[j], ax[k][j], vx[k]);
            }

            GGML_F32_SSE_VEC_STORE(y + i + j*GGML_F32_SSE_EPR, ay[j]);
        }
    }

    // leftovers
    for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
        for (int i = np; i < n; ++i) {
            y[i] += x[k][i]*v[k][0];
        }
    }
}

__attribute__((target("sse3")))
inline static void ggml_vec_mad_f32_unroll_sse3(const int n, const int xs, const int vs, float * restrict y, const float * restrict xv, const float * restrict vv) {

    const float * restrict x[GGML_VEC_MAD_UNROLL];
    const float * restrict v[GGML_VEC_MAD_UNROLL];

    for (int i = 0; i < GGML_VEC_MAD_UNROLL; ++i) {
        x[i] = (const float *) ((const char *) xv + i*xs);
        v[i] = (const float *) ((const char *) vv + i*vs);
    }

    const int np = (n & ~(GGML_F32_SSE_STEP - 1));

    GGML_F32_SSE_VEC vx[GGML_VEC_MAD_UNROLL];

    for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
        vx[k] = GGML_F32_SSE_VEC_SET1(v[k][0]);
    }

    GGML_F32_SSE_VEC ax[GGML_VEC_MAD_UNROLL][GGML_F32_SSE_ARR];
    GGML_F32_SSE_VEC ay[GGML_F32_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F32_SSE_STEP) {
        for (int j = 0; j < GGML_F32_SSE_ARR; j++) {
            ay[j] = GGML_F32_SSE_VEC_LOAD(y + i + j*GGML_F32_SSE_EPR);

            for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
                ax[k][j] = GGML_F32_SSE_VEC_LOAD(x[k] + i + j*GGML_F32_SSE_EPR);
                ay[j] = GGML_F32_SSE_VEC_FMA(ay[j], ax[k][j], vx[k]);
            }

            GGML_F32_SSE_VEC_STORE(y + i + j*GGML_F32_SSE_EPR, ay[j]);
        }
    }

    // leftovers
    for (int k = 0; k < GGML_VEC_MAD_UNROLL; ++k) {
        for (int i = np; i < n; ++i) {
            y[i] += x[k][i]*v[k][0];
        }
    }
}
#endif


#ifdef DYNAMIC_ARCH
__attribute__((target("avx512f")))
inline static void ggml_vec_scale_f32_avx512(const int n, float * y, const float v) {
    const int np = (n & ~(GGML_F32_AVX512_STEP - 1));

    GGML_F32_AVX512_VEC vx = GGML_F32_AVX512_VEC_SET1(v);

    GGML_F32_AVX512_VEC ay[GGML_F32_AVX512_ARR];

    for (int i = 0; i < np; i += GGML_F32_AVX512_STEP) {
        for (int j = 0; j < GGML_F32_AVX512_ARR; j++) {
            ay[j] = GGML_F32_AVX512_VEC_LOAD(y + i + j*GGML_F32_AVX512_EPR);
            ay[j] = GGML_F32_AVX512_VEC_MUL(ay[j], vx);

            GGML_F32_AVX512_VEC_STORE(y + i + j*GGML_F32_AVX512_EPR, ay[j]);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] *= v;
    }
}

__attribute__((target("avx")))
inline static void ggml_vec_scale_f32_avx(const int n, float * y, const float v) {
    const int np = (n & ~(GGML_F32_AVX_STEP - 1));

    GGML_F32_AVX_VEC vx = GGML_F32_AVX_VEC_SET1(v);

    GGML_F32_AVX_VEC ay[GGML_F32_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F32_AVX_STEP) {
        for (int j = 0; j < GGML_F32_AVX_ARR; j++) {
            ay[j] = GGML_F32_AVX_VEC_LOAD(y + i + j*GGML_F32_AVX_EPR);
            ay[j] = GGML_F32_AVX_VEC_MUL(ay[j], vx);

            GGML_F32_AVX_VEC_STORE(y + i + j*GGML_F32_AVX_EPR, ay[j]);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] *= v;
    }
}

__attribute__((target("sse3")))
inline static void ggml_vec_scale_f32_sse3(const int n, float * y, const float v) {
    const int np = (n & ~(GGML_F32_SSE_STEP - 1));

    GGML_F32_SSE_VEC vx = GGML_F32_SSE_VEC_SET1(v);

    GGML_F32_SSE_VEC ay[GGML_F32_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F32_SSE_STEP) {
        for (int j = 0; j < GGML_F32_SSE_ARR; j++) {
            ay[j] = GGML_F32_SSE_VEC_LOAD(y + i + j*GGML_F32_SSE_EPR);
            ay[j] = GGML_F32_SSE_VEC_MUL(ay[j], vx);

            GGML_F32_SSE_VEC_STORE(y + i + j*GGML_F32_SSE_EPR, ay[j]);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] *= v;
    }
}
#endif

#ifdef DYNAMIC_ARCH
__attribute__((target("avx512f")))
inline static void ggml_vec_scale_f16_avx512(const int n, ggml_fp16_t * y, const float v) {
    const int np = (n & ~(GGML_F16_AVX512_STEP - 1));

    GGML_F16_AVX512_VEC vx = GGML_F16_AVX512_VEC_SET1(v);

    GGML_F16_AVX512_VEC ay[GGML_F16_AVX512_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX512_STEP) {
        for (int j = 0; j < GGML_F16_AVX512_ARR; j++) {
            ay[j] = GGML_F16_AVX512_VEC_LOAD(y + i + j*GGML_F16_AVX512_EPR, j);
            ay[j] = GGML_F16_AVX512_VEC_MUL(ay[j], vx);

            GGML_F16_AVX512_VEC_STORE(y + i + j*GGML_F16_AVX512_EPR, ay, j);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] = GGML_FP32_TO_FP16(GGML_FP16_TO_FP32(y[i])*v);
    }
}

__attribute__((target("avx")))
inline static void ggml_vec_scale_f16_avx(const int n, ggml_fp16_t * y, const float v) {
    const int np = (n & ~(GGML_F16_AVX_STEP - 1));

    GGML_F16_AVX_VEC vx = GGML_F16_AVX_VEC_SET1(v);

    GGML_F16_AVX_VEC ay[GGML_F16_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX_STEP) {
        for (int j = 0; j < GGML_F16_AVX_ARR; j++) {
            ay[j] = GGML_F16_AVX_VEC_LOAD(y + i + j*GGML_F16_AVX_EPR, j);
            ay[j] = GGML_F16_AVX_VEC_MUL(ay[j], vx);

            GGML_F16_AVX_VEC_STORE(y + i + j*GGML_F16_AVX_EPR, ay, j);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] = GGML_FP32_TO_FP16(GGML_FP16_TO_FP32(y[i])*v);
    }
}

__attribute__((target("sse3")))
inline static void ggml_vec_scale_f16_sse3(const int n, ggml_fp16_t * y, const float v) {
    const int np = (n & ~(GGML_F16_SSE_STEP - 1));

    GGML_F16_SSE_VEC vx = GGML_F16_SSE_VEC_SET1(v);

    GGML_F16_SSE_VEC ay[GGML_F16_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F16_SSE_STEP) {
        for (int j = 0; j < GGML_F16_SSE_ARR; j++) {
            ay[j] = GGML_F16_SSE_VEC_LOAD(y + i + j*GGML_F16_SSE_EPR, j);
            ay[j] = GGML_F16_SSE_VEC_MUL(ay[j], vx);

            GGML_F16_SSE_VEC_STORE(y + i + j*GGML_F16_SSE_EPR, ay, j);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] = GGML_FP32_TO_FP16(GGML_FP16_TO_FP32(y[i])*v);
    }
}

#endif

#ifdef DYNAMIC_ARCH
__attribute__((target("avx512f")))
inline static void ggml_vec_mad_f16_avx512(const int n, ggml_fp16_t * restrict y, const ggml_fp16_t * restrict x, const float v) {
    const int np = (n & ~(GGML_F16_AVX512_STEP - 1));

    GGML_F16_AVX512_VEC vx = GGML_F16_AVX512_VEC_SET1(v);

    GGML_F16_AVX512_VEC ax[GGML_F16_AVX512_ARR];
    GGML_F16_AVX512_VEC ay[GGML_F16_AVX512_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX512_STEP) {
        for (int j = 0; j < GGML_F16_AVX512_ARR; j++) {
            ax[j] = GGML_F16_AVX512_VEC_LOAD(x + i + j*GGML_F16_AVX512_EPR, j);
            ay[j] = GGML_F16_AVX512_VEC_LOAD(y + i + j*GGML_F16_AVX512_EPR, j);
            ay[j] = GGML_F16_AVX512_VEC_FMA(ay[j], ax[j], vx);

            GGML_F16_AVX512_VEC_STORE(y + i + j*GGML_F16_AVX512_EPR, ay, j);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] = GGML_FP32_TO_FP16(GGML_FP16_TO_FP32(y[i]) + GGML_FP16_TO_FP32(x[i])*v);
    }
}

__attribute__((target("avx,fma")))
inline static void ggml_vec_mad_f16_avx_fma(const int n, ggml_fp16_t * restrict y, const ggml_fp16_t * restrict x, const float v) {
    const int np = (n & ~(GGML_F16_AVX_STEP - 1));

    GGML_F16_AVX_VEC vx = GGML_F16_AVX_VEC_SET1(v);

    GGML_F16_AVX_VEC ax[GGML_F16_AVX_ARR];
    GGML_F16_AVX_VEC ay[GGML_F16_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX_STEP) {
        for (int j = 0; j < GGML_F16_AVX_ARR; j++) {
            ax[j] = GGML_F16_AVX_VEC_LOAD(x + i + j*GGML_F16_AVX_EPR, j);
            ay[j] = GGML_F16_AVX_VEC_LOAD(y + i + j*GGML_F16_AVX_EPR, j);
            ay[j] = GGML_F16_AVX_VEC_FMA2(ay[j], ax[j], vx);

            GGML_F16_AVX_VEC_STORE(y + i + j*GGML_F16_AVX_EPR, ay, j);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] = GGML_FP32_TO_FP16(GGML_FP16_TO_FP32(y[i]) + GGML_FP16_TO_FP32(x[i])*v);
    }
}

__attribute__((target("avx")))
inline static void ggml_vec_mad_f16_avx(const int n, ggml_fp16_t * restrict y, const ggml_fp16_t * restrict x, const float v) {
    const int np = (n & ~(GGML_F16_AVX_STEP - 1));

    GGML_F16_AVX_VEC vx = GGML_F16_AVX_VEC_SET1(v);

    GGML_F16_AVX_VEC ax[GGML_F16_AVX_ARR];
    GGML_F16_AVX_VEC ay[GGML_F16_AVX_ARR];

    for (int i = 0; i < np; i += GGML_F16_AVX_STEP) {
        for (int j = 0; j < GGML_F16_AVX_ARR; j++) {
            ax[j] = GGML_F16_AVX_VEC_LOAD(x + i + j*GGML_F16_AVX_EPR, j);
            ay[j] = GGML_F16_AVX_VEC_LOAD(y + i + j*GGML_F16_AVX_EPR, j);
            ay[j] = GGML_F16_AVX_VEC_FMA(ay[j], ax[j], vx);

            GGML_F16_AVX_VEC_STORE(y + i + j*GGML_F16_AVX_EPR, ay, j);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] = GGML_FP32_TO_FP16(GGML_FP16_TO_FP32(y[i]) + GGML_FP16_TO_FP32(x[i])*v);
    }
}

__attribute__((target("sse3,fma")))
inline static void ggml_vec_mad_f16_sse3_fma(const int n, ggml_fp16_t * restrict y, const ggml_fp16_t * restrict x, const float v) {
    const int np = (n & ~(GGML_F16_SSE_STEP - 1));

    GGML_F16_SSE_VEC vx = GGML_F16_SSE_VEC_SET1(v);

    GGML_F16_SSE_VEC ax[GGML_F16_SSE_ARR];
    GGML_F16_SSE_VEC ay[GGML_F16_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F16_SSE_STEP) {
        for (int j = 0; j < GGML_F16_SSE_ARR; j++) {
            ax[j] = GGML_F16_SSE_VEC_LOAD(x + i + j*GGML_F16_SSE_EPR, j);
            ay[j] = GGML_F16_SSE_VEC_LOAD(y + i + j*GGML_F16_SSE_EPR, j);
            ay[j] = GGML_F16_SSE_VEC_FMA2(ay[j], ax[j], vx);

            GGML_F16_SSE_VEC_STORE(y + i + j*GGML_F16_SSE_EPR, ay, j);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] = GGML_FP32_TO_FP16(GGML_FP16_TO_FP32(y[i]) + GGML_FP16_TO_FP32(x[i])*v);
    }
}

__attribute__((target("sse3")))
inline static void ggml_vec_mad_f16_sse3(const int n, ggml_fp16_t * restrict y, const ggml_fp16_t * restrict x, const float v) {
    const int np = (n & ~(GGML_F16_SSE_STEP - 1));

    GGML_F16_SSE_VEC vx = GGML_F16_SSE_VEC_SET1(v);

    GGML_F16_SSE_VEC ax[GGML_F16_SSE_ARR];
    GGML_F16_SSE_VEC ay[GGML_F16_SSE_ARR];

    for (int i = 0; i < np; i += GGML_F16_SSE_STEP) {
        for (int j = 0; j < GGML_F16_SSE_ARR; j++) {
            ax[j] = GGML_F16_SSE_VEC_LOAD(x + i + j*GGML_F16_SSE_EPR, j);
            ay[j] = GGML_F16_SSE_VEC_LOAD(y + i + j*GGML_F16_SSE_EPR, j);
            ay[j] = GGML_F16_SSE_VEC_FMA(ay[j], ax[j], vx);

            GGML_F16_SSE_VEC_STORE(y + i + j*GGML_F16_SSE_EPR, ay, j);
        }
    }

    // leftovers
    for (int i = np; i < n; ++i) {
        y[i] = GGML_FP32_TO_FP16(GGML_FP16_TO_FP32(y[i]) + GGML_FP16_TO_FP32(x[i])*v);
    }
}
#endif




#endif
