#pragma once

// floating point rounding implementations for sse2/sse4/neon/wasm
// provides truncf/floorf/ceilf/roundf/nearbyintf functionalty
// without C runtime math dependency

// code in this file depends that current rounding has NOT been changed
// it must remain with default mode = round to nearest

// all functions are correct for ALL float values
// including negative zeroes, subnormals, INFs, and NANs

// truncf() - truncate float towards zero
// -3.5 -> -3.0
// -2.5 -> -2.0
//  2.5 ->  2.0
//  3.5 ->  3.0
static inline float FloatTrunc(float x);

// floorf() - round down, towards negative infinity
// -3.5 -> -4.0
// -2.5 -> -3.0
//  2.5 ->  2.0
//  3.5 ->  3.0
static inline float FloatFloor(float x);

// ceilf() - round up, towards positive infinity
// -3.5 -> -3.0
// -2.5 -> -2.0
//  2.5 ->  3.0
//  3.5 ->  4.0
static inline float FloatCeil(float x);

// roundf() - round towards nearest integer with halfway cases away from zero (commercial rounding)
// -3.5 -> -4.0
// -2.5 -> -3.0
//  2.5 ->  3.0
//  3.5 ->  4.0
static inline float FloatRound(float x);

// nearbyintf() - round towards nearest integer with halfway cases to even value (bankers' rounding)
// -3.5 -> -4.0
// -2.5 -> -2.0
//  2.5 ->  2.0
//  3.5 ->  4.0
static inline float FloatNearbyInt(float x);

//
// implementation
//

#if !defined(ROUNDING_NO_SIMD) // do not use any SIMD intrinsics or compiler floating rounding builtins
#  if defined(__wasm__)
#    define ROUNDING_WASM
#  elif defined(__riscv) && __riscv_zfa >= 1000000  // assumes Zfa extension, which is mandatory in RVA23U64 profile
#    define ROUNDING_RISCV
#  elif defined(__aarch64__)
#    define ROUNDING_NEON
#  elif defined(_M_ARM64)
#    define ROUNDING_NEON_MSVC
#  elif defined(__x86_64__) && defined(__SSE4_1__)  // on x64 SSE4.1+ can mostly compile to a single "roundss" instruction
#    define ROUNDING_SSE4
#  elif defined(_M_AMD64) && defined(_NO_PREFETCHW) // MSVC sets _NO_PREFETCHW macro when /arch:SSE4.2 or higher is enabled
#    define ROUNDING_SSE4_MSVC
#  elif defined(_M_AMD64) || defined(__x86_64__)    // otherwise on x64 use baseline SSE2
#    define ROUNDING_SSE2
#  endif
#endif

#if defined(ROUNDING_SSE4_MSVC)
#  include <intrin.h> // __truncf, __floorf, __ceilf, __roundf, __nearbyintf, __copysignf
#elif defined(ROUNDING_NEON_MSVC)
#  include <arm_neon.h>
#  include <intrin.h> //__floorf, __ceilf
#elif defined(ROUNDING_SSE2)
#  include <emmintrin.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define ROUNDING_ABS(x)         __builtin_fabsf(x)
#  define ROUNDING_COPYSIGN(x, s) __builtin_copysignf(x, s)
#elif defined(_MSC_VER)
#  include <intrin.h>
#  define ROUNDING_ABS(x)         __copysignf(x, 0.f)
#  define ROUNDING_COPYSIGN(x, s) __copysignf(x, s)
#endif

#if defined(__FAST_MATH__) || defined(_M_FP_FAST)
#  error this code will not work correctly if -ffast-math or /fp:fast is used
#endif

float FloatTrunc(float x)
{
#if defined(ROUNDING_WASM) || defined(ROUNDING_SSE4) || defined(ROUNDING_NEON) || defined(ROUNDING_RISCV)

    return __builtin_truncf(x);

#elif defined(ROUNDING_SSE4_MSVC)

    return __truncf(x);

#elif defined(ROUNDING_NEON_MSVC)

    // __truncf on msvc/arm64 generates call to truncf()
    return vget_lane_f32(vrnd_f32(vdup_n_f32(x)), 0);

#elif defined(ROUNDING_SSE2)

    const __m128 kNoFraction = _mm_set_ss(0x1p+23f);
    const __m128 kSignBit = _mm_set_ss(-0.f);

    __m128 vx = _mm_set_ss(x);

    // absolute value
    __m128 va = _mm_andnot_ps(kSignBit, vx);

    // truncate to integer value
    __m128 vy = _mm_cvtepi32_ps(_mm_cvttps_epi32(va));

    // restore sign
    vy = _mm_or_ps(_mm_and_ps(kSignBit, vx), vy);

    // choose original value if there's no fraction, otherwise truncated value
    __m128 mask = _mm_cmplt_ss(va, kNoFraction);
    vy = _mm_or_ps(_mm_andnot_ps(mask, vx), _mm_and_ps(mask, vy));

    return _mm_cvtss_f32(vy);

#else

    const float kNoFraction = 0x1p+23f;

    // absolute value of input
    float a = ROUNDING_ABS(x);

    // if value is "small" then truncate it to integer via casting (no UB because value is small)
    // otherwise leave original value - it does not have fractional bits (because value is large)
    float y = (a < kNoFraction) ? ROUNDING_COPYSIGN((float)(int)a, x) : x;

    return y;

#endif
}

float FloatFloor(float x)
{
#if defined(ROUNDING_WASM) || defined(ROUNDING_SSE4) || defined(ROUNDING_NEON) || defined(ROUNDING_RISCV)

    return __builtin_floorf(x);

#elif defined(ROUNDING_SSE4_MSVC) || defined(ROUNDING_NEON_MSVC)

    return __floorf(x);

#elif defined(ROUNDING_SSE2)

    const __m128 kNoFraction = _mm_set_ss(0x1p+23f);
    const __m128 kSignBit = _mm_set_ss(-0.f);

    __m128 vx = _mm_set_ss(x);

    // absolute value
    __m128 va = _mm_andnot_ps(kSignBit, vx);

    // truncate to integer value
    __m128 vy = _mm_cvtepi32_ps(_mm_cvttps_epi32(vx));

    // restore sign
    vy = _mm_or_ps(_mm_and_ps(kSignBit, vx), vy);

    // choose original value if there's no fraction, otherwise truncated value
    __m128 mask = _mm_cmplt_ss(va, kNoFraction);
    vy = _mm_or_ps(_mm_andnot_ps(mask, vx), _mm_and_ps(mask, vy));

    // subtract 1 for negative values
    vy = _mm_sub_ss(vy, _mm_and_ps(_mm_cmplt_ss(vx, vy), _mm_set_ss(1.f)));

    return _mm_cvtss_f32(vy);

#else

    const float kNoFraction = 0x1p+23f;

    // absolute value
    float a = ROUNDING_ABS(x);

    // if value is "small" then truncate it to integer via casting (no UB because value is small)
    // otherwise leave original value - it does not have fractional bits (because value is large)
    float y = (a < kNoFraction) ? ROUNDING_COPYSIGN((float)(int)a, x) : x;

    // if original value x is smaller than y that means value was negative and it was not a whole
    // number and was truncated towards zero, thus need to subtract 1 to get floored result
    y -= (x < y) ? 1.f : 0.f;

    return y;

#endif
}

float FloatCeil(float x)
{
#if defined(ROUNDING_WASM) || defined(ROUNDING_SSE4) || defined(ROUNDING_NEON) || defined(ROUNDING_RISCV)

    return __builtin_ceilf(x);

#elif defined(ROUNDING_SSE4_MSVC) || defined(ROUNDING_NEON_MSVC)

    return __ceilf(x);

#elif defined(ROUNDING_SSE2)

    const __m128 kNoFraction = _mm_set_ss(0x1p+23f);
    const __m128 kSignBit = _mm_set_ss(-0.f);

    __m128 vx = _mm_set_ss(x);

    // absolute value
    __m128 va = _mm_andnot_ps(kSignBit, vx);

    // truncate to integer
    __m128 vy = _mm_cvtepi32_ps(_mm_cvttps_epi32(vx));

    // restore sign
    vy = _mm_or_ps(_mm_and_ps(kSignBit, vx), vy);

    // choose original value if there's no fraction, otherwise truncated value
    __m128 mask = _mm_cmplt_ss(va, kNoFraction);
    vy = _mm_or_ps(_mm_andnot_ps(mask, vx), _mm_and_ps(mask, vy));

    // subtract -1 for positive values
    vy = _mm_sub_ss(vy, _mm_and_ps(_mm_cmpgt_ss(vx, vy), _mm_set_ss(-1.f)));

    return _mm_cvtss_f32(vy);

#else

    const float kNoFraction = 0x1p+23f;

    // absolute value
    float a = ROUNDING_ABS(x);

    // if value is "small" then truncate it to integer via casting (no UB because value is small)
    // otherwise leave original value - it does not have fractional bits (because value is large)
    float y = (a < kNoFraction) ? ROUNDING_COPYSIGN((float)(int)a, x) : x;

    // if original value x is larger than y that means value was negative and it was not a whole
    // number and was truncated towards zero, thus need to add 1 to get floored result
    // the addition is done by subtracting -1 to preserve negative 0 result
    y -= (x > y) ? -1.f : 0.f;

    return y;

#endif
}

float FloatRound(float x)
{
#if defined(ROUNDING_WASM) || defined(ROUNDING_SSE4)

    // __builtin_roundf on wasm or gcc/sse4 generates call to roundf()
    const float kHalfMinusOne = 0x1.fffffep-2f;

    x += __builtin_copysignf(kHalfMinusOne, x);
    return __builtin_truncf(x);

#elif defined(ROUNDING_NEON) || defined(ROUNDING_RISCV)

    return __builtin_roundf(x);

#elif defined(ROUNDING_NEON_MSVC)
    
    // __roundf on msvc/arm64 generates call to roundf()
    return vget_lane_f32(vrnda_f32(vdup_n_f32(x)), 0);

#elif defined(ROUNDING_SSE4_MSVC)

    // __roundf on msvc/x64 generates call to roundf()
    const float kHalfMinusOne = 0x1.fffffep-2f;

    x += __copysignf(kHalfMinusOne, x);
    return __truncf(x);

#elif defined(ROUNDING_SSE2)

    const __m128 kNoFraction = _mm_set_ss(0x1p+23f);
    const __m128 kHalfMinusOne = _mm_set_ss(0x1.fffffep-2f);
    const __m128 kSignBit = _mm_set_ss(-0.f);

    __m128 vx = _mm_set_ss(x);

    // abssolute value
    __m128 va = _mm_andnot_ps(kSignBit, vx);
    __m128 vy = va;

    // add special value
    vy = _mm_add_ss(vy, kHalfMinusOne);

    // truncate to integer value
    vy = _mm_cvtepi32_ps(_mm_cvttps_epi32(vy));

    // restore sign
    vy = _mm_or_ps(vy, _mm_and_ps(kSignBit, vx));

    // choose original value if there's no fraction, otherwise truncated value
    __m128 mask = _mm_cmplt_ss(va, kNoFraction);
    vy = _mm_or_ps(_mm_andnot_ps(mask, vx), _mm_and_ps(mask, vy));

    return _mm_cvtss_f32(vy);

#else

    // rounding to nearest integer will be done by truncation
    // but before trunction kHalfMinusOne will be added
    // this means anything with fraction >= 0.5 will overflow to next integer
    // everything else with fraction < 0.5 will stay on current integer
    // thus implementing rounding half-way cases away from zero

    const float kNoFraction = 0x1p+23f;
    const float kHalfMinusOne = 0x1.fffffep-2f; // 0.5 minus 1 ulp

    // absolute value
    float a = ROUNDING_ABS(x);

    // if value is "small" then add special constant and truncate to integer via casting
    // otherwise leave original value - it does not have fractional bits (because value is large)
    float y = (a < kNoFraction) ? ROUNDING_COPYSIGN((float)(int)(a + kHalfMinusOne), x) : x;

    return y;

#endif
}

float FloatNearbyInt(float x)
{
#if defined(ROUNDING_WASM) || defined(ROUNDING_SSE4) || defined(ROUNDING_NEON) || defined(ROUNDING_RISCV)

    return __builtin_nearbyintf(x);

#elif defined(ROUNDING_SSE4_MSVC)

    return __nearbyintf(x);

#elif defined(ROUNDING_NEON_MSVC)

    // no __nearbyintf exists on msvc/arm64
    return vrndns_f32(x);

#elif defined(ROUNDING_SSE2)

    const __m128 kNoFraction = _mm_set_ss(0x1p+23f);
    const __m128 kSignBit = _mm_set_ss(-0.f);

    __m128 vx = _mm_set_ss(x);

    // absolute value
    __m128 va = _mm_andnot_ps(kSignBit, vx);
    __m128 vy = va;

    // force rounding to nearest integer
    vy = _mm_add_ss(vy, kNoFraction);
    vy = _mm_sub_ss(vy, kNoFraction);

    // restore sign
    vy = _mm_or_ps(vy, _mm_and_ps(kSignBit, vx));

    // choose original value if there's no fraction, otherwise rounded value
    __m128 mask = _mm_cmplt_ss(va, kNoFraction);
    vy = _mm_or_ps(_mm_andnot_ps(mask, vx), _mm_and_ps(mask, vy));

    return _mm_cvtss_f32(vy);

#else

    const float kNoFraction = 0x1p+23f;

    // absolute value
    float a = ROUNDING_ABS(x);
    float y = a;

    // force rounding to nearest integer
    // half-way cases will get rounded to even value, which is default rounding mode
    y += kNoFraction;
    y -= kNoFraction;

    // restore sign
    y = ROUNDING_COPYSIGN(y, x);

    // choose rounded value if there was fraction possible, otherwise leave original value
    y = (a < kNoFraction) ? y : x;

    return y;

#endif
}
