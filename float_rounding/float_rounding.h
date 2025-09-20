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

// roundf() - round towards nearest integer with halfway cases away from zero
// -3.5 -> -4.0
// -2.5 -> -3.0
//  2.5 ->  3.0
//  3.5 ->  4.0
static inline float FloatRound(float x);

// nearbyintf() - round towards nearest integer with halfway cases to even value
// -3.5 -> -4.0
// -2.5 -> -2.0
//  2.5 ->  2.0
//  3.5 ->  4.0
static inline float FloatNearbyInt(float x);

//
// implementation
//

#if defined(__wasm__)
#   define ROUNDING_WASM
#elif !defined(ROUNDING_NO_SIMD)
#   if defined(__x86_64__) && defined(__SSE4_1__)
#       define ROUNDING_SSE4      // on x64 SSE4.1 can mostly compile to a single "roundss" instruction
#   elif defined(_M_AMD64) && defined(_NO_PREFETCHW)
#       define ROUNDING_SSE4_MSVC // MSVC sets _NO_PREFETCHW macro when /arch:SSE4.2 or higher is enabled
#   elif defined(_M_AMD64) || defined(__x86_64__)
#       define ROUNDING_SSE2     // otherwise on x64 use baseline SSE2
#   elif defined(_M_ARM64) || defined(__aarch64__)
#       define ROUNDING_NEON
#   endif
#endif

#if defined(ROUNDING_SSE4_MSVC)
#   include <immintrin.h> // this is where __round_ss is for MSVC
#elif defined(ROUNDING_SSE4)
#   include <smmintrin.h>
#elif defined(ROUNDING_SSE2)
#   include <emmintrin.h>
#elif defined(ROUNDING_NEON)
#   include <arm_neon.h>
#endif

#if defined(__FAST_MATH__) || defined(_M_FP_FAST)
#   error this code will not work if -ffast-math or /fp:fast is used
#endif

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunused-function"
#endif

static inline float FloatAbs(float x)
{
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_fabsf(x);
#else
    // this is not a good way to implement this function, but it's here
    // only for "no simd" reference C code without math.h dependency
    union {
        float f;
        struct { unsigned int m:23, e:8, s:1; };
    } u = { x };
    u.s = 0;
    return u.f;
#endif
}

static inline float FloatCopySign(float x, float sign)
{
#if defined(__clang__) || defined(__GNUC__)
    return __builtin_copysignf(x, sign);
#else
    // this is not a good way to implement this function, but it's here
    // only for "no simd" reference C code without math.h dependency
    union {
        float f;
        struct { unsigned int m:23, e:8, s:1; };
    } ux = { x }, us = { sign };
    ux.s = us.s;
    return ux.f;
#endif
}

float FloatTrunc(float x)
{
#if defined(ROUNDING_WASM)

    return __builtin_truncf(x);

#elif defined(ROUNDING_NEON)

    return vget_lane_f32(vrnd_f32(vdup_n_f32(x)), 0);

#elif defined(ROUNDING_SSE4)

    __m128 y = _mm_set_ss(x);
    y = _mm_round_ss(_mm_undefined_ps(), y, 3);
    return _mm_cvtss_f32(y);

#elif defined(ROUNDING_SSE4_MSVC)

    return __round_ss(x, 3);

#elif defined(ROUNDING_SSE2)

    const __m128 kNoFraction = _mm_set_ss(0x1p+23f);
    const __m128 kSignBit = _mm_set_ss(-0.f);

    __m128 vx = _mm_set_ss(x);
    __m128 vt = _mm_andnot_ps(kSignBit, vx);
    __m128 vy = _mm_cvtepi32_ps(_mm_cvttps_epi32(vt));
    vy = _mm_or_ps(_mm_and_ps(kSignBit, vx), vy);

    __m128 mask = _mm_cmplt_ss(vt, kNoFraction);
    vy = _mm_or_ps(_mm_andnot_ps(mask, vx), _mm_and_ps(mask, vy));

    return _mm_cvtss_f32(vy);

#else

    const float kNoFraction = 0x1p+23f;

    float t = FloatAbs(x);
    float y = (float)(int)t;
    y = FloatCopySign(y, x);

    y = (t < kNoFraction) ? y : x;
    return y;

#endif
}

float FloatFloor(float x)
{
#if defined(ROUNDING_WASM)

    return __builtin_floorf(x);

#elif defined(ROUNDING_NEON)

    return vget_lane_f32(vrndm_f32(vdup_n_f32(x)), 0);

#elif defined(ROUNDING_SSE4)

    __m128 y = _mm_set_ss(x);
    y = _mm_round_ss(_mm_undefined_ps(), y, 1);
    return _mm_cvtss_f32(y);

#elif defined(ROUNDING_SSE4_MSVC)

    return __round_ss(x, 1);

#elif defined(ROUNDING_SSE2)

    const __m128 kNoFraction = _mm_set_ss(0x1p+23f);
    const __m128 kSignBit = _mm_set_ss(-0.f);

    __m128 vx = _mm_set_ss(x);
    __m128 vt = _mm_andnot_ps(kSignBit, vx);
    __m128 vy = _mm_cvtepi32_ps(_mm_cvttps_epi32(vx));
    vy = _mm_or_ps(_mm_and_ps(kSignBit, vx), vy);

    __m128 mask = _mm_cmplt_ss(vt, kNoFraction);
    vy = _mm_or_ps(_mm_andnot_ps(mask, vx), _mm_and_ps(mask, vy));

    vy = _mm_sub_ss(vy, _mm_and_ps(_mm_cmplt_ss(vx, vy), _mm_set_ss(1.f)));
    return _mm_cvtss_f32(vy);

#else

    const float kNoFraction = 0x1p+23f;

    float t = FloatAbs(x);
    float y = (float)(int)t;
    y = FloatCopySign(y, x);

    y = (t < kNoFraction) ? y : x;
    y -= (x < y) ? 1.f : 0.f;

    return y;

#endif
}

float FloatCeil(float x)
{
#if defined(ROUNDING_WASM)

    return __builtin_ceilf(x);

#elif defined(ROUNDING_NEON)

    return vget_lane_f32(vrndp_f32(vdup_n_f32(x)), 0);

#elif defined(ROUNDING_SSE4)

    __m128 y = _mm_set_ss(x);
    y = _mm_round_ss(_mm_undefined_ps(), y, 2);
    return _mm_cvtss_f32(y);

#elif defined(ROUNDING_SSE4_MSVC)

    return __round_ss(x, 2);

#elif defined(ROUNDING_SSE2)

    const __m128 kNoFraction = _mm_set_ss(0x1p+23f);
    const __m128 kSignBit = _mm_set_ss(-0.f);

    __m128 vx = _mm_set_ss(x);
    __m128 vt = _mm_andnot_ps(kSignBit, vx);
    __m128 vy = _mm_cvtepi32_ps(_mm_cvttps_epi32(vx));
    vy = _mm_or_ps(_mm_and_ps(kSignBit, vx), vy);

    __m128 mask = _mm_cmplt_ss(vt, kNoFraction);
    vy = _mm_or_ps(_mm_andnot_ps(mask, vx), _mm_and_ps(mask, vy));

    vy = _mm_sub_ss(vy, _mm_and_ps(_mm_cmpgt_ss(vx, vy), _mm_set_ss(-1.f)));
    return _mm_cvtss_f32(vy);

#else

    const float kNoFraction = 0x1p+23f;

    float t = FloatAbs(x);
    float y = (float)(int)x;
    y = FloatCopySign(y, x);

    y = (t < kNoFraction) ? y : x;
    y -= (x > y) ? -1.f : 0.f;

    return y;

#endif
}

float FloatRound(float x)
{
#if defined(ROUNDING_WASM)

    const float kHalfMinusOne = 0x1.fffffep-2f;

    x += __builtin_copysignf(kHalfMinusOne, x);
    return __builtin_truncf(x);

#elif defined(ROUNDING_NEON)

    return vget_lane_f32(vrnda_f32(vdup_n_f32(x)), 0);

#elif defined(ROUNDING_SSE4) || defined(ROUNDING_SSE4_MSVC)

    const __m128 kHalfMinusOne = _mm_set_ss(0x1.fffffep-2f);
    const __m128 kSignBit = _mm_set_ss(-0.f);

    __m128 vx = _mm_set_ss(x);
    __m128 vy = vx;

    vy = _mm_add_ss(vy, _mm_or_ps(_mm_and_ps(vx, kSignBit), kHalfMinusOne));
    vy = _mm_round_ss(vy, vy, 3);

    return _mm_cvtss_f32(vy);

#elif defined(ROUNDING_SSE2)

    const __m128 kNoFraction = _mm_set_ss(0x1p+23f);
    const __m128 kHalfMinusOne = _mm_set_ss(0x1.fffffep-2f);
    const __m128 kSignBit = _mm_set_ss(-0.f);

    __m128 vx = _mm_set_ss(x);
    __m128 vt = _mm_andnot_ps(kSignBit, vx);
    __m128 vy = vt;

    vy = _mm_add_ss(vy, kHalfMinusOne);
    vy = _mm_cvtepi32_ps(_mm_cvttps_epi32(vy));
    vy = _mm_or_ps(vy, _mm_and_ps(kSignBit, vx));

    __m128 mask = _mm_cmplt_ss(vt, kNoFraction);
    vy = _mm_or_ps(_mm_andnot_ps(mask, vx), _mm_and_ps(mask, vy));

    return _mm_cvtss_f32(vy);

#else

    const float kNoFraction = 0x1p+23f;
    const float kHalfMinusOne = 0x1.fffffep-2f;

    float t = FloatAbs(x);
    float y = t;

    y += kHalfMinusOne;
    y = (float)(int)y;
    y = FloatCopySign(y, x);

    y = (t < kNoFraction) ? y : x;
    return y;

#endif
}

float FloatNearbyInt(float x)
{
#if defined(ROUNDING_WASM)

    return __builtin_nearbyintf(x);

#elif defined(ROUNDING_NEON)

    return vget_lane_f32(vrndi_f32(vdup_n_f32(x)), 0);

#elif defined(ROUNDING_SSE4)

    __m128 y = _mm_set_ss(x);
    y = _mm_round_ss(_mm_undefined_ps(), y, 0);
    return _mm_cvtss_f32(y);

#elif defined(ROUNDING_SSE4_MSVC)

    return __round_ss(x, 0);

#elif defined(ROUNDING_SSE2)

    const __m128 kNoFraction = _mm_set_ss(0x1p+23f);
    const __m128 kSignBit = _mm_set_ss(-0.f);

    __m128 vx = _mm_set_ss(x);
    __m128 vt = _mm_andnot_ps(kSignBit, vx);
    __m128 vy = vt;

    vy = _mm_add_ss(vy, kNoFraction);
    vy = _mm_sub_ss(vy, kNoFraction);
    vy = _mm_or_ps(vy, _mm_and_ps(kSignBit, vx));

    __m128 mask = _mm_cmplt_ss(vt, kNoFraction);
    vy = _mm_or_ps(_mm_andnot_ps(mask, vx), _mm_and_ps(mask, vy));

    return _mm_cvtss_f32(vy);

#else

    const float kNoFraction = 0x1p+23f;

    float t = FloatAbs(x);
    float y = t;

    y += kNoFraction;
    y -= kNoFraction;
    y = FloatCopySign(y, x);

    y = (t < kNoFraction) ? y : x;
    return y;

#endif
}
