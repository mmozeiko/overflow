#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
  uint64_t lo, hi;
} x128;

static inline x128 x128_zero(void);

// set from 64-bit value
static inline x128 x128_set_u64(uint64_t x); // unsigned
static inline x128 x128_set_s64(int64_t  x); // signed

// get low 64-bit value, ignores top 64 bits
static inline uint64_t x128_get_u64(x128 x);
static inline int64_t  x128_get_s64(x128 x);

// make raw 128-bit value
static inline x128 x128_make(uint64_t lo, uint64_t hi);

// formatting to/from string, base = [2..36]
static inline x128   x128_set_str(const char* str, size_t len, size_t base);
static inline size_t x128_get_str_u(x128 x, char* str, size_t maxlen, size_t base); // signed
static inline size_t x128_get_str_s(x128 x, char* str, size_t maxlen, size_t base); // unsigned

// comparisons
static inline bool x128_is_zero(x128 x);
static inline bool x128_is_equal(x128 a, x128 b);
static inline bool x128_is_even(x128 x);
static inline bool x128_is_odd(x128 x);
static inline bool x128_is_neg(x128 x);        // signed, x <  0
static inline bool x128_is_pos(x128 x);        // signed, x >= 0
static inline int  x128_signum(x128 x);        // signed,   -1 or 0 or +1
static inline int  x128_cmp_u(x128 a, x128 b); // unsigned, -1 or 0 or +1
static inline int  x128_cmp_s(x128 a, x128 b); // signed,   -1 or 0 or +1

static inline x128 x128_min_u(x128 a, x128 b); // unsigned
static inline x128 x128_min_s(x128 a, x128 b); // signed
static inline x128 x128_max_u(x128 a, x128 b); // unsigned
static inline x128 x128_max_s(x128 a, x128 b); // signed

// bitwise
static inline x128 x128_not(x128 x);
static inline x128 x128_or (x128 a, x128 b);
static inline x128 x128_and(x128 a, x128 b);
static inline x128 x128_xor(x128 a, x128 b);
static inline x128 x128_shl(x128 x, size_t n);   //                 0 if n >= 128
static inline x128 x128_shr_u(x128 x, size_t n); // unsigned,       0 if n >= 128
static inline x128 x128_shr_s(x128 x, size_t n); // signed,   -1 or 0 if n >= 128

static inline size_t x128_clz(x128 x);    // 128 if x == 0
static inline size_t x128_ctz(x128 x);    // 128 if x == 0
static inline size_t x128_popcnt(x128 x); 

// arithmetic
static inline x128 x128_abs(x128 x); // signed
static inline x128 x128_neg(x128 x); // signed

static inline x128 x128_add(x128 a, x128 b);
static inline x128 x128_sub(x128 a, x128 b);

static inline x128 x128_mul_64x64 (uint64_t a, uint64_t b); // unsigned
static inline x128 x128_mul_128x64(    x128 a, uint64_t b); // unsigned
static inline x128 x128_mul       (    x128 a,     x128 b);

static inline bool x128_div_u(x128 x, x128 d, x128* q, x128* r); // unsigned, false if d == 0
static inline bool x128_div_s(x128 x, x128 d, x128* q, x128* r); // signed,   false if d == 0. r sign will be same as dividend x

// unsigned division by non-zero 64-bit divisor
static inline uint64_t x128_div_u64_reciprocal(uint64_t d, size_t* shift);
static inline uint64_t x128_div_u64(x128 x, uint64_t d, uint64_t reciprocal, size_t shift, x128* q);

// implementation

#if defined(__clang__)
#	define X128_CLANG 1
#elif defined(__GNUC__)
#	define X128_GCC 1
#elif defined(_MSC_VER)
#	define X128_MSVC 1
#   include <intrin.h>
#else
#	error unknown compiler
#endif

#if defined(__x86_64__) || defined(_M_AMD64)
#	define X128_X64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#	define X128_ARM64 1
#   include <arm_neon.h>
#endif

#if !defined(X128_NO_INT128)
#   ifdef __SIZEOF_INT128__
#       define X128_INT128 1
#       define X128_FULL_S(x) ( (((         __int128)(x).hi) << 64) | (x).lo )
#       define X128_FULL_U(x) ( (((unsigned __int128)(x).hi) << 64) | (x).lo )
#   endif
#endif

static inline uint32_t x128__clz64(uint64_t x)
{
#if X128_MSVC
    unsigned long n;
    _BitScanReverse64(&n, x);
    return x ? n ^ 63 : 64;
#else
    return x ? __builtin_clzll(x) : 64;
#endif
}

static inline uint32_t x128__ctz64(uint64_t x)
{
#if X128_MSVC
    unsigned long n;
    return _BitScanForward64(&n, x) ? n : 64;
#else
    return x ? __builtin_ctzll(x) : 64;
#endif
}

x128 x128_zero(void)
{
    x128 r = { 0, 0 };
    return r;
}

x128 x128_set_u64(uint64_t x)
{
    x128 r = { x, 0 };
    return r;
}

x128 x128_set_s64(int64_t x)
{
    x128 r = { (uint64_t)x, (uint64_t)(x >> 63) };
    return r;
}

uint64_t x128_get_u64(x128 x)
{
    return x.lo;
}

int64_t x128_get_s64(x128 x)
{
    return (int64_t)x.lo;
}

x128 x128_make(uint64_t lo, uint64_t hi)
{
    x128 r = { lo, hi };
    return r;
}

x128 x128_set_str(const char* str, size_t len, size_t base)
{
    x128 x = x128_zero();
    bool negative = false;

    if (str[0] == '-')
    {
        negative = true;
        str++;
        if (len)
        {
            len--;
        }
    }

    size_t maxlen = len;

    for (;;)
    {
        if (maxlen && len-- == 0)
        {
            break;
        }

        char ch = *str++;
        if (ch == 0)
        {
            break;
        }

        uint64_t digit = (ch >= '0' && ch <= '9') ? (ch - '0')
                       : (ch >= 'a' && ch <= 'z') ? (ch - 'a' + 10)
                       : (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 10)
                       : 0;
        x = x128_mul_128x64(x, base);
        x = x128_add(x, x128_set_u64(digit));
    }

    return negative ? x128_neg(x) : x;
}

size_t x128_get_str_u(x128 x, char* str, size_t maxlen, size_t base)
{
    char digits[128];
    size_t len = 0;

    if (x128_is_zero(x))
    {
        digits[len++] = '0';
    }
    else
    {
        size_t shift;
        uint64_t inv = x128_div_u64_reciprocal(base, &shift);

        do
        {
            uint64_t digit = x128_div_u64(x, base, inv, shift, &x);
            digits[len++] = (char)((digit < 10 ? '0' : 'a' - 10) + digit);
        }
        while (!x128_is_zero(x));
    }

    for (size_t i=0; i<len && i<maxlen; i++)
    {
        str[i] = digits[len-1-i];
    }
    if (len < maxlen)
    {
        str[len] = 0;
    }
    return len;
}

size_t x128_get_str_s(x128 x, char* str, size_t maxlen, size_t base)
{   
    size_t ret = 0;

    if (x128_is_neg(x))
    {
        if (maxlen)
        {
            *str++ = '-';
            maxlen--;
        }
        ret++;
        x = x128_abs(x);
    }

    return ret + x128_get_str_u(x, str, maxlen, base);
}

bool x128_is_zero(x128 x)
{
    return (x.lo | x.hi) == 0;
}

bool x128_is_equal(x128 a, x128 b)
{
    return ((a.lo ^ b.lo) | (a.hi ^ b.hi)) == 0;
}

bool x128_is_neg(x128 x)
{
    return !!(x.hi >> 63);
}

bool x128_is_pos(x128 x)
{
    return !(x.hi >> 63);
}

bool x128_is_even(x128 x)
{
    return (x.lo & 1) == 0;
}

bool x128_is_odd(x128 x)
{
    return (x.lo & 1) != 0;
}

int x128_signum(x128 x)
{
    uint64_t r = (uint64_t)((int64_t)x.hi >> 63);
    r |= 1;
    return x.hi | x.lo ? (int)r : 0;
}

int x128_cmp_u(x128 a, x128 b)
{
#if X128_INT128

    unsigned __int128 aval = X128_FULL_U(a);
    unsigned __int128 bval = X128_FULL_U(b);

    return (aval > bval) - (aval < bval);

#else

    int lo = (a.lo > b.lo) - (a.lo < b.lo);
    int hi = (a.hi > b.hi) - (a.hi < b.hi);

    return hi ? hi : lo;

#endif
}

int x128_cmp_s(x128 a, x128 b)
{
#if X128_INT128

    __int128 aval = X128_FULL_S(a);
    __int128 bval = X128_FULL_S(b);

    return (aval > bval) - (aval < bval);

#else

    int an = a.hi >> 63;
    int bn = b.hi >> 63;

    int r = x128_cmp_u(a, b);
    int n = bn - an;
    return n ? n : r;

#endif
}

x128 x128_min_u(x128 a, x128 b)
{
    return x128_cmp_u(a, b) < 0 ? a : b;
}

x128 x128_min_s(x128 a, x128 b)
{
    return x128_cmp_s(a, b) < 0 ? a : b;
}

x128 x128_max_u(x128 a, x128 b)
{
    return x128_cmp_u(a, b) > 0 ? a : b;
}

x128 x128_max_s(x128 a, x128 b)
{
    return x128_cmp_s(a, b) > 0 ? a : b;
}

x128 x128_not(x128 x)
{
    return x128_make(~x.lo, ~x.hi);
}
    
x128 x128_or(x128 a, x128 b)
{
    return x128_make(a.lo | b.lo, a.hi | b.hi);
}

x128 x128_and(x128 a, x128 b)
{
    return x128_make(a.lo & b.lo, a.hi & b.hi);
}

x128 x128_xor(x128 a, x128 b)
{
    return x128_make(a.lo ^ b.lo, a.hi ^ b.hi);
}

x128 x128_shl(x128 x, size_t n)
{   
    size_t full = n;
    n &= 127;

    uint64_t lo, hi;

#if X128_INT128

    unsigned __int128 r = X128_FULL_U(x) << n;
    lo = (uint64_t)r;
    hi = (uint64_t)(r >> 64);

#elif X128_MSVC && X128_X64

    hi = __shiftleft128(x.lo, x.hi, (uint8_t)n);
    lo = x.lo << n;

    hi = n < 64 ? hi : lo;
    lo = n < 64 ? lo : 0;

#else

    if (n == 0)
    {
        hi = x.hi;
        lo = x.lo;
    }
    else if (n < 64)
    {
        hi = (x.hi << n) | (x.lo >> (64 - n));
        lo = (x.lo << n);
    }
    else
    {
        hi = (x.lo << (n - 64));
        lo = 0;
    }

#endif

    lo = full < 128 ? lo : 0;
    hi = full < 128 ? hi : 0;

    return x128_make(lo, hi);
}

x128 x128_shr_u(x128 x, size_t n)
{
    size_t full = n;
    n &= 127;

    uint64_t lo, hi;

#if X128_INT128

    unsigned __int128 r = X128_FULL_U(x) >> n;
    lo = (uint64_t)r;
    hi = (uint64_t)(r >> 64);

#elif X128_MSVC && X128_X64

    lo = __shiftright128(x.lo, x.hi, (uint8_t)n);
    hi = x.hi >> n;

    lo = n < 64 ? lo : hi;
    hi = n < 64 ? hi : 0;

#else

    if (n == 0)
    {
        lo = x.lo;
        hi = x.hi;
    }
    else if (n < 64)
    {
        lo = (x.lo >> n) | (x.hi << (64 - n));
        hi = (x.hi >> n);
    }
    else
    {
        lo = (x.hi >> (n - 64));
        hi = 0;
    }

#endif

    lo = full < 128 ? lo : 0;
    hi = full < 128 ? hi : 0;

    return x128_make(lo, hi);
}

x128 x128_shr_s(x128 x, size_t n)
{
    size_t full = n;
    n &= 127;

    uint64_t lo, hi;

#if X128_INT128

    __int128 r = X128_FULL_S(x) >> n;
    lo = (uint64_t)r;
    hi = (uint64_t)(r >> 64);

#elif X128_MSVC && X128_X64

    lo = __shiftright128(x.lo, x.hi, (uint8_t)n);
    hi = (int64_t)x.hi >> n;

    uint64_t top = (int64_t)x.hi >> 63;
    lo = n < 64 ? lo : hi;
    hi = n < 64 ? hi : top;

#else

    if (n == 0)
    {
        lo = x.lo;
        hi = x.hi;
    }
    else if (n < 64)
    {
        lo = (         x.lo >> n) | (x.hi << (64 - n));
        hi = ((int64_t)x.hi >> n);
    }
    else
    {
        lo = ((int64_t)x.hi >> (n - 64));
        hi = ((int64_t)x.hi >> 63);
    }

#endif

    uint64_t topbits = (uint64_t)((int64_t)x.hi >> 63);
    lo = full < 128 ? lo : topbits;
    hi = full < 128 ? hi : topbits;

    return x128_make(lo, hi);
}

size_t x128_clz(x128 x)
{
    size_t lo = x128__clz64(x.lo);
    size_t hi = x128__clz64(x.hi);

    size_t r = lo + hi;
    return hi == 64 ? r : hi;
}

size_t x128_ctz(x128 x)
{
    size_t lo = x128__ctz64(x.lo);
    size_t hi = x128__ctz64(x.hi);

    size_t r = lo + hi;
    return lo == 64 ? r : lo;
}

size_t x128_popcnt(x128 x)
{
#if X128_ARM64

    uint64x2_t v = vcombine_u64(vcreate_u64(x.lo), vcreate_u64(x.hi));
    uint8x16_t r = vcntq_u8(vreinterpretq_u8_u64(v));
    return vaddvq_u8(r);

#else

    // Chapter 5. Counting Bits from "Hacker's Delight"
    // or https://nimrod.blog/posts/algorithms-behind-popcount/

    const uint64_t k1 = 0x5555555555555555;
    const uint64_t k2 = 0x3333333333333333;
    const uint64_t k3 = 0x0f0f0f0f0f0f0f0f;
    const uint64_t k4 = 0x0101010101010101;

    uint64_t lo = x.lo;
    lo -= (lo >> 1) & k1;
    lo = (lo & k2) + ((lo >> 2) & k2);
    lo = (lo + (lo >> 4)) & k3;
    lo = (lo * k4) >> 56;

    uint64_t hi = x.hi;
    hi -= (hi >> 1) & k1;
    hi = (hi & k2) + ((hi >> 2) & k2);
    hi = (hi + (hi >> 4)) & k3;
    hi = (hi * k4) >> 56;

    return lo + hi;

#endif
}

x128 x128_abs(x128 x)
{
    bool neg = x128_is_neg(x);
    x128 xn = x128_neg(x);
    x.lo = neg ? xn.lo : x.lo;
    x.hi = neg ? xn.hi : x.hi;
    return x;
}

x128 x128_neg(x128 x)
{
    return x128_sub(x128_zero(), x);
}

x128 x128_add(x128 a, x128 b)
{
    uint64_t lo, hi;

#if X128_INT128

    unsigned __int128 r = X128_FULL_U(a) + X128_FULL_U(b);
    lo = (uint64_t)r;
    hi = (uint64_t)(r >> 64);

#elif X128_MSVC && X128_X64

    unsigned char carry = 0;
    carry = _addcarry_u64(carry, a.lo, b.lo, &lo);
    carry = _addcarry_u64(carry, a.hi, b.hi, &hi);
    (void)carry;

#else

    lo = a.lo + b.lo;
    hi = a.hi + b.hi + (lo < a.lo);

#endif

    return x128_make(lo, hi);
}

x128 x128_sub(x128 a, x128 b)
{
    uint64_t lo, hi;

#if X128_INT128

    unsigned __int128 r = X128_FULL_U(a) - X128_FULL_U(b);
    lo = (uint64_t)r;
    hi = (uint64_t)(r >> 64);

#elif X128_MSVC && X128_X64

    unsigned char carry = 0;
    carry = _subborrow_u64(carry, a.lo, b.lo, &lo);
    carry = _subborrow_u64(carry, a.hi, b.hi, &hi);
    (void)carry;

#else

    lo = a.lo - b.lo;
    hi = a.hi - b.hi - (a.lo < b.lo);

#endif

    return x128_make(lo, hi);
}

x128 x128_mul_128x64(x128 a, uint64_t b)
{
    uint64_t lo, hi;

#if X128_INT128

    unsigned __int128 r = X128_FULL_U(a) * b;
    lo = (uint64_t)r;
    hi = (uint64_t)(r >> 64);

#elif X128_MSVC && X128_X64

    lo = _umul128(a.lo, b, &hi);
    hi += a.hi * b;

#elif X128_MSVC && X128_ARM64

    lo = a.lo * b;
    hi = __umulh(a.lo, b);
    hi += a.hi * b;

#else

    uint64_t a0 = (uint32_t)(a.lo >>  0);
    uint64_t a1 = (uint32_t)(a.lo >> 32);
    uint64_t a2 = (uint32_t)(a.hi >>  0);
    uint64_t a3 = (uint32_t)(a.hi >> 32);

    uint64_t b0 = (uint32_t)(b >>  0);
    uint64_t b1 = (uint32_t)(b >> 32);

    uint64_t r0, r1, r2, r3;

    //     a3 a2 a1 a0
    //           b1 b0
    //  --------------
    //           a0*b0
    //        a1*b0
    //     a2*b0
    //  a3*b0
    //        a0*b1
    //     a1*b1
    //  a2*b1
    //  --------------
    //     r3 r2 r1 r0

    r0 = a0 * b0;
    r1 = a1 * b0 + (r0 >> 32);
    r2 = a2 * b0 + (r1 >> 32);
    r3 = a3 * b0 + (r2 >> 32);
    r0 = (uint32_t)r0;
    r1 = (uint32_t)r1;
    r2 = (uint32_t)r2;

    r1 += a0 * b1;
    r2 += a1 * b1 + (r1 >> 32);
    r3 += a2 * b1 + (r2 >> 32);
    r1 = (uint32_t)r1;
    r2 = (uint32_t)r2;

    lo = r0 | (r1 << 32);
    hi = r2 | (r3 << 32);

#endif

    return x128_make(lo, hi);
}

x128 x128_mul_64x64(uint64_t a, uint64_t b)
{
    uint64_t lo, hi;

#if X128_INT128

    unsigned __int128 r = (unsigned __int128)a * b;
    lo = (uint64_t)r;
    hi = (uint64_t)(r >> 64);

#elif X128_MSVC && X128_X64

    lo = _umul128(a, b, &hi);

#elif X128_MSVC && X128_ARM64

    lo = a * b;
    hi = __umulh(a, b);

#else

    uint64_t a0 = (uint32_t)(a >>  0);
    uint64_t a1 = (uint32_t)(a >> 32);

    uint64_t b0 = (uint32_t)(b >>  0);
    uint64_t b1 = (uint32_t)(b >> 32);

    uint64_t r0, r1, r2, r3;

    //           a1 a0
    //           b1 b0
    //  --------------
    //           a0*b0
    //        a1*b0
    //        a0*b1
    //     a1*b1
    //  --------------
    //     r3 r2 r1 r0

    r0 = a0 * b0;
    r1 = a1 * b0 + (r0 >> 32);
    r2 = r1 >> 32;
    r3 = 0;
    r0 = (uint32_t)r0;
    r1 = (uint32_t)r1;

    r1 += a0 * b1;
    r2 += a1 * b1 + (r1 >> 32);
    r3 += r2 >> 32;
    r1 = (uint32_t)r1;
    r2 = (uint32_t)r2;

    lo = r0 | (r1 << 32);
    hi = r2 | (r3 << 32);

#endif

    return x128_make(lo, hi);
}

x128 x128_mul(x128 a, x128 b)
{
    uint64_t lo, hi;

#if X128_INT128

    unsigned __int128 r = X128_FULL_U(a) * X128_FULL_U(b);
    lo = (uint64_t)r;
    hi = (uint64_t)(r >> 64);

#elif X128_MSVC && X128_X64

    lo = _umul128(a.lo, b.lo, &hi);
    hi += a.lo * b.hi + a.hi * b.lo;

#elif X128_MSVC && X128_ARM64

    lo = a.lo * b.lo;
    hi = __umulh(a.lo, b.lo);
    hi += a.lo * b.hi + a.hi * b.lo;

#else

    uint64_t a0 = (uint32_t)(a.lo >>  0);
    uint64_t a1 = (uint32_t)(a.lo >> 32);
    uint64_t a2 = (uint32_t)(a.hi >>  0);
    uint64_t a3 = (uint32_t)(a.hi >> 32);

    uint64_t b0 = (uint32_t)(b.lo >>  0);
    uint64_t b1 = (uint32_t)(b.lo >> 32);
    uint64_t b2 = (uint32_t)(b.hi >>  0);
    uint64_t b3 = (uint32_t)(b.hi >> 32);

    uint64_t r0, r1, r2, r3;

    //     a3 a2 a1 a0
    //     b3 b2 b1 b0
    //  --------------
    //           a0*b0
    //        a1*b0
    //     a2*b0
    //  a3*b0
    //        a0*b1
    //     a1*b1
    //  a2*b1
    //     a0*b2
    //  a1*b2
    //  a0*b3
    //  --------------
    //     r3 r2 r1 r0

    r0 = a0 * b0;
    r1 = a1 * b0 + (r0 >> 32);
    r2 = a2 * b0 + (r1 >> 32);
    r3 = a3 * b0 + (r2 >> 32);
    r0 = (uint32_t)r0;
    r1 = (uint32_t)r1;
    r2 = (uint32_t)r2;

    r1 += a0 * b1;
    r2 += a1 * b1 + (r1 >> 32);
    r3 += a2 * b1 + (r2 >> 32);
    r1 = (uint32_t)r1;
    r2 = (uint32_t)r2;

    r2 += a0 * b2;
    r3 += a1 * b2 + (r2 >> 32);
    r2 = (uint32_t)r2;

    r3 += a0 * b3;

    lo = r0 | (r1 << 32);
    hi = r2 | (r3 << 32);

#endif

    return x128_make(lo, hi);
}

static inline uint64_t x128__reciprocal_2by1(uint64_t d)
{
    // Algorithm 2 from "Improved division by invariant integers"
    // https://gmplib.org/~tege/division-paper.pdf

    // d must have top bit set

#if X128_X64 && X128_MSVC

    uint64_t r;
    uint64_t v4 = _udiv128(~0ULL - d, ~0ULL, d, &r);

#elif X128_X64 && (X128_CLANG || X128_GCC)

    uint64_t lo = ~0ULL;
    uint64_t hi = ~0ULL - d;

    uint64_t v4;
    __asm__ __volatile__("divq %3" : "=a"(v4), "+d"(hi) : "0"(lo), "r"(d) : "cc");

#else

    uint64_t d0  = d & 1;                                                           // (1)
    uint64_t d9  = d >> 55;                                                         // (2)
    uint64_t d40 = (d >> 24) + 1;                                                   // (3)
    uint64_t d63 = (d + 1) >> 1;                                                    // (4)

    // 11 bits
    uint64_t v0 = ((1U<<19) - (3U<<8)) / (uint32_t)d9;                              // (5)

    // 21 bits
    uint64_t v1 = (v0 << 11) - ((v0 * v0 * d40) >> 40) - 1;                         // (6)

    // 34 bits
    uint64_t v2 = (v1 << 13) + ((v1 * ((1ULL<<60) - v1*d40)) >> 47);                // (7)

    // 2^96 - v2*d63 + (v2/2)*d0
    uint64_t e = 0ULL - v2 * d63 + (v2 >> 1) * d0;                                  // (8)

    //   2^31*v2 + 2^-65 * v2*e
    // = 2^31*v2 + (2^-64 * v2*e) >> 1
    // = 2^31*v2 + mul_64x64(v2, e).hi >> 1 
    uint64_t v3 = (v2 << 31) + (x128_mul_64x64(v2, e).hi >> 1);                     // (9)

    //   v3 - (2^-64 * (v3 + 2^64 + 1) * d)
    // = v3 - (2^-64 * (v3 * d + 2^64 * d + d))
    // = v3 - (2^-64 * (mul_64x64(v3,d) + d) + d)
    // = v3 - (mul_64x64(v3,d) + d).hi + d)
    uint64_t v4 = v3 - ((x128_add(x128_mul_64x64(v3, d), x128_set_u64(d))).hi + d); // (10)

#endif

    return v4;
}
#ifndef X128_TEST_MARKER
#define X128_TEST_MARKER(...)
#endif

static inline uint64_t x128__reciprocal_3by2(x128 d)
{
    // Algorithm 6 from "Improved division by invariant integers"
    // https://gmplib.org/~tege/division-paper.pdf

    // d must have top bit set

    X128_TEST_MARKER(0)

    uint64_t v = x128__reciprocal_2by1(d.hi);                         // (1)

    uint64_t p = d.hi * v;                                            // (2)
    p += d.lo;                                                        // (3)

    int cmp1 = (p < d.lo) & (p >= d.hi);                              // (4) and (6)
    if (p < d.lo)                                                     // (4)
    {
        X128_TEST_MARKER(1)
    }
    {
        v -= (p < d.lo);                                              // (5)
        if (cmp1)                                                     // (6)
        {
            X128_TEST_MARKER(2)
        }
        v -= cmp1;                                                    // (7)
        p -= cmp1 ? d.hi : 0;                                         // (8)
        p -= d.hi;                                                    // (9)
    }

    x128 t = x128_mul_64x64(v, d.lo);                                 // (10)
    p += t.hi;                                                        // (11)

    int cmp2 = (p < t.hi) & (x128_cmp_u(x128_make(t.lo, p), d) >= 0); // (12) and (14)
    if (p < t.hi)                                                     // (12)
    {
        X128_TEST_MARKER(3)
    }
    {
        X128_TEST_MARKER(3)
        v -= (p < t.hi);                                              // (13)
        if (cmp2)                                                     // (14)
        {
            X128_TEST_MARKER(4)
        }
        v -= cmp2;                                                    // (15)
    }

    return v;
}

static inline uint64_t x128__div_2by1(uint64_t u0, uint64_t u1, uint64_t d, uint64_t v, uint64_t* rem)
{
    // Algorithm 4 from "Improved division by invariant integers"
    // https://gmplib.org/~tege/division-paper.pdf

    X128_TEST_MARKER(5)
    
    x128 q = x128_mul_64x64(v, u1);     // (1)
    q = x128_add(q, x128_make(u0, u1)); // (2)

    q.hi += 1;                          // (3)

    uint64_t r = u0 - (q.hi * d);       // (4)

    if (r > q.lo)                       // (5)
    {
        X128_TEST_MARKER(6)
        q.hi -= 1;                      // (6)
    }
    // having this outside of "if" generates branchless code on MSVC
    r += (r > q.lo) ? d : 0;            // (7)

    if (r >= d)                         // (8)
    {
        X128_TEST_MARKER(7)
        q.hi += 1;                      // (9)
        r -= d;                         // (10)
    }

    *rem = r;
    return q.hi;
}

static inline uint64_t x128__div_3by2(uint64_t u0, uint64_t u1, uint64_t u2, x128 d, uint64_t v, x128* rem)
{
    // Algorithm 5 from "Improved division by invariant integers"
    // https://gmplib.org/~tege/division-paper.pdf

    X128_TEST_MARKER(8)

    x128 q = x128_mul_64x64(v, u2);                       // (1)
    q = x128_add(q, x128_make(u1, u2));                   // (2)

    uint64_t r1 = u1 - (q.hi * d.hi);                     // (3)

    x128 t = x128_mul_64x64(d.lo, q.hi);                  // (4)
    x128 r = x128_sub(x128_sub(x128_make(u0, r1), t), d); // (5)

    q.hi += 1;                                            // (6)

    x128 rd = x128_add(r, d);                             // (9)
    int cmp1 = (r.hi >= q.lo);
    if (cmp1)                                             // (7)
    {
        X128_TEST_MARKER(9)
    }
    q.hi -= cmp1;                                         // (8)
    // writing r assignemnt this way generates branchless code on MSVC
    r.lo = cmp1 ? rd.lo : r.lo;                           // (9)
    r.hi = cmp1 ? rd.hi : r.hi;

    rd = x128_sub(r, d);                                  // (9)
    int cmp2 = (x128_cmp_u(r, d) >= 0);                   // (10)
    if (cmp2)
    {
        X128_TEST_MARKER(10)
    }
    q.hi += cmp2;                                         // (11)
    r.lo = cmp2 ? rd.lo : r.lo;                           // (12)
    r.hi = cmp2 ? rd.hi : r.hi;

    *rem = r;
    return q.hi;
}

bool x128_div_u(x128 x, x128 d, x128* q, x128* r)
{
    if (x128_is_zero(d))
    {
        // cannot divide by zero
        *q = *r = x128_zero();
        return false;
    }

    if (x128_cmp_u(d, x) > 0)
    {
        // if d > x, then q=0, r=x
        *q = x128_zero();
        *r = x;
        return true;
    }

    if (d.hi == 0) // when d is 64-bit value
    {
        size_t shift;
        uint64_t reciprocal = x128_div_u64_reciprocal(d.lo, &shift);
        uint64_t rem = x128_div_u64(x, d.lo, reciprocal, shift, q);
        *r = x128_set_u64(rem);
        return true;
    }

    size_t shift = x128__clz64(d.hi);

    // x << shift in 192 bits
    x128 x0 = x128_shl(x, shift);
    x128 x1 = x128_shl(x128_set_u64(x.hi), shift);

    // d << shift
    d = x128_shl(d, shift);

    uint64_t reciprocal = x128__reciprocal_3by2(d);

    x128 rem;
    uint64_t quo = x128__div_3by2(x0.lo, x0.hi, x1.hi, d, reciprocal, &rem);

    *q = x128_set_u64(quo);
    *r = x128_shr_u(rem, shift);
    return true;
}

bool x128_div_s(x128 x, x128 d, x128* q, x128* r)
{
    bool xn = x128_is_neg(x);
    bool dn = x128_is_neg(d);

    x128 qabs, rabs;
    bool res = x128_div_u(x128_abs(x), x128_abs(d), &qabs, &rabs);

    x128 qabsn = x128_neg(qabs);
    x128 rabsn = x128_neg(rabs);

    bool qn = (xn != dn);
    *q = qn ? qabsn : qabs;
    *r = xn ? rabsn : rabs;

    return res;
}

uint64_t x128_div_u64_reciprocal(uint64_t d, size_t* shift)
{
    // d must be non-zero

    size_t n = x128__clz64(d);
    *shift = n;
    
    return x128__reciprocal_2by1(d << n);
}

uint64_t x128_div_u64(x128 x, uint64_t d, uint64_t reciprocal, size_t shift, x128* q)
{
    // x << shift in 192 bits
    x128 x0 = x128_shl(x, shift);
    x128 x1 = x128_shl(x128_set_u64(x.hi), shift);

    d <<= shift;

    // 2 iterations of Algorithm 7 from "Improved division by invariant integers"
    // https://gmplib.org/~tege/division-paper.pdf

    uint64_t rem;
    uint64_t qhi = x128__div_2by1(x0.hi, x1.hi, d, reciprocal, &rem);
    uint64_t qlo = x128__div_2by1(x0.lo,   rem, d, reciprocal, &rem);

    *q = x128_make(qlo, qhi);
    return rem >> shift;
}
