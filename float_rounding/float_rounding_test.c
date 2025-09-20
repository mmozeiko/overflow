#include "float_rounding.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// mingw roundf function incorrectly rounds +/-0.49999997 to +/-1.0
#if defined(__MINGW32__)
#   define MINGW_ROUNDING_BUGFIX(func, input, result)          \
        if (func == &roundf && fabsf(input) == 0.49999997f) {  \
            result = copysignf(0.f, input);                    \
        }
#else
#   define MINGW_ROUNDING_BUGFIX(func, f, result)
#endif

#define CHECK(x, func, ref) do                                                             \
{                                                                                          \
    float f;                                                                               \
    memcpy(&f, &x, sizeof(f));                                                             \
                                                                                           \
    float value = func(f);                                                                 \
    float expect = ref(f);                                                                 \
    MINGW_ROUNDING_BUGFIX(ref, f, expect);                                                 \
    if (isnan(value) && isnan(expect))                                                     \
    {                                                                                      \
        /* OK, both values are NaNs */                                                     \
    }                                                                                      \
    else if (memcmp(&value, &expect, sizeof(value)) != 0)                                  \
    {                                                                                      \
        uint32_t ivalue, iexpect;                                                          \
        memcpy(&ivalue, &value, sizeof(value));                                            \
        memcpy(&iexpect, &expect, sizeof(expect));                                         \
        printf("\nERROR: f=%1.8e (0x%08x) %s=%1.8e (0x%08x) expected %s=%1.8e (0x%08x)\n", \
            f, x, #func, value, ivalue, #ref, expect, iexpect);                            \
        return 1;                                                                          \
    }                                                                                      \
} while (0)

int main()
{

#if defined(ROUNDING_QUICK)

    // quick sanity checks
    float quick[] =
    {
        +INFINITY,
        +0x1.fffffep+127,   // largest float
        +8388609.f,
        +8388608.f,         // first number without fraction
        +8388607.5f,        // last number with fraction
        +1.5f,
        +0x1.000002p-1f,    // before 0.5
        +0.5f,
        +0x1.fffffep-2f,    // after 0.5
        +0x1.fffffep-126f,  // first subnormal
        +0x1p-149f,         // smallest float
        +0.f,
        NAN,
        -0.f,
        -0x1p-149f,
        -0x1.fffffep-126f,
        -0x1.fffffep-2f,
        -0.5f,
        -0x1.000002p-1f,
        -1.5f,
        -8388607.5f,
        -8388608.f,
        -8388609.f,
        -0x1.fffffep+127,
        -INFINITY,
    };

    for (volatile size_t i=0; i<sizeof(quick)/sizeof(quick[0]); i++)
    {
        float xf = quick[i];

        unsigned int x;
        memcpy(&x, &xf, sizeof(xf));

        CHECK(x, FloatTrunc,      truncf    );
        CHECK(x, FloatFloor,      floorf    );
        CHECK(x, FloatCeil,       ceilf     );
        CHECK(x, FloatRound,      roundf    );
        CHECK(x, FloatNearbyInt,  nearbyintf);
    }

    printf("OK!\n");

#else

    unsigned int x = 0;
    do
    {
        if (x % (1<<28) == 0)
        {
            printf("%d%%", (100 * (x >> 28) + 8) / 16);
            fflush(stdout);
        }
        if (x % (1<<26) == 0)
        {
            printf(".");
            fflush(stdout);
        }

        CHECK(x, FloatTrunc,      truncf    );
        CHECK(x, FloatFloor,      floorf    );
        CHECK(x, FloatCeil,       ceilf     );
        CHECK(x, FloatRound,      roundf    );
        CHECK(x, FloatNearbyInt,  nearbyintf);

        x++;
    }
    while (x != 0);

    printf("100%% OK!\n");

#endif // defined(ROUNDING_QUICK)

}
