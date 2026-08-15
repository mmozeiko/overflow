#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic ignored "-Wunused-function"
#endif
#define _GNU_SOURCE 1

#define MEM_STATIC
#include "memfun.h"

#if defined(__GNUC__) && !defined(__clang__)
#    pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#if defined(_WIN32)
#  include <windows.h>
#  include <powrprof.h>
#  pragma comment (lib, "powrprof")
#elif defined(__linux__)
#  include <time.h>
#  include <unistd.h>
#  include <sched.h>
#  include <sys/mman.h>
#  include <sys/syscall.h>
#  include <linux/perf_event.h>
#elif defined(__APPLE__)
#  include <time.h>
#  include <dlfcn.h>
#  include <unistd.h>
#  include <pthread.h>
#  include <sys/mman.h>
#else
#  error N/A
#endif

#define countof(arr) (sizeof(arr)/sizeof(0[arr]))

#if defined(__clang__) || defined(__GNUC__)
#  define BENCH_DO_NOT_OPTIMIZE(var) __asm__ __volatile__("" : "+r"(var) : : "memory")
#else
#  define BENCH_DO_NOT_OPTIMIZE(var) do { volatile __typeof__(var) __temp__; _ReadWriteBarrier(); __temp__ = var; _ReadWriteBarrier(); } while (0)
#endif

#if defined(__x86_64__) || defined(_M_AMD64)
#  include <emmintrin.h>
#  define BENCH_MEMORY_BARRIER() _mm_mfence()
#elif defined(_M_ARM64)
#  include <intrin.h>
#  define BENCH_MEMORY_BARRIER() __dmb(_ARM64_BARRIER_ISH)
#elif defined(__clang__) || defined(__GNUC__)
#  define BENCH_MEMORY_BARRIER() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#endif

#if defined(__x86_64__) || defined(_M_AMD64)
#  if defined(__clang__) || defined(__GNUC__)
#    include <x86intrin.h>
#    include <cpuid.h>
#    define BENCH_CPUID(num, regs) __cpuid(num, regs[0], regs[1], regs[2], regs[3])
#    define BENCH_RDPRU(reg)       ({ uint32_t hi, lo; __asm__ __volatile__("rdpru" : "=a"(lo), "=d"(hi) : "c"(reg)); ((int64_t)hi << 32) | lo; })
#  else
#    include <intrin.h>
#    define BENCH_CPUID(num, regs) __cpuid(regs, num)
#    define BENCH_RDPRU(reg)       (int64_t)_rdpru(reg)
#  endif
#endif

static int64_t bench_get_ticks(void)
{
#if defined(__aarch64__) && (defined(__GNUC__) || defined(__clang__))
    int64_t reg;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(reg));
    return reg;
#elif defined(_M_ARM64) && defined(_MSC_VER)
    return _ReadStatusReg(ARM64_CNTVCT_EL0);
#elif defined(_WIN32)
    LARGE_INTEGER c;
    QueryPerformanceCounter(&c);
    return c.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}

static double bench_ticks_to_seconds(int64_t ticks)
{
    int64_t freq;
#if defined(__aarch64__) && (defined(__GNUC__) || defined(__clang__))
    __asm__ __volatile__("mrs %0, cntfrq_el0" : "=r"(freq));
#elif defined(_M_ARM64) && defined(_MSC_VER)
    freq = _ReadStatusReg(ARM64_CNTFRQ_EL0);
#elif defined(_WIN32)
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    freq = f.QuadPart;
#else
    freq = 1000000000LL;
#endif

    return (double)ticks / (double)freq;
}

#if defined(_WIN32)

#if defined(_M_AMD64) || defined(__x86_64__)
static bool use_rdpru;
#endif

static GUID* power_current_scheme;
static DWORD power_old_mode;

static void bench_init(void)
{
#if defined(_M_AMD64) || defined(__x86_64__)
    // https://en.wikipedia.org/wiki/CPUID#EAX=80000000h:_Get_Highest_Extended_Function_Implemented
    int info[4];
    BENCH_CPUID(0x80000000, info);
    if ((unsigned)info[0] >= 0x80000008)
    {
        // https://en.wikipedia.org/wiki/CPUID#EAX=80000008h:_Virtual_and_Physical_address_Sizes
        BENCH_CPUID(0x80000008, info);
        if (info[1] & 0x10)
        {
#if defined(__clang__) || defined(__GNUC__) // currently MSVC has bug with RDPRU codegen, cannot use it
            use_rdpru = true;
#endif
        }
    }
#endif

    // pin current thread to one core
    HANDLE thread = GetCurrentThread();
    DWORD cpu_index = SetThreadIdealProcessor(thread, MAXIMUM_PROCESSORS);
    SetThreadAffinityMask(thread, 1ULL << cpu_index);

    // disable turbo-boost
    PowerGetActiveScheme(NULL, &power_current_scheme);
    PowerReadACValueIndex(NULL, power_current_scheme, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_PERF_BOOST_MODE, &power_old_mode);
    PowerWriteACValueIndex(NULL, power_current_scheme, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_PERF_BOOST_MODE, PROCESSOR_PERF_BOOST_MODE_DISABLED);
    PowerSetActiveScheme(NULL, power_current_scheme);
}

static void bench_done(void)
{
    // restore old turbo-boost setting
    PowerWriteACValueIndex(NULL, power_current_scheme, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_PERF_BOOST_MODE, power_old_mode);
    //PowerWriteACValueIndex(NULL, power_current_scheme, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_PERF_BOOST_MODE, PROCESSOR_PERF_BOOST_MODE_ENABLED);
    PowerSetActiveScheme(NULL, power_current_scheme);
    LocalFree(power_current_scheme);
    power_current_scheme = NULL;
}

static int64_t bench_read_cycle_counter(void)
{
#if defined(_M_AMD64) || defined(__x86_64__)
    return use_rdpru ? BENCH_RDPRU(1) : (int64_t)__rdtsc();
#elif defined(_M_ARM64) || defined(__aarch64__)
    return _ReadStatusReg(ARM64_PMCCNTR_EL0);
#else
#   error Not supported for this target!
#endif
}

#elif defined(__linux__)

static void bench_init(void)
{
    // pin current thread to one core
    size_t cpu = (size_t)sched_getcpu();

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    // check if cpufreq governor is set to performance
    {
        char governor_path[1024];
        snprintf(governor_path, sizeof(governor_path), "/sys/devices/system/cpu/cpu%zu/cpufreq/scaling_governor", cpu);
        FILE* f = fopen(governor_path, "r");
        if (f)
        {
            char line[128];
            if (fgets(line, sizeof(line), f) && strcmp(line, "performance\n") != 0)
            {
                fprintf(stderr, "WARNING: cpufreq governor is not set to performance, results may vary due cpu frequency changes!\n");
                fprintf(stderr, "Set cpufreq governor to \"performance\" by running the following command:\n");
                fprintf(stderr, "sudo cpupower frequency-set -g performance\n\n");
            }
            fclose(f);
        }
    }

    // check if cpufreq boosting is allowed
    {
        FILE* f = fopen("/sys/devices/system/cpu/cpufreq/boost", "r");
        if (f)
        {
            int value;
            if (fscanf(f, "%d", &value) == 1 && value != 0)
            {
                fprintf(stderr, "WARNING: cpufreq boosting is allowed, to disable run the following command:\n");
                fprintf(stderr, "echo 0 | sudo tee /sys/devices/system/cpu/cpufreq/boost\n\n");
            }
            fclose(f);
        }
    }

#if defined(__x86_64__)

    // check if Intel Turbo Boost is turned off
    {
        FILE* f = fopen("/sys/devices/system/cpu/intel_pstate/no_turbo", "r");
        if (f)
        {
            int value;
            if (fscanf(f, "%d", &value) == 1 && value != 1)
            {
                fprintf(stderr, "WARNING: Intel Turbo Boost is enabled, to disable run the following command:\n");
                fprintf(stderr, "echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo\n\n");
            }
            fclose(f);
        }
    }

    // check if AMD Core Performance Boost is turned off
    {
        FILE* f = fopen("/sys/devices/system/cpu/amd_pstate/cpb_boost", "r");
        if (f)
        {
            int value;
            if (fscanf(f, "%d", &value) == 1 && value != 0)
            {
                fprintf(stderr, "WARNING: AMD Core Performance Boost is enabled, to disable run the following command:\n");
                fprintf(stderr, "echo 0 | sudo tee /sys/devices/system/cpu/amd_pstate/cpb_boost\n\n");
            }
            fclose(f);
        }
    }

#elif defined(__aarch64__)

    // setup armv8 PMU cycle counter to be accessible from user-space
    {
        struct perf_event_attr attr = {};
        attr.size = sizeof(attr);
        attr.type = PERF_TYPE_HARDWARE;
        attr.config = PERF_COUNT_HW_CPU_CYCLES;
        attr.config1 = 1 | 2; // 1=64-bit counters, 2=allow user access

        int fd = (int)syscall(__NR_perf_event_open, &attr, 0, cpu, -1, 0);
        if (fd < 0)
        {
            fprintf(stderr, "ERROR: failed to initialize perf, not enabled in kernel, or requires root!\n");
            fprintf(stderr, "To allow non-root access, run the following command:\n");
            fprintf(stderr, "echo 1 | sudo tee /proc/sys/kernel/perf_event_paranoid\n");
            exit(1);
        }

        uint64_t reg;
        __asm__ __volatile__("mrs %0, pmuserenr_el0" : "=r"(reg));
        if (!(reg & 4))
        {
            fprintf(stderr, "ERROR: PMU not allowed for user-space access, to allow run the following command:\n");
            fprintf(stderr, "echo 1 | sudo tee /proc/sys/kernel/perf_user_access\n");
            exit(1);
        }
    }

#elif defined(__riscv)

    // setup RISC-V PMU cycle counter to be accessible from user-space
    {
        struct perf_event_attr attr =
        {
            .size = sizeof(attr),
            .type = PERF_TYPE_HARDWARE,
            .config = PERF_COUNT_HW_CPU_CYCLES,
            .exclude_kernel = 1,
            .exclude_hv = 1,
        };

        int fd = (int)syscall(__NR_perf_event_open, &attr, 0, cpu, -1, 0);
        if (fd < 0)
        {
            fprintf(stderr, "WARNING: failed to initialize perf, not enabled in kernel, or requires root!\n");
            fprintf(stderr, "To allow non-root access, run the following command:\n");
            fprintf(stderr, "echo 1 | sudo tee /proc/sys/kernel/perf_event_paranoid\n");
        }
        else
        {
            struct perf_event_mmap_page* perf_page = mmap(NULL, (size_t)getpagesize(), PROT_READ, MAP_SHARED, fd, 0);
            if (!perf_page || perf_page->cap_user_rdpmc == 0)
            {
                fprintf(stderr, "WARNING: PMU not allowed for user-space access, to allow run the following command:\n");
                fprintf(stderr, "echo 1 | sudo tee /proc/sys/kernel/perf_user_access\n\n");
            }
        }
    }

#endif
}

static void bench_done(void)
{
}

static int64_t bench_read_cycle_counter(void)
{
#if defined(__linux__) && defined(__aarch64__)
    int64_t reg;
    __asm__ __volatile__("mrs %0, pmccntr_el0" : "=r"(reg));
    return reg;
#elif defined(__linux__) && defined(__x86_64__)
    return (int64_t)__rdtsc();
#elif defined(__linux__) && defined(__riscv)
    int64_t reg;
    __asm__ __volatile__("rdcycle %0" : "=r"(reg));
    return reg;
#else
#   error Not supported for this target!
#endif
}

#elif defined(__APPLE__)

typedef struct kpep_db     kpep_db;
typedef struct kpep_event  kpep_event;
typedef struct kpep_config kpep_config;
typedef uint64_t           kpc_config_t;

#define KPC_MAX_COUNTERS            32
#define KPC_CLASS_CONFIGURABLE      (1)
#define KPC_CLASS_CONFIGURABLE_MASK (1U << KPC_CLASS_CONFIGURABLE)

#define KPERF_FUNCS(X)                                                                                      \
    X(int,  kpc_force_all_ctrs_get,     int* value)                                                         \
    X(int,  kpc_force_all_ctrs_set,     int value)                                                          \
    X(int,  kpc_set_config,             uint32_t classes, kpc_config_t* config)                             \
    X(int,  kpc_set_counting,           uint32_t classes)                                                   \
    X(int,  kpc_set_thread_counting,    uint32_t classes)                                                   \
    X(int,  kpc_get_thread_counters,    uint32_t tid, uint32_t buf_count, void* buf)                        \

#define KPERFDATA_FUNCS(X)                                                                                  \
    X(int,  kpep_db_create,             const char *name, kpep_db** db)                                     \
    X(int,  kpep_db_event,              kpep_db* db, const char* name, kpep_event** ev)                     \
    X(void, kpep_db_free,               kpep_db* db)                                                        \
    X(int,  kpep_config_create,         kpep_db* db, kpep_config** config)                                  \
    X(int,  kpep_config_force_counters, kpep_config* cfg)                                                   \
    X(int,  kpep_config_add_event,      kpep_config* cfg, kpep_event** ev, uint32_t flag, uint32_t* err)    \
    X(int,  kpep_config_kpc_classes,    kpep_config* cfg, uint32_t* classes)                                \
    X(int,  kpep_config_kpc_count,      kpep_config* cfg, size_t* count)                                    \
    X(int,  kpep_config_kpc_map,        kpep_config* cfg, void* buf, size_t buf_size)                       \
    X(int,  kpep_config_kpc,            kpep_config* cfg, kpc_config_t* buf, size_t buf_size)               \
    X(void, kpep_config_free,           kpep_config *cfg)                                                   \

#define X(ret, name, ...) static ret (*name)(__VA_ARGS__);
KPERF_FUNCS(X)
KPERFDATA_FUNCS(X)
#undef X

static void bench_init(void)
{
    void* kperf = dlopen("/System/Library/PrivateFrameworks/kperf.framework/kperf", RTLD_LAZY | RTLD_LOCAL);
    assert(kperf);

#define X(ret, name, ...) name = (ret (*)(__VA_ARGS__))dlsym(kperf, #name); assert(name);
    KPERF_FUNCS(X)
#undef X

    void* kperfdata = dlopen("/System/Library/PrivateFrameworks/kperfdata.framework/kperfdata", RTLD_LAZY | RTLD_LOCAL);
    assert(kperfdata);

#define X(ret, name, ...) name = (ret (*)(__VA_ARGS__))dlsym(kperfdata, #name); assert(name);
    KPERFDATA_FUNCS(X)
#undef X

    uint32_t     counter_classes;
    size_t       counter_reg_count;
    size_t       counter_map[KPC_MAX_COUNTERS];
    kpc_config_t counter_regs[KPC_MAX_COUNTERS];

    kpep_db*     db;
    kpep_config* config;
    kpep_event*  event;
    int ret;

    ret = kpep_db_create(NULL, &db);                                        assert(!ret && "kpep_db_create failed");
    ret = kpep_config_create(db, &config);                                  assert(!ret && "kpep_config_create failed");
    ret = kpep_config_force_counters(config);                               assert(!ret && "kpep_config_force_counters failed");
    ret = kpep_db_event(db, "FIXED_CYCLES", &event);                        assert(!ret && "kpep_db_event failed");
    ret = kpep_config_add_event(config, &event, 1, NULL);                   assert(!ret && "kpep_config_add_event failed");
    ret = kpep_config_kpc_classes(config, &counter_classes);                assert(!ret && "kpep_config_kpc_classes failed");
    ret = kpep_config_kpc_count(config, &counter_reg_count);                assert(!ret && "kpep_config_kpc_count failed");
    ret = kpep_config_kpc_map(config, counter_map, sizeof(counter_map));    assert(!ret && "kpep_config_kpc_map failed");
    ret = kpep_config_kpc(config, counter_regs, sizeof(counter_regs));      assert(!ret && "kpep_config_kpc failed");

    kpep_config_free(config);
    kpep_db_free(db);

    int value;
    if (kpc_force_all_ctrs_get(&value) != 0)
    {
        fprintf(stderr, "ERROR: cannot use PMU, this requires running with root privileges - use sudo!\n");
        exit(1);
    }

    pthread_set_qos_class_self_np(getenv("ECORE") ? QOS_CLASS_BACKGROUND : QOS_CLASS_USER_INTERACTIVE, 0);

    kpc_force_all_ctrs_set(1);
    if ((counter_classes & KPC_CLASS_CONFIGURABLE_MASK) && counter_reg_count)
    {
        kpc_set_config(counter_classes, counter_regs);
    }
    kpc_set_counting(counter_classes);
    kpc_set_thread_counting(counter_classes);
}

static void bench_done(void)
{
}

static int64_t bench_read_cycle_counter(void)
{
    int64_t counters[KPC_MAX_COUNTERS];
    kpc_get_thread_counters(0, KPC_MAX_COUNTERS, counters);
    return counters[0];
}

#else

#error N/A

#endif


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
        uint8_t c1 = MemToLower1(p1[i]);
        uint8_t c2 = MemToLower1(p2[i]);
        if (c1 < c2) return -1;
        if (c1 > c2) return +1;
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
    const void* r = memchr(ptr, value, size);
    return r ? (size_t)((char*)r - (char*)ptr) : size;
}

typedef int    MemCompareFun(const void* ptr1, const void* ptr2, size_t size);
typedef bool   MemIsEqualFun(const void* ptr1, const void* ptr2, size_t size);
typedef size_t MemFindFun   (const void* ptr, size_t size, uint8_t value);

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
#if MEM_ARCH_RVV
    { "rvv",            &MemCompare_rvv,     &MemCompareI_rvv,           &MemIsEqual_rvv,     &MemFind_rvv,       &MemFindNot_rvv,      0                   },
#elif MEM_ARCH_ARM64
    { "neon",           &MemCompare_neon,    &MemCompareI_neon,          &MemIsEqual_neon,    &MemFind_neon,      &MemFindNot_neon,     0                   },
#elif MEM_ARCH_X64
    { "sse2",           &MemCompare_sse2,    &MemCompareI_sse2,          &MemIsEqual_sse2,    &MemFind_sse2,      &MemFindNot_sse2,     0                   },
    { "avx2",           &MemCompare_avx2,    &MemCompareI_avx2,          &MemIsEqual_avx2,    &MemFind_avx2,      &MemFindNot_avx2,     MEM_CPUID_AVX2      },
    { "avx512",         &MemCompare_avx512,  &MemCompareI_avx512,        &MemIsEqual_avx512,  &MemFind_avx512,    &MemFindNot_avx512,   MEM_CPUID_AVX512    },
#endif
    { "generic",        &MemCompare_generic, &MemCompareI_generic,       &MemIsEqual_generic, &MemFind_generic,   &MemFindNot_generic,  0                   },
};

#define BENCH_TINY_LIMIT  1024
#define BENCH_SMALL_LIMIT (64*1024)

#define BENCH_TINY_COUNT  1000000
#define BENCH_SMALL_COUNT 100000
#define BENCH_LARGE_COUNT 1000

#define BENCH_ITER_COUNT  8

static const size_t bench_sizes[] =
{
    // tiny size
    1, 3, 7, 15, 31, 63, 256, 512,
    // small size
    1024, 2*1024, 8*1024, 16*1024,
    // large size
    512*1024,
#if MEM_ARCH_X64
    2*1024*1024,
#endif
};

static size_t bench_index;
static size_t bench_variant;

static struct
{
    double bpc;
    double mbps;
}
bench_results[5][countof(memfun)][countof(bench_sizes)];

typedef struct {

    size_t iter_index;
    size_t iter_count;

    size_t size_index;
    size_t size_count;
    size_t size_limit;

    size_t unroll_count;

    int64_t ticks;
    int64_t counter;

    int64_t best_ticks;
    int64_t best_counter;

} bench_context;

static bool bench_begin(bench_context* ctx, const char* name, const char* suffix, int cpuid)
{
    (void)cpuid;

#if MEM_ARCH_X64
    if (cpuid && ((MemCPUID() & cpuid) == 0))
    {
        return false;
    }
#endif

    memset(ctx, 0, sizeof(*ctx));

    if (strcmp(name, "MemCompareI") == 0 && strcmp(suffix, "std") == 0)
    {
        ctx->size_limit = 512;
    }
    else if (strcmp(suffix, "generic") == 0)
    {
        ctx->size_limit = bench_sizes[countof(bench_sizes)-1];
    }

    printf("=== %s_%s\n", name, suffix);

    printf("%8s | %10s | %5s | %6s\n", "bytes", "cycles", "b/c", "MB/s");
    for (int i=0; i<8+10+5+6+1+3*3; i++) printf("-");
    printf("\n");
    fflush(stdout);

    return true;
}

static bool bench_loop(bench_context* ctx, size_t* size, size_t* unroll)
{
    size_t iter_count = ctx->iter_count;
    if (iter_count == 0)
    {
        ctx->iter_index = 0;
        ctx->size_index = 0;

        size_t s = *size = bench_sizes[0];
        ctx->iter_count = BENCH_ITER_COUNT;
        ctx->size_count = countof(bench_sizes);
        ctx->unroll_count = *unroll = s < BENCH_TINY_LIMIT ? BENCH_TINY_COUNT : s < BENCH_SMALL_LIMIT ? BENCH_SMALL_COUNT : BENCH_LARGE_COUNT;

        ctx->best_ticks   = LLONG_MAX;
        ctx->best_counter = LLONG_MAX;

        BENCH_MEMORY_BARRIER();
        ctx->ticks   = bench_get_ticks();
        ctx->counter = bench_read_cycle_counter();

        return true;
    }

    {
        BENCH_MEMORY_BARRIER();
        int64_t counter = bench_read_cycle_counter();
        int64_t ticks   = bench_get_ticks();

        ticks   -= ctx->ticks;
        counter -= ctx->counter;

        if (ticks < ctx->best_ticks)
        {
            ctx->best_ticks   = ticks;
            ctx->best_counter = counter;
        }
    }

    size_t iter_index = ctx->iter_index;
    if (++iter_index < ctx->iter_count)
    {
        ctx->iter_index = iter_index;

        BENCH_MEMORY_BARRIER();
        ctx->ticks   = bench_get_ticks();
        ctx->counter = bench_read_cycle_counter();

        return true;
    }

    size_t size_index = ctx->size_index;
    {
        size_t s = bench_sizes[size_index];

        double cycles  = (double)ctx->best_counter / (double)ctx->unroll_count;
        double seconds = bench_ticks_to_seconds(ctx->best_ticks) / (double)ctx->unroll_count;

        double bpc = (double)s / cycles;
        double mbps = (double)s / seconds / (1024.0 * 1024.0);

        printf("%8zu | %10.1f | %5.2f | %6.0f\n", s, cycles, bpc, mbps);
        fflush(stdout);

        bench_results[bench_index][bench_variant][size_index].bpc  = bpc;
        bench_results[bench_index][bench_variant][size_index].mbps = mbps;
    }

    if (++size_index < ctx->size_count)
    {
        ctx->size_index = size_index;
        ctx->iter_index = 0;

        size_t s = *size = bench_sizes[size_index];
        if (ctx->size_limit && s >= ctx->size_limit)
        {
            printf("\n");
            fflush(stdout);

            return false;
        }
        ctx->unroll_count = *unroll = s < BENCH_TINY_LIMIT ? BENCH_TINY_COUNT : s < BENCH_SMALL_LIMIT ? BENCH_SMALL_COUNT : BENCH_LARGE_COUNT;

        ctx->best_ticks   = LLONG_MAX;
        ctx->best_counter = LLONG_MAX;

        BENCH_MEMORY_BARRIER();
        ctx->ticks   = bench_get_ticks();
        ctx->counter = bench_read_cycle_counter();

        return true;
    }

    printf("\n");
    fflush(stdout);

    return false;
}

int main()
{
    size_t max_size = bench_sizes[countof(bench_sizes)-1];

#if defined(_WIN32)
    char* ptr = (char*)VirtualAlloc(NULL, 2 * max_size, MEM_COMMIT, PAGE_READWRITE);
    assert(ptr != NULL);
#elif defined(__linux__) || defined(__APPLE__)
    char* ptr = (char*)mmap(NULL, 2 * max_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(ptr != MAP_FAILED);
#else
    #error N/A
#endif

    memset(ptr, 0xff, 2 * max_size);
    char* ptr1 = ptr;
    char* ptr2 = ptr + max_size;

    bench_init();

    for (size_t i=0; i<countof(memfun); i++)
    {
        MemCompareFun* fun = memfun[i].compare;

        bench_context ctx;
        if (fun && bench_begin(&ctx, "MemCompare", memfun[i].name, memfun[i].cpuid))
        {
            size_t size, unroll;
            while (bench_loop(&ctx, &size, &unroll))
            {
                for (size_t u=0; u<unroll; u++)
                {
                    int result = fun(ptr1, ptr2, size);
                    BENCH_DO_NOT_OPTIMIZE(result);
                }
            }
        }
        bench_variant++;
    }
    bench_variant = 0;
    bench_index++;

    for (size_t i=0; i<countof(memfun); i++)
    {
        MemCompareFun* fun = memfun[i].comparei;

        bench_context ctx;
        if (fun && bench_begin(&ctx, "MemCompareI", memfun[i].name, memfun[i].cpuid))
        {
            size_t size, unroll;
            while (bench_loop(&ctx, &size, &unroll))
            {
                for (size_t u=0; u<unroll; u++)
                {
                    int result = fun(ptr1, ptr2, size);
                    BENCH_DO_NOT_OPTIMIZE(result);
                }
            }
        }
        bench_variant++;
    }
    bench_variant = 0;
    bench_index++;

    for (size_t i=0; i<countof(memfun); i++)
    {
        MemIsEqualFun* fun = memfun[i].isequal;

        bench_context ctx;
        if (fun && bench_begin(&ctx, "MemIsEqual", memfun[i].name, memfun[i].cpuid))
        {
            size_t size, unroll;
            while (bench_loop(&ctx, &size, &unroll))
            {
                for (size_t u=0; u<unroll; u++)
                {
                    bool result = fun(ptr1, ptr2, size);
                    BENCH_DO_NOT_OPTIMIZE(result);
                }
            }
        }
        bench_variant++;
    }
    bench_variant = 0;
    bench_index++;

    for (size_t i=0; i<countof(memfun); i++)
    {
        MemFindFun* fun = memfun[i].find;

        bench_context ctx;
        if (fun && bench_begin(&ctx, "MemFind", memfun[i].name, memfun[i].cpuid))
        {
            size_t size, unroll;
            while (bench_loop(&ctx, &size, &unroll))
            {
                for (size_t u=0; u<unroll; u++)
                {
                    size_t result = fun(ptr1, size, 0);
                    BENCH_DO_NOT_OPTIMIZE(result);
                }
            }
        }
        bench_variant++;
    }
    bench_variant = 0;
    bench_index++;

    for (size_t i=0; i<countof(memfun); i++)
    {
        MemFindFun* fun = memfun[i].findnot;

        bench_context ctx;
        if (fun && bench_begin(&ctx, "MemFindNot", memfun[i].name, memfun[i].cpuid))
        {
            size_t size, unroll;
            while (bench_loop(&ctx, &size, &unroll))
            {
                for (size_t u=0; u<unroll; u++)
                {
                    size_t result = fun(ptr1, size, 0xff);
                    BENCH_DO_NOT_OPTIMIZE(result);
                }
            }
        }
        bench_variant++;
    }
    bench_variant = 0;
    bench_index++;

    bench_done();

    {
        static const char* names[] = { "MemCompare", "MemCompareI", "MemIsEqual", "MemFind", "MemFindNot" };
        static const size_t sizes[] = { 15, 63, 1024, 16384 };

        for (size_t n=0; n<countof(names); n++)
        {
            for (size_t s=0; s<countof(sizes); s++)
            {
                for (size_t i=0; i<countof(bench_sizes); i++)
                {
                    if (bench_sizes[i] == sizes[s])
                    {
                        printf("%-14s | %5zu", names[n], sizes[s]);

                        for (size_t t=0; t<countof(memfun)-1; t++)
                        {
                            if (strcmp(names[n], "MemCompareI") == 0 && t == 0 && sizes[s] > 64)
                            {
                                printf(" | %-19s", "(slow)");
                            }
                            else if (strcmp(names[n], "MemFindNot") == 0 && t == 0)
                            {
                                printf(" | %-19s", "(n/a)");
                            }
                            else
                            {
                                printf(" | %5.2f @ %6.0f MB/s", bench_results[n][t][i].bpc, bench_results[n][t][i].mbps);
                            }
                        }

                        printf("\n");
                        break;
                    }
                }
            }
        }
    }
}
