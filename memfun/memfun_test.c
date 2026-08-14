#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define MEM_STATIC
#include "memfun.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#if defined(_WIN32)
#  include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#  include <unistd.h>
#  include <sys/mman.h>
#endif

#define countof(arr) (sizeof(arr)/sizeof(0[arr]))

typedef int    MemCompareFun(const void* ptr1, const void* ptr2, size_t size);
typedef bool   MemIsEqualFun(const void* ptr1, const void* ptr2, size_t size);
typedef size_t MemFindFun   (const void* ptr, size_t size, uint8_t value);

static int MemCompare_ref(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    for (size_t i=0; i<size; i++)
    {
        uint8_t c1 = p1[i];
        uint8_t c2 = p2[i];
        if (c1 != c2) return c1 - c2;
    }

    return 0;
}

static int MemCompareI_ref(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    for (size_t i=0; i<size; i++)
    {
        uint8_t c1 = MemToLower1(p1[i]);
        uint8_t c2 = MemToLower1(p2[i]);
        if (c1 != c2) return c1 - c2;
    }

    return 0;
}

static bool MemIsEqual_ref(const void* ptr1, const void* ptr2, size_t size)
{
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    for (size_t i=0; i<size; i++)
    {
        uint8_t c1 = p1[i];
        uint8_t c2 = p2[i];
        if (c1 != c2) return false;
    }

    return true;
}

static size_t MemFind_ref(const void* ptr, size_t size, uint8_t value)
{
    const uint8_t* p = (const uint8_t*)ptr;

    for (size_t i=0; i<size; i++)
    {
        if (p[i] == value) return i;
    }

    return size;
}

static size_t MemFindNot_ref(const void* ptr, size_t size, uint8_t value)
{
    const uint8_t* p = (const uint8_t*)ptr;

    for (size_t i=0; i<size; i++)
    {
        if (p[i] != value) return i;
    }

    return size;
}

static int MemCompare_std(const void* ptr1, const void* ptr2, size_t size)
{
    return memcmp(ptr1, ptr2, size);
}

static int MemCompareI_std(const void* ptr1, const void* ptr2, size_t size)
{
#if defined(_WIN32) && !defined(__MINGW32__)
    return _memicmp(ptr1, ptr2, size);
#else
    const uint8_t* p1 = (const uint8_t*)ptr1;
    const uint8_t* p2 = (const uint8_t*)ptr2;

    for (size_t i=0; i<size; i++)
    {
        uint8_t c1 = (uint8_t)tolower(p1[i]);
        uint8_t c2 = (uint8_t)tolower(p2[i]);
        if (c1 != c2) return c1 - c2;
    }

    return 0;
#endif
}

static bool MemIsEqual_std(const void* ptr1, const void* ptr2, size_t size)
{
#if defined(__linux__)
    return __memcmpeq(ptr1, ptr2, size) == 0;
#else
    return memcmp(ptr1, ptr2, size) == 0;
#endif
}

static size_t MemFind_std(const void* ptr, size_t size, uint8_t value)
{
    void* r = (void*)memchr(ptr, value, size);
    return r ? (size_t)((char*)r - (char*)ptr) : size;
}

static bool test_error(int expected, int result, const char* ptr1, const char* ptr2, size_t size)
{
    printf("ERROR\n");

    printf("size     = %zu\n", size);
    printf("expected = %d\n", expected);
    printf("result   = %d\n", result);

    printf("%s     = ", ptr2 ? "ptr1" : "ptr ");
    for (size_t i=0; i<size; i++) printf("%02hhx", (uint8_t)ptr1[i]);
    printf("\n");

    if (ptr2)
    {
        printf("ptr2     = ");
        for (size_t i=0; i<size; i++) printf("%02hhx", (uint8_t)ptr2[i]);
        printf("\n");
    }

    return false;
}

static bool test_compare(const char* ptr1, const char* ptr2, size_t size, MemCompareFun* ref, MemCompareFun* fun)
{
    int expected = ref(ptr1, ptr2, size);
    int result   = fun(ptr1, ptr2, size);

    expected = expected < 0 ? -1 : expected > 0 ? +1 : 0;
    result   = result   < 0 ? -1 : result   > 0 ? +1 : 0;

    if (result == expected)
    {
        return true;
    }
    return test_error(expected, result, ptr1, ptr2, size);
}

static bool test_isequal(const char* ptr1, const char* ptr2, size_t size, MemIsEqualFun* ref, MemIsEqualFun* fun)
{
    bool expected = ref(ptr1, ptr2, size);
    bool result   = fun(ptr1, ptr2, size);

    if (result == expected)
    {
        return true;
    }
    return test_error(expected, result, ptr1, ptr2, size);
}

static bool test_find(const char* ptr, size_t size, uint8_t value, MemFindFun* ref, MemFindFun* fun)
{
    size_t expected = ref(ptr, size, value);
    size_t result   = fun(ptr, size, value);

    if (result == expected)
    {
        return true;
    }
    return test_error((int)expected, (int)result, ptr, NULL, size);
}

static bool run_compare(char* ptr, size_t page_size, MemCompareFun* ref, MemCompareFun* fun)
{
    if (!test_compare(NULL, NULL, 0, ref, fun)) return false;

    // max size to test
    const size_t size = 256;

    memset(ptr + page_size, 0, 2 * page_size);

    // test all sizes
    for (size_t n=1; n<size; n++)
    {
        char* ptr1 = ptr + page_size;               // ptr1 is at start of page boundary (no reading before it)
        char* ptr2 = ptr + 3 * page_size - n;       // ptr2 is at end of page boundary (no reading after it)
        char* ptr3 = ptr + page_size + page_size/2; // ptr3 is in middle, can be written before & after

        // strings are equal by default
        for (size_t i=0; i<n; i++)
        {
            if (ref == &MemCompare_ref)
            {
                ptr1[i] = ptr2[i] = ptr3[i] = (char)(31 * i + 13);
            }
            else
            {
                ptr1[i] = ptr2[i] = (char)('A' + (i % ('Z'-'A'+1)));
                ptr3[i] = i%2 ? ptr1[i] : (char)tolower(ptr1[i]);
            }
        }

        for (size_t t=0; t<2; t++)
        {
            if (!test_compare(ptr1, ptr3, n, ref, fun)) return false;
            if (!test_compare(ptr3, ptr1, n, ref, fun)) return false;

            // will mismatch if ptr1 is read past the end
            ptr1[n] ^= (char)0xff;
        }

        for (size_t t=0; t<2; t++)
        {
            if (!test_compare(ptr2, ptr3, n, ref, fun)) return false;
            if (!test_compare(ptr3, ptr2, n, ref, fun)) return false;

            // will mismatch if ptr2 is read past the beginning
            ptr2[-1] ^= (char)0xff;
        }

        // test a difference in each position in [0,n] interval
        for (size_t k=0; k<n; k++)
        {
            ptr3[k] ^= (char)0xff;
            if (!test_compare(ptr1, ptr3, n, ref, fun)) return false;
            if (!test_compare(ptr1, ptr3, n, ref, fun)) return false;
            if (!test_compare(ptr3, ptr1, n, ref, fun)) return false;
            if (!test_compare(ptr3, ptr2, n, ref, fun)) return false;
            ptr3[k] ^= (char)0xff;
        }
    }

    printf("OK\n");
    return true;
}

static bool run_isequal(char* ptr, size_t page_size, MemIsEqualFun* ref, MemIsEqualFun* fun)
{
    if (!test_isequal(NULL, NULL, 0, ref, fun)) return false;

    // max size to test
    const size_t size = 256;

    memset(ptr + page_size, 0, 2 * page_size);

    // test all sizes
    for (size_t n=1; n<size; n++)
    {
        char* ptr1 = ptr + page_size;               // ptr1 is at start of page boundary (no reading before it)
        char* ptr2 = ptr + 3 * page_size - n;       // ptr2 is at end of page boundary (no reading after it)
        char* ptr3 = ptr + page_size + page_size/2; // ptr3 is in middle, can be written before & after

        // strings are equal by default
        for (size_t i=0; i<n; i++)
        {
            ptr1[i] = ptr2[i] = ptr3[i] = (char)(31 * i + 13);
        }

        for (size_t t=0; t<2; t++)
        {
            if (!test_isequal(ptr1, ptr3, n, ref, fun)) return false;
            if (!test_isequal(ptr3, ptr1, n, ref, fun)) return false;

            // will mismatch if ptr1 is read past the end
            ptr1[n] ^= (char)0xff;
        }

        for (size_t t=0; t<2; t++)
        {
            if (!test_isequal(ptr2, ptr3, n, ref, fun)) return false;
            if (!test_isequal(ptr3, ptr2, n, ref, fun)) return false;

            // will mismatch if ptr2 is read past the beginning
            ptr2[-1] ^= (char)0xff;
        }

        // test a difference in each position in [0,n] interval
        for (size_t k=0; k<n; k++)
        {
            ptr3[k] ^= (char)0xff;
            if (!test_isequal(ptr1, ptr3, n, ref, fun)) return false;
            if (!test_isequal(ptr1, ptr3, n, ref, fun)) return false;
            if (!test_isequal(ptr3, ptr1, n, ref, fun)) return false;
            if (!test_isequal(ptr3, ptr2, n, ref, fun)) return false;
            ptr3[k] ^= (char)0xff;
        }
    }

    printf("OK\n");
    return true;
}

static bool run_find(char* ptr, size_t page_size, MemFindFun* ref, MemFindFun* fun)
{
    if (!test_find(NULL, 0, 0xff, ref, fun)) return false;

    // max size to test
    const size_t size = 256;

    uint8_t initial = (ref == &MemFind_ref) ? 0x00 : 0xff;

    memset(ptr + page_size, initial, 2 * page_size);

    // test when page boundary can be crossed, and difference is on next page
    for (size_t i=2; i<32; i++)
    {
        ptr[2*page_size] ^= (char)0xff;
        if (!test_find(ptr + 2*page_size - 1, i, 0xff, ref, fun)) return false;
        ptr[2*page_size] ^= (char)0xff;
    }

    // test all sizes
    for (size_t n=1; n<size; n++)
    {
        char* ptr1 = ptr + page_size;               // ptr1 is at start of page boundary (no reading before it)
        char* ptr2 = ptr + 3 * page_size - n;       // ptr2 is at end of page boundary (no reading after it)
        char* ptr3 = ptr + page_size + page_size/2; // ptr3 is in middle, can be written before & after

        for (size_t t=0; t<2; t++)
        {
            if (!test_find(ptr1, n, 0xff, ref, fun)) return false;
            if (!test_find(ptr2, n, 0xff, ref, fun)) return false;

            // will mismatch if ptr1 is read past the end
            ptr1[n] ^= (char)0xff;

            // will mismatch if ptr2 is read past the beginning
            ptr2[-1] ^= (char)0xff;
        }

        // test a difference in each position in [0,n] interval
        for (size_t k=0; k<n; k++)
        {
            ptr1[k] ^= (char)0xff;
            ptr2[k] ^= (char)0xff;
            ptr3[k] ^= (char)0xff;
            if (!test_find(ptr1, n, 0xff, ref, fun)) return false;
            if (!test_find(ptr2, n, 0xff, ref, fun)) return false;
            if (!test_find(ptr3, n, 0xff, ref, fun)) return false;
            ptr1[k] ^= (char)0xff;
            ptr2[k] ^= (char)0xff;
            ptr3[k] ^= (char)0xff;
        }
    }

    printf("OK\n");
    return true;
}

static const struct
{
    const char*    name;
    MemCompareFun* compare;
    MemCompareFun* comparei;
    MemIsEqualFun* isequal;
    MemFindFun*    find;
    MemFindFun*    findnot;
    int            cpuid;
}
memfun[] =
{
    { "std",            &MemCompare_std,     &MemCompareI_std,           &MemIsEqual_std,     &MemFind_std,       0,                    0                   },
    { "generic",        &MemCompare_generic, &MemCompareI_generic,       &MemIsEqual_generic, &MemFind_generic,   &MemFindNot_generic,  0                   },
    { "auto",           &MemCompare,         &MemCompareI,               &MemIsEqual,         &MemFind,           &MemFindNot,          0                   },
#if MEM_ARCH_RVV
    { "rvv",            &MemCompare_rvv,     &MemCompareI_rvv,           &MemIsEqual_rvv,     &MemFind_rvv,       &MemFindNot_rvv,      0                   },
#elif MEM_ARCH_ARM64
    { "neon",           &MemCompare_neon,    &MemCompareI_neon,          &MemIsEqual_neon,    &MemFind_neon,      &MemFindNot_neon,     0                   },
#elif MEM_ARCH_X64
    { "sse2",           &MemCompare_sse2,    &MemCompareI_sse2,          &MemIsEqual_sse2,    &MemFind_sse2,      &MemFindNot_sse2,     0                   },
    { "avx2",           &MemCompare_avx2,    &MemCompareI_avx2,          &MemIsEqual_avx2,    &MemFind_avx2,      &MemFindNot_avx2,     MEM_CPUID_AVX2      },
    { "avx512",         &MemCompare_avx512,  &MemCompareI_avx512,        &MemIsEqual_avx512,  &MemFind_avx512,    &MemFindNot_avx512,   MEM_CPUID_AVX512    },
#endif
};

#if MEM_ARCH_X64
#    define MEMFUN_CPU_SKIP(cpuid) ((cpuid) && (MemCPUID() & (cpuid)) == 0)
#else
#    define MEMFUN_CPU_SKIP(cpuid) (0)
#endif

int main()
{
    // allocate 4 pages where first and last page is not accessible
    // only two middle pages are valid to read/write
#if defined(_WIN32)
    size_t page_size = 4096;
    char* ptr = (char*)VirtualAlloc(NULL, 4 * page_size, MEM_RESERVE, PAGE_NOACCESS);
    assert(ptr);
    char* commit = (char*)VirtualAlloc(ptr + page_size, 2 * page_size, MEM_COMMIT, PAGE_READWRITE);
    assert(commit);
#elif defined(__linux__) || defined(__APPLE__)
    size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
    char* ptr = (char*)mmap(NULL, 4 * page_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(ptr != MAP_FAILED);
    int protect = mprotect(ptr + page_size, 2 * page_size, PROT_READ | PROT_WRITE);
    assert(protect == 0);
#elif defined(__wasm__)
    size_t page_size = 4096;
    static char buffer[4*4096];
    char* ptr = buffer;
#else
    #error N/A
#endif

    int ret = EXIT_SUCCESS;

    for (size_t i=0; i<countof(memfun); i++)
    {
        if (!memfun[i].compare) continue;

        int n = printf("MemCompare_%s", memfun[i].name);
        printf("%*s", 25 - n, ": ");

        if (MEMFUN_CPU_SKIP(memfun[i].cpuid))
        {
            printf("N/A\n");
            continue;
        }

        if (!run_compare(ptr, page_size, &MemCompare_ref, memfun[i].compare))
        {
            ret = EXIT_FAILURE;
        }
        fflush(stdout);
    }

    for (size_t i=0; i<countof(memfun); i++)
    {
        if (!memfun[i].comparei) continue;

        int n = printf("MemCompareI_%s", memfun[i].name);
        printf("%*s", 25 - n, ": ");

        if (MEMFUN_CPU_SKIP(memfun[i].cpuid))
        {
            printf("N/A\n");
            continue;
        }

        if (!run_compare(ptr, page_size, &MemCompareI_ref, memfun[i].comparei))
        {
            ret = EXIT_FAILURE;
        }
        fflush(stdout);
    }

    for (size_t i=0; i<countof(memfun); i++)
    {
        if (!memfun[i].isequal) continue;

        int n = printf("MemIsEqual_%s", memfun[i].name);
        printf("%*s", 25 - n, ": ");

        if (MEMFUN_CPU_SKIP(memfun[i].cpuid))
        {
            printf("N/A\n");
            continue;
        }

        if (!run_isequal(ptr, page_size, &MemIsEqual_ref, memfun[i].isequal))
        {
            ret = EXIT_FAILURE;
        }
        fflush(stdout);
    }

    for (size_t i=0; i<countof(memfun); i++)
    {
        if (!memfun[i].find) continue;

        int n = printf("MemFind_%s", memfun[i].name);
        printf("%*s", 25 - n, ": ");

        if (MEMFUN_CPU_SKIP(memfun[i].cpuid))
        {
            printf("N/A\n");
            continue;
        }

        if (!run_find(ptr, page_size, &MemFind_ref, memfun[i].find))
        {
            ret = EXIT_FAILURE;
        }
        fflush(stdout);
    }

    for (size_t i=0; i<countof(memfun); i++)
    {
        if (!memfun[i].findnot) continue;

        int n = printf("MemFindNot_%s", memfun[i].name);
        printf("%*s", 25 - n, ": ");

        if (MEMFUN_CPU_SKIP(memfun[i].cpuid))
        {
            printf("N/A\n");
            continue;
        }

        if (!run_find(ptr, page_size, &MemFindNot_ref, memfun[i].findnot))
        {
            ret = EXIT_FAILURE;
        }
        fflush(stdout);
    }

    return ret;
}
