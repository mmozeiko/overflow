static int test_cpuid_mask;
static int test_cpuid_value;
#define MD5_CPUID_MASK    test_cpuid_mask
#define SHA1_CPUID_MASK   test_cpuid_mask
#define SHA256_CPUID_MASK test_cpuid_mask
#define SHA512_CPUID_MASK test_cpuid_mask

#include "../md5.h"
#include "../sha1.h"
#include "../sha256.h"
#include "../sha512.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    const char* digest;
    const char* input;
    size_t input_size;
} hash_test;

#define TEST_STR(str) str, sizeof(str)-1

static const hash_test md5_tests[] =
{
    // https://www.rfc-editor.org/rfc/rfc1321.html#appendix-A.5
    { "\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e", TEST_STR("") },
    { "\x0c\xc1\x75\xb9\xc0\xf1\xb6\xa8\x31\xc3\x99\xe2\x69\x77\x26\x61", TEST_STR("a") },
    { "\x90\x01\x50\x98\x3c\xd2\x4f\xb0\xd6\x96\x3f\x7d\x28\xe1\x7f\x72", TEST_STR("abc") },
    { "\xf9\x6b\x69\x7d\x7c\xb7\x93\x8d\x52\x5a\x2f\x31\xaa\xf1\x61\xd0", TEST_STR("message digest") },
    { "\xc3\xfc\xd3\xd7\x61\x92\xe4\x00\x7d\xfb\x49\x6c\xca\x67\xe1\x3b", TEST_STR("abcdefghijklmnopqrstuvwxyz") },
    { "\xd1\x74\xab\x98\xd2\x77\xd9\xf5\xa5\x61\x1c\x2c\x9f\x41\x9d\x9f", TEST_STR("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789") },
    { "\x57\xed\xf4\xa2\x2b\xe3\xc9\x55\xac\x49\xda\x2e\x21\x07\xb6\x7a", TEST_STR("12345678901234567890123456789012345678901234567890123456789012345678901234567890") },

    // https://www.nist.gov/itl/ssd/software-quality-group/nsrl-test-data
    #include "generated/md5_nsrl.h"

    { NULL, NULL, 0 },
};

static const hash_test sha1_tests[] =
{
    // https://en.wikipedia.org/wiki/SHA-1#Example_hashes
    { "\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18\x90\xaf\xd8\x07\x09", TEST_STR("") },
    { "\x2f\xd4\xe1\xc6\x7a\x2d\x28\xfc\xed\x84\x9e\xe1\xbb\x76\xe7\x39\x1b\x93\xeb\x12", TEST_STR("The quick brown fox jumps over the lazy dog") },
    { "\xde\x9f\x2c\x7f\xd2\x5e\x1b\x3a\xfa\xd3\xe8\x5a\x0b\xd1\x7d\x9b\x10\x0d\xb4\xb3", TEST_STR("The quick brown fox jumps over the lazy cog") },

    // https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHA1.pdf
    { "\xa9\x99\x3e\x36\x47\x06\x81\x6a\xba\x3e\x25\x71\x78\x50\xc2\x6c\x9c\xd0\xd8\x9d", TEST_STR("abc") },
    { "\x84\x98\x3e\x44\x1c\x3b\xd2\x6e\xba\xae\x4a\xa1\xf9\x51\x29\xe5\xe5\x46\x70\xf1", TEST_STR("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") },

    // https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing
    #include "generated/sha1_cavp.h"

    { NULL, NULL, 0 },
};

static const hash_test sha256_tests[] =
{
    // https://en.wikipedia.org/wiki/SHA-2#Test_vectors
    { "\xe3\xb0\xc4\x42\x98\xfc\x1c\x14\x9a\xfb\xf4\xc8\x99\x6f\xb9\x24\x27\xae\x41\xe4\x64\x9b\x93\x4c\xa4\x95\x99\x1b\x78\x52\xb8\x55", TEST_STR("") },

    // https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHA256.pdf
    { "\xba\x78\x16\xbf\x8f\x01\xcf\xea\x41\x41\x40\xde\x5d\xae\x22\x23\xb0\x03\x61\xa3\x96\x17\x7a\x9c\xb4\x10\xff\x61\xf2\x00\x15\xad", TEST_STR("abc") },
    { "\x24\x8d\x6a\x61\xd2\x06\x38\xb8\xe5\xc0\x26\x93\x0c\x3e\x60\x39\xa3\x3c\xe4\x59\x64\xff\x21\x67\xf6\xec\xed\xd4\x19\xdb\x06\xc1", TEST_STR("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") },

    // https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing
    #include "generated/sha256_cavp.h"

    { NULL, NULL, 0 },
};

static const hash_test sha224_tests[] =
{
    // https://en.wikipedia.org/wiki/SHA-2#Test_vectors
    { "\xd1\x4a\x02\x8c\x2a\x3a\x2b\xc9\x47\x61\x02\xbb\x28\x82\x34\xc4\x15\xa2\xb0\x1f\x82\x8e\xa6\x2a\xc5\xb3\xe4\x2f", TEST_STR("") },
    { "\x73\x0e\x10\x9b\xd7\xa8\xa3\x2b\x1c\xb9\xd9\xa0\x9a\xa2\x32\x5d\x24\x30\x58\x7d\xdb\xc0\xc3\x8b\xad\x91\x15\x25", TEST_STR("The quick brown fox jumps over the lazy dog") },
    { "\x61\x9c\xba\x8e\x8e\x05\x82\x6e\x9b\x8c\x51\x9c\x0a\x5c\x68\xf4\xfb\x65\x3e\x8a\x3d\x8a\xa0\x4b\xb2\xc8\xcd\x4c", TEST_STR("The quick brown fox jumps over the lazy dog.") },

    // https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHA224.pdf
    { "\x23\x09\x7d\x22\x34\x05\xd8\x22\x86\x42\xa4\x77\xbd\xa2\x55\xb3\x2a\xad\xbc\xe4\xbd\xa0\xb3\xf7\xe3\x6c\x9d\xa7", TEST_STR("abc") },
    { "\x75\x38\x8b\x16\x51\x27\x76\xcc\x5d\xba\x5d\xa1\xfd\x89\x01\x50\xb0\xc6\x45\x5c\xb4\xf5\x8b\x19\x52\x52\x25\x25", TEST_STR("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") },

    // https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing
    #include "generated/sha224_cavp.h"

    { NULL, NULL, 0 },
};

static const hash_test sha512_tests[] =
{
    // https://en.wikipedia.org/wiki/SHA-2#Test_vectors
    { "\xcf\x83\xe1\x35\x7e\xef\xb8\xbd\xf1\x54\x28\x50\xd6\x6d\x80\x07\xd6\x20\xe4\x05\x0b\x57\x15\xdc\x83\xf4\xa9\x21\xd3\x6c\xe9\xce\x47\xd0\xd1\x3c\x5d\x85\xf2\xb0\xff\x83\x18\xd2\x87\x7e\xec\x2f\x63\xb9\x31\xbd\x47\x41\x7a\x81\xa5\x38\x32\x7a\xf9\x27\xda\x3e", TEST_STR("") },

    // https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHA512.pdf
    { "\xdd\xaf\x35\xa1\x93\x61\x7a\xba\xcc\x41\x73\x49\xae\x20\x41\x31\x12\xe6\xfa\x4e\x89\xa9\x7e\xa2\x0a\x9e\xee\xe6\x4b\x55\xd3\x9a\x21\x92\x99\x2a\x27\x4f\xc1\xa8\x36\xba\x3c\x23\xa3\xfe\xeb\xbd\x45\x4d\x44\x23\x64\x3c\xe8\x0e\x2a\x9a\xc9\x4f\xa5\x4c\xa4\x9f", TEST_STR("abc") },
    { "\x8e\x95\x9b\x75\xda\xe3\x13\xda\x8c\xf4\xf7\x28\x14\xfc\x14\x3f\x8f\x77\x79\xc6\xeb\x9f\x7f\xa1\x72\x99\xae\xad\xb6\x88\x90\x18\x50\x1d\x28\x9e\x49\x00\xf7\xe4\x33\x1b\x99\xde\xc4\xb5\x43\x3a\xc7\xd3\x29\xee\xb6\xdd\x26\x54\x5e\x96\xe5\x5b\x87\x4b\xe9\x09", TEST_STR("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu") },

    // https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing
    #include "generated/sha512_cavp.h"

    { NULL, NULL, 0 },
};

static const hash_test sha384_tests[] =
{
    // https://en.wikipedia.org/wiki/SHA-2#Test_vectors
    { "\x38\xb0\x60\xa7\x51\xac\x96\x38\x4c\xd9\x32\x7e\xb1\xb1\xe3\x6a\x21\xfd\xb7\x11\x14\xbe\x07\x43\x4c\x0c\xc7\xbf\x63\xf6\xe1\xda\x27\x4e\xde\xbf\xe7\x6f\x65\xfb\xd5\x1a\xd2\xf1\x48\x98\xb9\x5b", TEST_STR("") },

    // https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Standards-and-Guidelines/documents/examples/SHA384.pdf
    { "\xcb\x00\x75\x3f\x45\xa3\x5e\x8b\xb5\xa0\x3d\x69\x9a\xc6\x50\x07\x27\x2c\x32\xab\x0e\xde\xd1\x63\x1a\x8b\x60\x5a\x43\xff\x5b\xed\x80\x86\x07\x2b\xa1\xe7\xcc\x23\x58\xba\xec\xa1\x34\xc8\x25\xa7", TEST_STR("abc") },
    { "\x09\x33\x0c\x33\xf7\x11\x47\xe8\x3d\x19\x2f\xc7\x82\xcd\x1b\x47\x53\x11\x1b\x17\x3b\x3b\x05\xd2\x2f\xa0\x80\x86\xe3\xb0\xf7\x12\xfc\xc7\xc7\x1a\x55\x7e\x2d\xb9\x66\xc3\xe9\xfa\x91\x74\x60\x39", TEST_STR("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu") },

    // https://csrc.nist.gov/projects/cryptographic-algorithm-validation-program/secure-hashing
    #include "generated/sha384_cavp.h"

    { NULL, NULL, 0 },
};

typedef struct {
    size_t index;
    const void* digest;
    const void* input;
    size_t input_size;
} test;

static bool test_step(test* t, const char* name, int cpuid, const hash_test tests[])
{
    size_t index;

    if (t->digest == NULL)
    {
        printf("%-15s ... ", name);
        if (cpuid != 0)
        {
            if ((test_cpuid_value & cpuid) == 0)
            {
                printf("N/A\n");
                return false;
            }
            test_cpuid_mask = cpuid;
        }
        index = 0;
    }
    else
    {
        index = ++t->index;
    }

    t->digest = tests[index].digest;
    if (t->digest == NULL)
    {
        printf("OK\n");
        t->index = 0;
        return false;
    }

    t->input = tests[index].input;
    t->input_size = tests[index].input_size;

    return true;
}

static void test_check(test* t, const void* digest, size_t digest_size)
{
    if (memcmp(t->digest, digest, digest_size) != 0)
    {
        printf("test %zu failed!\n", t->index);
        exit(1);
    }
}

int main()
{
    test_cpuid_mask = 0;

    test t;
    memset(&t, 0, sizeof(t));

    //
    // md5
    //

#if defined(MD5_CPUID_INIT)
    test_cpuid_mask = ~0;
    test_cpuid_value = md5_cpuid();
    test_cpuid_mask = 0;
#endif

    while (test_step(&t, "md5", 0, md5_tests))
    {
        uint8_t digest[MD5_DIGEST_SIZE];

        md5_ctx ctx;
        md5_init(&ctx);
        md5_update(&ctx, t.input, t.input_size);
        md5_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }

#if defined(MD5_CPUID_BMI2)
    while (test_step(&t, "md5(bmi2)", MD5_CPUID_BMI2, md5_tests))
    {
        uint8_t digest[MD5_DIGEST_SIZE];

        md5_ctx ctx;
        md5_init(&ctx);
        md5_update(&ctx, t.input, t.input_size);
        md5_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }
#endif

    //
    // sha1
    //

#if defined(SHA1_CPUID_INIT)
    test_cpuid_mask = ~0;
    test_cpuid_value = sha1_cpuid();
    test_cpuid_mask = 0;
#endif

    while (test_step(&t, "sha1", 0, sha1_tests))
    {
        uint8_t digest[SHA1_DIGEST_SIZE];

        sha1_ctx ctx;
        sha1_init(&ctx);
        sha1_update(&ctx, t.input, t.input_size);
        sha1_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }

#if defined(SHA1_CPUID_SHANI)
    while (test_step(&t, "sha1(shani)", SHA1_CPUID_SHANI, sha1_tests))
    {
        uint8_t digest[SHA1_DIGEST_SIZE];

        sha1_ctx ctx;
        sha1_init(&ctx);
        sha1_update(&ctx, t.input, t.input_size);
        sha1_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }
#endif

#if defined(SHA1_CPUID_ARM64)
    while (test_step(&t, "sha1(arm64)", SHA1_CPUID_ARM64, sha1_tests))
    {
        uint8_t digest[SHA1_DIGEST_SIZE];

        sha1_ctx ctx;
        sha1_init(&ctx);
        sha1_update(&ctx, t.input, t.input_size);
        sha1_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }
#endif

    //
    // sha256
    //

#if defined(SHA256_CPUID_INIT)
    test_cpuid_mask = ~0;
    test_cpuid_value = sha256_cpuid();
    test_cpuid_mask = 0;
#endif

    while (test_step(&t, "sha256", 0, sha256_tests))
    {
        uint8_t digest[SHA256_DIGEST_SIZE];

        sha256_ctx ctx;
        sha256_init(&ctx);
        sha256_update(&ctx, t.input, t.input_size);
        sha256_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }

    while (test_step(&t, "sha224", 0, sha224_tests))
    {
        uint8_t digest[SHA224_DIGEST_SIZE];

        sha224_ctx ctx;
        sha224_init(&ctx);
        sha224_update(&ctx, t.input, t.input_size);
        sha224_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }

#if defined(SHA256_CPUID_SHANI)
    while (test_step(&t, "sha256(shani)", SHA256_CPUID_SHANI, sha256_tests))
    {
        uint8_t digest[SHA256_DIGEST_SIZE];

        sha256_ctx ctx;
        sha256_init(&ctx);
        sha256_update(&ctx, t.input, t.input_size);
        sha256_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }
#endif

#if defined(SHA256_CPUID_ARM64)
    while (test_step(&t, "sha256(arm64)", SHA256_CPUID_ARM64, sha256_tests))
    {
        uint8_t digest[SHA256_DIGEST_SIZE];

        sha256_ctx ctx;
        sha256_init(&ctx);
        sha256_update(&ctx, t.input, t.input_size);
        sha256_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }
#endif

    //
    // sha512
    //

#if defined(SHA512_CPUID_INIT)
    test_cpuid_mask = ~0;
    test_cpuid_value = sha512_cpuid();
    test_cpuid_mask = 0;
#endif

    while (test_step(&t, "sha512", 0, sha512_tests))
    {
        uint8_t digest[SHA512_DIGEST_SIZE];

        sha512_ctx ctx;
        sha512_init(&ctx);
        sha512_update(&ctx, t.input, t.input_size);
        sha512_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }

    while (test_step(&t, "sha384", 0, sha384_tests))
    {
        uint8_t digest[SHA384_DIGEST_SIZE];

        sha384_ctx ctx;
        sha384_init(&ctx);
        sha384_update(&ctx, t.input, t.input_size);
        sha384_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }

#if defined(SHA512_CPUID_VSHA512)
    while (test_step(&t, "sha512(vsha512)", SHA512_CPUID_VSHA512, sha512_tests))
    {
        uint8_t digest[SHA512_DIGEST_SIZE];

        sha512_ctx ctx;
        sha512_init(&ctx);
        sha512_update(&ctx, t.input, t.input_size);
        sha512_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }
#endif

#if defined(SHA512_CPUID_ARM64)
    while (test_step(&t, "sha512(arm64)", SHA512_CPUID_ARM64, sha512_tests))
    {
        uint8_t digest[SHA512_DIGEST_SIZE];

        sha512_ctx ctx;
        sha512_init(&ctx);
        sha512_update(&ctx, t.input, t.input_size);
        sha512_finish(&ctx, digest);

        test_check(&t, digest, sizeof(digest));
    }
#endif
}
