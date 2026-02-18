#if 0
static size_t test_marker[16];
#define X128_TEST_MARKER(i) test_marker[i]++;
#endif

#define _CRT_SECURE_NO_DEPRECATE
#include "x128.h"
#include "x128_test.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define COUNTOF(arr) (sizeof(arr)/sizeof(0[arr]))

static inline uint64_t rng()
{
    // https://arxiv.org/abs/1704.00358
    static uint64_t x = 0, w = 0;
    x = x*x + (w += 0xb5ad4eceda1ce2a9);
    return x = (x>>32) | (x<<32);
}

static const uint64_t test_values[] =
{
    0x0000000000000000,
    0x0000000000000001,
    0x0123456789abcdef,
    0x7fffffffffffffff,
    0x8000000000000000,
    0x8000000000000001,
    0xfedcba9876543210,
    0xffffffffffffffff,
};

static void test_fail(const char* res, const char* exp, const char* msg, ...)
{
    printf("\n");
    printf("ERROR:    ");
    {
        va_list args;
        va_start(args, msg);

        while (*msg)
        {
            if (*msg == '%')
            {
                switch (*++msg)
                {
                case 's':
                    printf("%s", va_arg(args, const char*));
                    break;
                case 'z':
                    printf("%zu", va_arg(args, size_t));
                    break;
                case 'u':
                    printf("%016llx", va_arg(args, unsigned long long));
                    break;
                case 'x':
                {
                    x128 x = va_arg(args, x128);
                    printf("0x%016llx'%016llx", (unsigned long long)x.hi, (unsigned long long)x.lo);
                    break;
                }
                default:
                    printf("%c", msg[-1]);
                }
            }
            else
            {
                printf("%c", msg[0]);
            }
            msg++;
        }
        va_end(args);
    }
    printf("\n");
    printf("expected: %s\n", exp);
    printf("result:   %s\n", res);
    exit(EXIT_FAILURE);
}

#define CHECK_I(res, exp, msg, ...) do {                              \
    if ((res) != (exp)) {                                             \
        char __sres[32]; snprintf(__sres, sizeof(__sres), "%d", res); \
        char __sexp[32]; snprintf(__sexp, sizeof(__sexp), "%d", exp); \
        test_fail(__sres, __sexp, msg, ##__VA_ARGS__);                \
    }                                                                 \
} while (0)

#define CHECK_Z(res, exp, msg, ...) do {                               \
    if ((res) != (exp)) {                                              \
        char __sres[32]; snprintf(__sres, sizeof(__sres), "%zu", res); \
        char __sexp[32]; snprintf(__sexp, sizeof(__sexp), "%zu", exp); \
        test_fail(__sres, __sexp, msg, ##__VA_ARGS__);                 \
    }                                                                  \
} while (0)

#define CHECK_U(res, exp, msg, ...) do {                                                           \
    if ((res) != (exp)) {                                                                          \
        char __sres[32]; snprintf(__sres, sizeof(__sres), "0x%016llu", (unsigned long long)(res)); \
        char __sexp[32]; snprintf(__sexp, sizeof(__sexp), "0x%016llu", (unsigned long long)(exp)); \
        test_fail(__sres, __sexp, msg, ##__VA_ARGS__);                                             \
    }                                                                                              \
} while (0)

#define CHECK_X(res, exp, msg, ...) do {                                                                                                    \
    if ((res).lo != (exp).lo || (res).hi != (exp).hi) {                                                                                     \
        char __sres[64]; snprintf(__sres, sizeof(__sres), "0x%016llx'%016llx", (unsigned long long)(res).hi, (unsigned long long)(res).lo); \
        char __sexp[64]; snprintf(__sexp, sizeof(__sexp), "0x%016llx'%016llx", (unsigned long long)(exp).hi, (unsigned long long)(exp).lo); \
        test_fail(__sres, __sexp, msg, ##__VA_ARGS__);                                                                                      \
    }                                                                                                                                       \
} while (0)

#define TEST_CMPI(cmp) ((cmp == 'n') ? -1 : (cmp == 'p') ? +1 : 0)

int main()
{
    printf("%-20s ", "x128_zero");
    {
        x128 res = x128_zero();
        x128 exp = { 0, 0 };

        CHECK_X(res, exp, "x128_zero");
    }
    printf(" OK\n");

    printf("%-20s ", "x128_set/get");
    {
        for (size_t i=0; i<COUNTOF(test_values); i++)
        {
            uint64_t value = test_values[i];

            x128 u = x128_set_u64(value);
            CHECK_X(u, test_x128_set_u64[i], "x128_set_u64(%u)", value);

            x128 s = x128_set_s64(value);
            CHECK_X(s, test_x128_set_s64[i], "x128_set_s64(%u)", value);

            assert(x128_get_u64(u) == value);
            assert(x128_get_s64(s) == (int64_t)value);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_make");
    {
        for (size_t i=0; i<8; i++)
        {
            uint64_t lo = rng();
            uint64_t hi = rng();

            x128 res = x128_make(lo, hi);
            x128 exp = { lo, hi };
            CHECK_X(res, exp, "x128_make(%u, %u)", lo, hi);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_is_zero");
    {
        x128 z = x128_zero();
        assert(x128_is_zero(z));

        x128 nz1 = { 1, 0 };
        assert(!x128_is_zero(nz1));

        x128 nz2 = { 0, 1 };
        assert(!x128_is_zero(nz2));
    }
    printf(" OK\n");

    printf("%-20s ", "x128_is_equal");
    {
        uint64_t r = rng();

        x128 a = x128_make( r, ~r);
        assert( x128_is_equal(a, a));

        x128 b = x128_make( r,  r);
        assert(!x128_is_equal(a, b));

        x128 c = x128_make(~r, ~r);
        assert(!x128_is_equal(a, c));
    }
    printf(" OK\n");

    printf("%-20s ", "x128_is_even/odd");
    {
        for (size_t i=0; i<1024; i++)
        {
            uint64_t r = rng();

            x128 x = x128_make(r, rng());
            assert(x128_is_even(x) == ((r%2) == 0));
            assert(x128_is_odd(x)  == ((r%2) != 0));
        }
        assert(x128_is_even(x128_zero()) == true );
        assert(x128_is_odd (x128_zero()) == false);
    }
    printf(" OK\n");

    printf("%-20s ", "x128_is_neg/pos");
    {
        for (size_t i=0; i<1024; i++)
        {
            uint64_t hi = rng();
            uint64_t lo = rng();

            x128 x = x128_make(lo, hi);
            assert(x128_is_neg(x) == ((int64_t)hi <  0));
            assert(x128_is_pos(x) == ((int64_t)hi >= 0));

        }
        assert(x128_is_neg(x128_zero()) == false);
        assert(x128_is_pos(x128_zero()) == true );
    }
    printf(" OK\n");

    printf("%-20s ", "x128_signum");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);

            int exp = TEST_CMPI(test_x128_signum[alo][ahi]);
            CHECK_I(x128_signum(a), exp, "x128_signum(%x)", a);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_cmp");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        for (size_t blo=0; blo<COUNTOF(test_values); blo++)
        for (size_t bhi=0; bhi<COUNTOF(test_values); bhi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);
            x128 b = x128_make(test_values[blo], test_values[bhi]);

            int exp_u = TEST_CMPI(test_x128_cmp_u[alo][ahi][blo][bhi]);
            CHECK_I(x128_cmp_u(a, b), exp_u, "x128_cmp_u(%x, %x)", a, b);

            int exp_s = TEST_CMPI(test_x128_cmp_s[alo][ahi][blo][bhi]);
            CHECK_I(x128_cmp_s(a, b), exp_s, "x128_cmp_s(%x, %x)", a, b);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_min/max");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        for (size_t blo=0; blo<COUNTOF(test_values); blo++)
        for (size_t bhi=0; bhi<COUNTOF(test_values); bhi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);
            x128 b = x128_make(test_values[blo], test_values[bhi]);

            CHECK_X(x128_min_u(a, b), test_x128_min_u[alo][ahi][blo][bhi], "x128_min_u(%x, %x)", a, b);
            CHECK_X(x128_min_s(a, b), test_x128_min_s[alo][ahi][blo][bhi], "x128_min_s(%x, %x)", a, b);
            CHECK_X(x128_max_u(a, b), test_x128_max_u[alo][ahi][blo][bhi], "x128_max_u(%x, %x)", a, b);
            CHECK_X(x128_max_s(a, b), test_x128_max_s[alo][ahi][blo][bhi], "x128_max_s(%x, %x)", a, b);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_not");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);

            CHECK_X(x128_not(a), test_x128_not[alo][ahi], "x128_not(%x)", a);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_or/and/xor");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        for (size_t blo=0; blo<COUNTOF(test_values); blo++)
        for (size_t bhi=0; bhi<COUNTOF(test_values); bhi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);
            x128 b = x128_make(test_values[blo], test_values[bhi]);

            CHECK_X(x128_or(a, b),  test_x128_or [alo][ahi][blo][bhi], "x128_or (%x, %x)", a, b);
            CHECK_X(x128_and(a, b), test_x128_and[alo][ahi][blo][bhi], "x128_and(%x, %x)", a, b);
            CHECK_X(x128_xor(a, b), test_x128_xor[alo][ahi][blo][bhi], "x128_xor(%x, %x)", a, b);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_shl/shr");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        for (size_t n=0; n<=128; n++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);

            CHECK_X(x128_shl  (a, n), test_x128_shl[alo][ahi][n],   "x128_shl(%x, %z)",   a, n);
            CHECK_X(x128_shr_u(a, n), test_x128_shr_u[alo][ahi][n], "x128_shr_u(%x, %z)", a, n);
            CHECK_X(x128_shr_s(a, n), test_x128_shr_s[alo][ahi][n], "x128_shr_s(%x, %z)", a, n);

            CHECK_X(x128_shl  (a, n+128), x128_zero(),                     "x128_shl(%x, %z)",   a, n+128);
            CHECK_X(x128_shr_u(a, n+128), x128_zero(),                     "x128_shr_u(%x, %z)", a, n+128);
            CHECK_X(x128_shr_s(a, n+128), x128_set_s64((int64_t)a.hi>>63), "x128_shr_s(%x, %z)", a, n+128);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_clz");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);

            CHECK_Z(x128_clz(a), test_x128_clz[alo][ahi], "x128_clz(%x)", a);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_ctz");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);

            CHECK_Z(x128_ctz(a), test_x128_ctz[alo][ahi], "x128_ctz(%x)", a);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_popcnt");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);

            CHECK_Z(x128_popcnt(a), test_x128_popcnt[alo][ahi], "x128_popcnt(%x)", a);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_not");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);

            CHECK_X(x128_abs(a), test_x128_abs[alo][ahi], "x128_abs(%x)", a);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_neg");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);

            CHECK_X(x128_neg(a), test_x128_neg[alo][ahi], "x128_neg(%x)", a);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_add/sub");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        for (size_t blo=0; blo<COUNTOF(test_values); blo++)
        for (size_t bhi=0; bhi<COUNTOF(test_values); bhi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);
            x128 b = x128_make(test_values[blo], test_values[bhi]);

            CHECK_X(x128_add(a, b), test_x128_add[alo][ahi][blo][bhi], "x128_add(%x, %x)", a, b);
            CHECK_X(x128_sub(a, b), test_x128_sub[alo][ahi][blo][bhi], "x128_sub(%x, %x)", a, b);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_mul_64x64");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t blo=0; blo<COUNTOF(test_values); blo++)
        {
            uint64_t a = test_values[alo];
            uint64_t b = test_values[blo];

            CHECK_X(x128_mul_64x64(a, b), test_x128_mul_64x64[alo][blo], "x128_mul_64x64(%u, %u)", a, b);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_mul_128x64");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        for (size_t blo=0; blo<COUNTOF(test_values); blo++)
        {
            x128     a = x128_make(test_values[alo], test_values[ahi]);
            uint64_t b = test_values[blo];

            CHECK_X(x128_mul_128x64(a, b), test_x128_mul_128x64[alo][ahi][blo], "x128_mul(%x, %u)", a, b);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_mul");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        for (size_t blo=0; blo<COUNTOF(test_values); blo++)
        for (size_t bhi=0; bhi<COUNTOF(test_values); bhi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);
            x128 b = x128_make(test_values[blo], test_values[bhi]);

            CHECK_X(x128_mul(a, b), test_x128_mul[alo][ahi][blo][bhi], "x128_mul(%x, %x)", a, b);
        }
    }
    printf(" OK\n");

#if 0
    for (size_t i=0; i<~0ULL; i++)
    {
        memset(test_marker, 0, sizeof(test_marker));

        x128 a = { rng(), rng() };
        x128 b = { rng(), rng() };

        x128 q, r;
        x128_div_u(a, b, &q, &r);

        size_t* m = test_marker;

        if (// Algorithm 6
            m[0] == 1 && m[1] == 0 && m[2] == 0 && m[3] == 0 && m[4] == 0 &&
            // Algorithm 4
            m[5] == 0 && m[6] == 0 && m[7] == 0 &&
            // Algorithm 5
            m[8] == 1 && m[9] == 0 && m[10] == 0)
        {
            printf("{ 0x%016llx, 0x%016llx }\n", a.lo, a.hi);
            printf("{ 0x%016llx, 0x%016llx }\n", b.lo, b.hi);
            printf("{ 0x%016llx, 0x%016llx }\n", q.lo, q.hi);
            printf("{ 0x%016llx, 0x%016llx }\n", r.lo, r.hi);
            return 0;
        }
    }
    return 0;
#endif

    printf("%-20s ", "x128_div");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        for (size_t blo=0; blo<COUNTOF(test_values); blo++)
        for (size_t bhi=0; bhi<COUNTOF(test_values); bhi++)
        {
            x128 a = x128_make(test_values[alo], test_values[ahi]);
            x128 b = x128_make(test_values[blo], test_values[bhi]);

            x128 q, r;
            bool res = x128_div_u(a, b, &q, &r);

            if (x128_is_zero(b))
            {
                assert(res == false);
            }
            else
            {
                assert(res == true);
            }

            CHECK_X(q, test_x128_div_u[alo][ahi][blo][bhi], "x128_div_u(%x, %x) -> q=%x, r=%x", a, b, q, r);
            CHECK_X(r, test_x128_mod_u[alo][ahi][blo][bhi], "x128_div_u(%x, %x) -> q=%x, r=%x", a, b, q, r);
        }

        static const struct
        {
            x128 a;
            x128 b;
            x128 q;
            x128 r;
        }
        extra_u[] =
        {
            // uses Algorithms 6/5, does not go inside if statements (6.4), (6.12), (5.7), (5.10)
            { { 0xc7d6e88eaec0f2e0, 0xd380c62741768b8c }, { 0x0052b96051ce0cb0, 0x9d2f4079b486caf4 }, { 0x0000000000000001, 0x0000000000000000 }, { 0xc7842f2e5cf2e630, 0x365185ad8cefc098 } },

            // uses Algorithms 6/5, goes inside only (6.4)
            { { 0xfc3e13abb01053c8, 0x92baaecbc0ffd869 }, { 0x308ba045b47bcc9f, 0x8ee0f29e1a8fc5ed }, { 0x0000000000000001, 0x0000000000000000 }, { 0xcbb27365fb948729, 0x03d9bc2da670127c } },
            // uses Algorithms 6/5, goes insside only (6.4), (6.6)
            { { 0xf82039afec78e32f, 0x9f444f30211d33ff }, { 0x9c0a6739d4f54af4, 0x92cbaa71d79a153b }, { 0x0000000000000001, 0x0000000000000000 }, { 0x5c15d2761783983b, 0x0c78a4be49831ec4 } },
            // uses Algorithms 6/5, goes inside only in (6.12)
            { { 0x6872a7ef5aff9ce7, 0xc71f45a01d901515 }, { 0xbc3c43914123f324, 0xc5c20f91184b2d21 }, { 0x0000000000000001, 0x0000000000000000 }, { 0xac36645e19dba9c3, 0x015d360f0544e7f3 } },
            // uses Algorithms 6/5, goes inside only (6.4), (6.12), (6.14)
            { { 0x9e64336bcf4c1a8f, 0xd8dfd3b485fb948f }, { 0xe5f3025d3bcf8ef2, 0x8a9d99e97dfdd310 }, { 0x0000000000000001, 0x0000000000000000 }, { 0xb871310e937c8b9d, 0x4e4239cb07fdc17e } },
            // uses Algorithms 6/5, goes inside only (6.4), (6.6), (6.12), (6.14)
            { { 0xf87c357f55f1a756, 0xb8aefbbbd06dd5a9 }, { 0x3dbb371145c15921, 0x2065666a69c2432d }, { 0x0000000000000005, 0x0000000000000000 }, { 0xc3d42228f92ae9b1, 0x16b3fba7bfa285c7 } },

            // uses Algorithms 6/5, goes inside only (5.8)
            { { 0x9a51314bc491a5c5, 0x7c9f16b41c04980a }, { 0xc4d6502821608a1f, 0x341cc22e43a5e33b }, { 0x0000000000000002, 0x0000000000000000 }, { 0x10a490fb81d09187, 0x1465925794b8d193 } },
            // uses Algorithms 6/5, goes inside only (5.10)
            { { 0xd5bf515c00000000, 0xee8693506870bdb5 }, { 0x065a2b1000000000, 0x0000000000000001 }, { 0xe8c01810be0d4c47, 0x0000000000000000 }, { 0x3e249fec00000000, 0x0000000000000000 } },
            // uses Algorithms 6/5, goes inside only (5.8), (5.10)
            { { 0xde0f5ace00000000, 0xf8729e9755bf820d }, { 0x0df08e1100000000, 0x0000000000000001 }, { 0xeb9e2f24e023f704, 0x0000000000000000 }, { 0x40e7bb8a00000000, 0x0000000000000000 } },

            // uses Algorithms 6/5, goes inside all if statements - (6.4), (6.6), (6.12), (6.14), (5.8), (5.10)
            { { 0x8000000000000000, 0xffffffffffffffff }, { 0x210d03dbf20dcd9f, 0x0000000000000004 }, { 0x3dffb8925fac115b, 0x0000000000000000 }, { 0x0079b87be29c597b, 0x0000000000000000 } },

            // uses Algorithm 4, does not go inside (5), (8)
            { { 0xd8dfd3b485fb948f, 0xe5f3025d3bcf8ef2 }, { 0x000000008a9d99e9, 0x0000000000000000 }, { 0x17f5f5179dc6453b, 0x00000001a8ad91cd }, { 0x0000000029f74edc, 0x0000000000000000 } },
            // uses Algorithm 4, goes inside only 1x(5)
            { { 0xfc3e13abb01053c8, 0x92baaecbc0ffd869 }, { 0x308ba045b47bcc9f, 0x0000000000000000 }, { 0x05c3866f51b55f97, 0x0000000000000003 }, { 0x231ef25d4eb0a0ff, 0x0000000000000000 } },
            // uses Algorithm 4, goes inside only 2x(5)
            { { 0x8ee0f29e1a8fc5ed, 0x6872a7ef5aff9ce7 }, { 0xc71f45a01d901515, 0x0000000000000000 }, { 0x86485d75c49511e5, 0x0000000000000000 }, { 0x790a3fd3be0d8524, 0x0000000000000000 } },
            // uses Algorithm 4, goes inside only 1x(8)
            { { 0xef2dbe815876e8ca, 0xf65460eae32f05ef }, { 0x8a6fb95066c26ff9, 0x0000000000000000 }, { 0xc784c975d1ef5467, 0x0000000000000001 }, { 0x014b72b26107279b, 0x0000000000000000 } },
            // uses Algorithm 4, goes inside 2x(5) and 1x(8)
            { { 0x7f9e701b6db265cd, 0x397e28ad520b626a }, { 0x4afcf9112dbddfca, 0x0000000000000000 }, { 0xc445e776b1ac4cee, 0x0000000000000000 }, { 0x126c582583046001, 0x0000000000000000 } },
        };

        for (size_t i=0; i<COUNTOF(extra_u); i++)
        {
            x128 a = extra_u[i].a;
            x128 b = extra_u[i].b;

            x128 q, r;
            bool res = x128_div_u(a, b, &q, &r);
            assert(res == true);

            CHECK_X(q, extra_u[i].q, "x128_div_u(%x, %x) -> q=%x, r=%x", a, b, q, r);
            CHECK_X(r, extra_u[i].r, "x128_div_u(%x, %x) -> q=%x, r=%x", a, b, q, r);
        }

        static const struct
        {
            x128 a;
            x128 b;
            x128 q;
            x128 r;
        }
        extra_s[] =
        {
            // +/+ => +/+
            { { 0xadac0bb65e7ea4a0, 0x00038f02a93500e7 }, { 0x00000000000d5247, 0x0000000000000000 }, { 0xee849e78b999377a, 0x00000000446312e5 }, { 0x0000000000092dca, 0x0000000000000000 } },
            // +/- => -/+
            { { 0xadac0bb65e7ea4a0, 0x00038f02a93500e7 }, { 0xfffffffffff2adb9, 0xffffffffffffffff }, { 0x117b61874666c886, 0xffffffffbb9ced1a }, { 0x0000000000092dca, 0x0000000000000000 } }, 
            // -/+ => -/-
            { { 0x5253f449a1815b60, 0xfffc70fd56caff18 }, { 0x00000000000d5247, 0x0000000000000000 }, { 0x117b61874666c886, 0xffffffffbb9ced1a }, { 0xfffffffffff6d236, 0xffffffffffffffff } },
            // -/- => +/-
            { { 0x5253f449a1815b60, 0xfffc70fd56caff18 }, { 0xfffffffffff2adb9, 0xffffffffffffffff }, { 0xee849e78b999377a, 0x00000000446312e5 }, { 0xfffffffffff6d236, 0xffffffffffffffff } },
        };

        for (size_t i=0; i<COUNTOF(extra_s); i++)
        {
            x128 a = extra_s[i].a;
            x128 b = extra_s[i].b;

            x128 q, r;
            bool res = x128_div_s(a, b, &q, &r);
            assert(res == true);

            CHECK_X(q, extra_s[i].q, "x128_div_s(%x, %x) -> q=%x, r=%x", a, b, q, r);
            CHECK_X(r, extra_s[i].r, "x128_div_s(%x, %x) -> q=%x, r=%x", a, b, q, r);
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_div_u64");
    {
        for (size_t alo=0; alo<COUNTOF(test_values); alo++)
        for (size_t ahi=0; ahi<COUNTOF(test_values); ahi++)
        for (size_t blo=0; blo<COUNTOF(test_values); blo++)
        {
            x128     a = x128_make(test_values[alo], test_values[ahi]);
            uint64_t b = test_values[blo];

            if (b != 0)
            {
                size_t shift;
                uint64_t reciprocal = x128_div_u64_reciprocal(b, &shift);

                x128 q;
                uint64_t r = x128_div_u64(a, b, reciprocal, shift, &q);

                CHECK_X(q, test_x128_div_u64[alo][ahi][blo].q, "x128_div_u64(%x, %u) -> q=%x, r=%u", a, b, q, r);
                CHECK_U(r, test_x128_div_u64[alo][ahi][blo].r, "x128_div_u64(%x, %u) -> q=%x, r=%u", a, b, q, r);
            }
        }
    }
    printf(" OK\n");

    printf("%-20s ", "x128_set_str");
    {
        x128 expected_u = x128_make(0x0123456789abcdef, 0xfedcba9876543210);
        x128 expected_s = x128_neg(x128_make(0xfedcba9876543210, 0x0123456789abcdef));

        for (size_t base=2; base<=36; base++)
        {
            const char* str_u = test_x128_str_u[base-2];
            x128 u = x128_set_str(str_u, 0, base);
            CHECK_X(u, expected_u, "x128_set_str(%s, %z)", str_u, base);

            const char* str_s = test_x128_str_s[base-2];
            x128 s = x128_set_str(str_s, 0, base);
            CHECK_X(s, expected_s, "x128_set_str(%s, %z)", str_s, base);
        }

        x128 x = x128_set_str("0", 1, 36);
        CHECK_X(x, x128_zero(), "x128_set_str(%s)", "0");
    }
    printf(" OK\n");

    printf("%-20s ", "x128_get_str");
    {
        x128 expected_u = x128_make(0x0123456789abcdef, 0xfedcba9876543210);
        x128 expected_s = x128_neg(x128_make(0xfedcba9876543210, 0x0123456789abcdef));

        for (size_t base=2; base<=36; base++)
        {
            char str[256] = { 0 };

            size_t size = x128_get_str_u(expected_u, str, sizeof(str), base);
            const char* test_str = test_x128_str_u[base-2];

            CHECK_Z(size, strlen(test_str),   "x128_get_str_u(%x, %z) -> %s", expected_u, base, test_str);
            CHECK_I(strcmp(str, test_str), 0, "x128_get_str_u(%x, %z) -> %s", expected_u, base, test_str);

            size = x128_get_str_s(expected_s, str, sizeof(str), base);
            test_str = test_x128_str_s[base-2];

            CHECK_Z(size, strlen(test_str),   "x128_get_str_s(%x, %z) -> %s", expected_s, base, test_str);
            CHECK_I(strcmp(str, test_str), 0, "x128_get_str_s(%x, %z) -> %s", expected_s, base, test_str);
        }

        {
            x128 zero = x128_zero();

            char str[256] = { 0 };
            size_t size = x128_get_str_u(zero, str, sizeof(str), 36);

            CHECK_Z(size,     (size_t)1, "x128_get_str_u(%x) -> %s", zero, str);
            CHECK_I(strcmp(str, "0"), 0, "x128_get_str_u(%x) -> %s", zero, str);
        }

        {
            x128 zero = x128_zero();

            char str[256] = { 0 };
            size_t size = x128_get_str_s(zero, str, sizeof(str), 36);

            CHECK_Z(size,     (size_t)1, "x128_get_str_s(%x) -> %s", zero, str);
            CHECK_I(strcmp(str, "0"), 0, "x128_get_str_s(%x) -> %s", zero, str);
        }
    }
    printf(" OK\n");
}
