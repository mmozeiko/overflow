# memfun

Memory functions with SIMD optimizations for SSE2, AVX2, AVX512, NEON and RISC-V V instructions.

```c
// compares bytes in lexicographic order and returns:
//  < 0 if ptr1 comes before ptr2
//  = 0 if ptr1 contains same values as ptr2
//  > 0 if ptr1 comes after ptr2
MEM_API int MemCompare(const void* ptr1, const void* ptr2, size_t size);

// same as above, but case insensitive for ASCII characters
// value returned is produced as if comparing bytes converted to ASCII lowercase
MEM_API int MemCompareI(const void* ptr1, const void* ptr2, size_t size);

// returns true if all bytes of ptr1 are the same as ptr2
MEM_API bool MemIsEqual(const void* ptr1, const void* ptr2, size_t size);

// returns first offset of "value" byte, or "size" if not found
MEM_API size_t MemFind(const void* ptr, size_t size, uint8_t value);

// returns first offset of byte not equal to "value", or "size" if not found
MEM_API size_t MemFindNot(const void* ptr, size_t size, uint8_t value);
```

# Benchmark results

### msvc 19.51.36256 on AMD 9950X3D @ 4.30 GHz, Windows

```
function / bpc |  size |   CRT               |  sse2               |  avx2               | avx512
---------------+-------+---------------------+---------------------+---------------------+--------------------
MemCompare     |    15 |  1.33 @   5454 MB/s |  2.51 @  10302 MB/s |  1.67 @   6858 MB/s |  2.38 @   9753 MB/s
MemCompare     |    63 |  4.12 @  16890 MB/s |  7.91 @  32429 MB/s |  7.03 @  28820 MB/s |  7.91 @  32431 MB/s
MemCompare     |  1024 | 14.28 @  58571 MB/s | 15.72 @  64464 MB/s | 30.73 @ 126024 MB/s | 50.93 @ 208846 MB/s
MemCompare     | 16384 | 14.38 @  58987 MB/s | 15.67 @  64243 MB/s | 32.00 @ 131220 MB/s | 51.16 @ 209782 MB/s
MemCompareI    |    15 |  0.32 @   1305 MB/s |  1.62 @   6644 MB/s |  1.38 @   5639 MB/s |  1.67 @   6853 MB/s
MemCompareI    |    63 |  0.33 @   1363 MB/s |  3.69 @  15123 MB/s |  5.27 @  21597 MB/s |  7.74 @  31724 MB/s
MemCompareI    |  1024 | (slow)              |  5.65 @  23158 MB/s | 12.23 @  50162 MB/s | 18.52 @  75938 MB/s
MemCompareI    | 16384 | (slow)              |  5.80 @  23769 MB/s | 12.61 @  51690 MB/s | 18.70 @  76675 MB/s
MemIsEqual     |    15 |  1.16 @   4746 MB/s |  2.51 @  10300 MB/s |  2.51 @  10294 MB/s |  2.15 @   8826 MB/s
MemIsEqual     |    63 |  3.70 @  15168 MB/s |  7.92 @  32469 MB/s |  7.90 @  32415 MB/s | 10.55 @  43265 MB/s
MemIsEqual     |  1024 | 14.06 @  57645 MB/s | 15.95 @  65427 MB/s | 31.65 @ 129776 MB/s | 50.45 @ 206899 MB/s
MemIsEqual     | 16384 | 14.25 @  58445 MB/s | 15.71 @  64441 MB/s | 32.08 @ 131538 MB/s | 50.72 @ 207992 MB/s
MemFind        |    15 |  0.59 @   2411 MB/s |  2.51 @  10305 MB/s |  2.51 @  10309 MB/s |  2.11 @   8656 MB/s
MemFind        |    63 |  1.54 @   6312 MB/s |  8.75 @  35891 MB/s |  7.03 @  28827 MB/s |  7.91 @  32427 MB/s
MemFind        |  1024 | 10.29 @  42180 MB/s | 15.60 @  63978 MB/s | 48.92 @ 200609 MB/s | 58.97 @ 241783 MB/s
MemFind        | 16384 | 10.45 @  42869 MB/s | 15.51 @  63596 MB/s | 62.93 @ 258038 MB/s | 62.33 @ 255611 MB/s
MemFindNot     |    15 | (n/a)               |  0.68 @   2775 MB/s |  0.86 @   3519 MB/s |  2.51 @  10280 MB/s
MemFindNot     |    63 | (n/a)               |  2.87 @  11750 MB/s |  3.27 @  13390 MB/s |  7.90 @  32391 MB/s
MemFindNot     |  1024 | (n/a)               | 20.57 @  84339 MB/s | 45.85 @ 188017 MB/s | 59.19 @ 242685 MB/s
MemFindNot     | 16384 | (n/a)               | 20.75 @  85095 MB/s | 62.61 @ 256762 MB/s | 62.00 @ 254247 MB/s
```

### clang 22.1.8 on AMD 9950X3D @ 4.30 GHz, Windows

```
function / bpc |  size |   CRT               |  sse2               |  avx2               | avx512
---------------+-------+---------------------+---------------------+---------------------+--------------------
MemCompare     |    15 |  1.25 @   5185 MB/s |  2.14 @   8906 MB/s |  2.50 @  10387 MB/s |  2.14 @   8905 MB/s
MemCompare     |    63 |  4.26 @  17674 MB/s |  6.99 @  29037 MB/s |  6.99 @  29060 MB/s |  9.00 @  37376 MB/s
MemCompare     |  1024 | 14.42 @  59912 MB/s | 15.63 @  64918 MB/s | 31.50 @ 130819 MB/s | 51.14 @ 212481 MB/s
MemCompare     | 16384 | 15.20 @  63139 MB/s | 15.62 @  64893 MB/s | 31.93 @ 132601 MB/s | 51.53 @ 214044 MB/s
MemCompareI    |    15 |  0.33 @   1378 MB/s |  1.53 @   6359 MB/s |  1.66 @   6914 MB/s |  1.84 @   7644 MB/s
MemCompareI    |    63 |  0.35 @   1455 MB/s |  3.71 @  15406 MB/s |  6.11 @  25377 MB/s |  8.12 @  33714 MB/s
MemCompareI    |  1024 | (slow)              |  5.06 @  21005 MB/s | 12.16 @  50481 MB/s | 18.72 @  77641 MB/s
MemCompareI    | 16384 | (slow)              |  5.14 @  21338 MB/s | 12.36 @  51327 MB/s | 18.79 @  78017 MB/s
MemIsEqual     |    15 |  0.97 @   4038 MB/s |  2.46 @  10212 MB/s |  2.46 @  10214 MB/s |  2.09 @   8699 MB/s
MemIsEqual     |    63 |  3.94 @  16368 MB/s |  7.81 @  32452 MB/s |  7.80 @  32422 MB/s |  9.10 @  37818 MB/s
MemIsEqual     |  1024 | 13.79 @  57260 MB/s | 15.62 @  64896 MB/s | 29.17 @ 121162 MB/s | 49.35 @ 204859 MB/s
MemIsEqual     | 16384 | 14.83 @  61598 MB/s | 15.41 @  64006 MB/s | 31.39 @ 130359 MB/s | 49.79 @ 206811 MB/s
MemFind        |    15 |  0.85 @   3528 MB/s |  2.12 @   8791 MB/s |  1.65 @   6875 MB/s |  1.83 @   7620 MB/s
MemFind        |    63 |  2.05 @   8504 MB/s |  7.83 @  32515 MB/s |  6.85 @  28433 MB/s |  7.73 @  32100 MB/s
MemFind        |  1024 | 12.92 @  53645 MB/s | 20.43 @  84874 MB/s | 52.32 @ 217159 MB/s | 58.93 @ 244814 MB/s
MemFind        | 16384 | 13.10 @  54403 MB/s | 20.51 @  85187 MB/s | 61.28 @ 254350 MB/s | 61.12 @ 253826 MB/s
MemFindNot     |    15 | (n/a)               |  0.68 @   2844 MB/s |  1.66 @   6877 MB/s |  1.85 @   7701 MB/s
MemFindNot     |    63 | (n/a)               |  2.64 @  10982 MB/s |  4.25 @  17667 MB/s |  7.77 @  32288 MB/s
MemFindNot     |  1024 | (n/a)               | 15.57 @  64673 MB/s | 38.97 @ 161897 MB/s | 57.90 @ 240592 MB/s
MemFindNot     | 16384 | (n/a)               | 15.30 @  63566 MB/s | 41.89 @ 173973 MB/s | 61.35 @ 254869 MB/s
```

### clang 22.1.8 on Intel i7-1185G7 @ 3.00 GHz, glibc 2.44, Linux

```
function / bpc |  size |   CRT               |  sse2               |  avx2               | avx512
---------------+-------+---------------------+---------------------+---------------------+--------------------
MemCompare     |    15 |  1.66 @   4751 MB/s |  2.14 @   6110 MB/s |  2.49 @   7124 MB/s |  1.87 @   5347 MB/s
MemCompare     |    63 |  5.72 @  16331 MB/s |  6.98 @  19930 MB/s |  6.99 @  19953 MB/s |  7.86 @  22457 MB/s
MemCompare     |  1024 | 29.74 @  84961 MB/s | 12.17 @  34750 MB/s | 29.97 @  85601 MB/s | 36.03 @ 102923 MB/s
MemCompare     | 16384 | 30.38 @  86792 MB/s | 12.51 @  35734 MB/s | 30.48 @  87053 MB/s | 51.52 @ 147158 MB/s
MemCompareI    |    15 |  0.24 @    680 MB/s |  2.14 @   6106 MB/s |  1.87 @   5347 MB/s |  1.66 @   4751 MB/s
MemCompareI    |    63 |  0.21 @    593 MB/s |  3.31 @   9450 MB/s |  5.24 @  14967 MB/s |  6.99 @  19955 MB/s
MemCompareI    |  1024 | (slow)              |  4.26 @  12172 MB/s |  9.24 @  26382 MB/s | 10.10 @  28840 MB/s
MemCompareI    | 16384 | (slow)              |  4.19 @  11967 MB/s |  9.32 @  26634 MB/s | 10.04 @  28679 MB/s
MemIsEqual     |    15 |  1.50 @   4277 MB/s |  2.49 @   7125 MB/s |  2.50 @   7127 MB/s |  2.14 @   6110 MB/s
MemIsEqual     |    63 |  5.24 @  14966 MB/s |  6.99 @  19956 MB/s |  7.86 @  22457 MB/s |  8.98 @  25662 MB/s
MemIsEqual     |  1024 | 29.36 @  83854 MB/s | 14.20 @  40552 MB/s | 28.21 @  80565 MB/s | 36.49 @ 104236 MB/s
MemIsEqual     | 16384 | 30.41 @  86871 MB/s | 13.90 @  39712 MB/s | 29.98 @  85648 MB/s | 43.38 @ 123902 MB/s
MemFind        |    15 |  1.25 @   3563 MB/s |  2.33 @   6665 MB/s |  2.39 @   6839 MB/s |  1.87 @   5347 MB/s
MemFind        |    63 |  4.84 @  13817 MB/s |  6.64 @  18966 MB/s |  8.98 @  25661 MB/s |  7.86 @  22451 MB/s
MemFind        |  1024 | 26.62 @  76028 MB/s | 14.81 @  42314 MB/s | 30.18 @  86208 MB/s | 37.84 @ 108070 MB/s
MemFind        | 16384 | 35.35 @ 100966 MB/s | 15.47 @  44176 MB/s | 31.93 @  91206 MB/s | 50.60 @ 144549 MB/s
MemFindNot     |    15 | (n/a)               |  2.16 @   6176 MB/s |  2.20 @   6281 MB/s |  1.87 @   5346 MB/s
MemFindNot     |    63 | (n/a)               |  6.49 @  18547 MB/s |  7.86 @  22457 MB/s |  7.86 @  22457 MB/s
MemFindNot     |  1024 | (n/a)               | 14.66 @  41863 MB/s | 29.76 @  84991 MB/s | 51.01 @ 145693 MB/s
MemFindNot     | 16384 | (n/a)               | 15.37 @  43898 MB/s | 31.88 @  91065 MB/s | 54.28 @ 155036 MB/s
```

### clang 22.1.8 on Intel N4200 @ 1.10 GHz, glibc 2.44, Linux

```
function / bpc |  size |   CRT               |  sse2              
---------------+-------+---------------------+--------------------
MemCompare     |    15 |  1.49 @   1559 MB/s |  0.79 @    821 MB/s
MemCompare     |    63 |  3.14 @   3274 MB/s |  1.99 @   2076 MB/s
MemCompare     |  1024 |  7.18 @   7493 MB/s |  5.83 @   6081 MB/s
MemCompare     | 16384 |  4.57 @   4765 MB/s |  4.61 @   4813 MB/s
MemCompareI    |    15 |  0.10 @    103 MB/s |  0.68 @    709 MB/s
MemCompareI    |    63 |  0.11 @    118 MB/s |  1.23 @   1280 MB/s
MemCompareI    |  1024 | (slow)              |  2.13 @   2226 MB/s
MemCompareI    | 16384 | (slow)              |  1.81 @   1891 MB/s
MemIsEqual     |    15 |  1.31 @   1363 MB/s |  1.30 @   1356 MB/s
MemIsEqual     |    63 |  2.85 @   2976 MB/s |  3.14 @   3273 MB/s
MemIsEqual     |  1024 |  7.08 @   7390 MB/s |  6.41 @   6694 MB/s
MemIsEqual     | 16384 |  4.56 @   4764 MB/s |  4.72 @   4926 MB/s
MemFind        |    15 |  0.75 @    779 MB/s |  0.60 @    624 MB/s
MemFind        |    63 |  2.32 @   2424 MB/s |  2.30 @   2403 MB/s
MemFind        |  1024 |  7.84 @   8184 MB/s |  7.03 @   7341 MB/s
MemFind        | 16384 | 10.27 @  10715 MB/s |  8.94 @   9327 MB/s
MemFindNot     |    15 | (n/a)               |  0.61 @    635 MB/s
MemFindNot     |    63 | (n/a)               |  2.44 @   2551 MB/s
MemFindNot     |  1024 | (n/a)               |  6.89 @   7188 MB/s
MemFindNot     | 16384 | (n/a)               |  8.91 @   9297 MB/s
```

### msvc 19.51.36256 on Snapdragon X Elite, Qualcomm Oryon X1E001DE @ 3.80 GHz, Windows

```
function / bpc |  size |   CRT               |  neon
---------------+-------+---------------------+--------------------
MemCompare     |    15 |  1.66 @   6035 MB/s |  2.50 @   9056 MB/s
MemCompare     |    63 |  3.94 @  14266 MB/s |  7.00 @  25363 MB/s
MemCompare     |  1024 | 10.13 @  36737 MB/s | 14.62 @  52996 MB/s
MemCompare     | 16384 | 10.44 @  37846 MB/s | 15.58 @  56478 MB/s
MemCompareI    |    15 |  0.38 @   1388 MB/s |  2.14 @   7763 MB/s
MemCompareI    |    63 |  0.33 @   1213 MB/s |  4.20 @  15216 MB/s
MemCompareI    |  1024 | (slow)              |  5.65 @  20488 MB/s
MemCompareI    | 16384 | (slow)              |  5.74 @  20817 MB/s
MemIsEqual     |    15 |  1.50 @   5435 MB/s |  2.50 @   9064 MB/s
MemIsEqual     |    63 |  3.81 @  13806 MB/s |  7.67 @  27783 MB/s
MemIsEqual     |  1024 | 10.04 @  36377 MB/s | 14.83 @  53763 MB/s
MemIsEqual     | 16384 | 10.44 @  37847 MB/s | 15.61 @  56577 MB/s
MemFind        |    15 |  1.81 @   6559 MB/s |  3.00 @  10877 MB/s
MemFind        |    63 |  5.73 @  20754 MB/s |  7.87 @  28532 MB/s
MemFind        |  1024 | 20.48 @  74237 MB/s | 18.96 @  68749 MB/s
MemFind        | 16384 | 21.69 @  78601 MB/s | 20.57 @  74539 MB/s
MemFindNot     |    15 | (n/a)               |  3.00 @  10877 MB/s
MemFindNot     |    63 | (n/a)               |  7.87 @  28531 MB/s
MemFindNot     |  1024 | (n/a)               | 18.62 @  67502 MB/s
MemFindNot     | 16384 | (n/a)               | 20.59 @  74622 MB/s
```

### clang 22.1.8 on Snapdragon X Elite, Qualcomm Oryon X1E001DE @ 3.80 GHz, Windows

```
function / bpc |  size |   CRT               |  neon
---------------+-------+---------------------+--------------------
MemCompare     |    15 |  1.87 @   6793 MB/s |  2.50 @   9064 MB/s
MemCompare     |    63 |  4.30 @  15573 MB/s |  6.29 @  22791 MB/s
MemCompare     |  1024 | 10.13 @  36737 MB/s | 15.28 @  55367 MB/s
MemCompare     | 16384 | 10.45 @  37870 MB/s | 15.65 @  56714 MB/s
MemCompareI    |    15 |  0.38 @   1372 MB/s |  2.14 @   7762 MB/s
MemCompareI    |    63 |  0.34 @   1224 MB/s |  3.94 @  14266 MB/s
MemCompareI    |  1024 | (slow)              |  5.72 @  20728 MB/s
MemCompareI    | 16384 | (slow)              |  5.76 @  20874 MB/s
MemIsEqual     |    15 |  1.87 @   6793 MB/s |  3.00 @  10877 MB/s
MemIsEqual     |    63 |  4.35 @  15765 MB/s |  7.55 @  27370 MB/s
MemIsEqual     |  1024 | 10.13 @  36735 MB/s | 15.51 @  56205 MB/s
MemIsEqual     | 16384 | 10.45 @  37878 MB/s | 15.70 @  56917 MB/s
MemFind        |    15 |  2.41 @   8753 MB/s |  7.06 @  25595 MB/s
MemFind        |    63 |  7.00 @  25363 MB/s | 10.50 @  38066 MB/s
MemFind        |  1024 | 21.48 @  77888 MB/s | 20.48 @  74246 MB/s
MemFind        | 16384 | 21.57 @  78187 MB/s | 20.79 @  75337 MB/s
MemFindNot     |    15 | (n/a)               |  6.67 @  24169 MB/s
MemFindNot     |    63 | (n/a)               | 10.50 @  38068 MB/s
MemFindNot     |  1024 | (n/a)               | 20.48 @  74252 MB/s
MemFindNot     | 16384 | (n/a)               | 20.80 @  75395 MB/s
```

### clang 22.1.8 on Apple M4, P-core @ 4.40 GHz, macOS

```
function / bpc |  size |   CRT               |  neon
---------------+-------+---------------------+--------------------
MemCompare     |    15 |  1.25 @   5319 MB/s |  2.50 @  10634 MB/s
MemCompare     |    63 |  7.00 @  29783 MB/s |  7.00 @  29783 MB/s
MemCompare     |  1024 | 14.62 @  62230 MB/s | 20.07 @  85392 MB/s
MemCompare     | 16384 | 14.15 @  60249 MB/s | 20.79 @  88499 MB/s
MemCompareI    |    15 |  0.47 @   1995 MB/s |  1.87 @   7974 MB/s
MemCompareI    |    63 |  0.49 @   2079 MB/s |  5.25 @  22333 MB/s
MemCompareI    |  1024 | (slow)              |  6.07 @  25821 MB/s
MemCompareI    | 16384 | (slow)              |  6.09 @  25937 MB/s
MemIsEqual     |    15 |  1.25 @   5318 MB/s |  3.75 @  15943 MB/s
MemIsEqual     |    63 |  9.00 @  38285 MB/s | 10.50 @  44660 MB/s
MemIsEqual     |  1024 | 14.62 @  62232 MB/s | 20.47 @  84780 MB/s
MemIsEqual     | 16384 | 14.18 @  60289 MB/s | 20.89 @  88941 MB/s
MemFind        |    15 |  3.00 @  12756 MB/s |  7.49 @  31844 MB/s
MemFind        |    63 |  7.87 @  33496 MB/s | 12.59 @  53583 MB/s
MemFind        |  1024 | 14.22 @  60500 MB/s | 27.65 @ 116922 MB/s
MemFind        | 16384 | 14.90 @  63424 MB/s | 28.34 @ 119644 MB/s
MemFindNot     |    15 | (n/a)               |  7.49 @  31613 MB/s
MemFindNot     |    63 | (n/a)               | 12.59 @  53584 MB/s
MemFindNot     |  1024 | (n/a)               | 30.03 @ 127554 MB/s
MemFindNot     | 16384 | (n/a)               | 30.90 @ 131531 MB/s
```

### clang 22.1.8 on Apple M4, E-core @ 2.90 GHz, macOS

```
function / bpc |  size |   CRT               |  neon
---------------+-------+---------------------+--------------------
MemCompare     |    15 |  1.07 @   1101 MB/s |  2.50 @   2570 MB/s
MemCompare     |    63 |  6.69 @   6882 MB/s |  6.28 @   6466 MB/s
MemCompare     |  1024 |  8.47 @   8722 MB/s | 12.32 @  12683 MB/s
MemCompare     | 16384 |  9.14 @   9413 MB/s | 12.60 @  12980 MB/s
MemCompareI    |    15 |  0.27 @    273 MB/s |  1.50 @   1543 MB/s
MemCompareI    |    63 |  0.25 @    353 MB/s |  3.00 @   3086 MB/s
MemCompareI    |  1024 | (slow)              |  3.95 @   4067 MB/s
MemCompareI    | 16384 | (slow)              |  3.98 @   4097 MB/s
MemIsEqual     |    15 |  1.07 @   1102 MB/s |  3.46 @   3556 MB/s
MemIsEqual     |    63 |  6.42 @   6609 MB/s |  8.58 @   8820 MB/s
MemIsEqual     |  1024 |  8.33 @   8578 MB/s | 12.32 @  12684 MB/s
MemIsEqual     | 16384 |  9.15 @   9418 MB/s | 12.63 @  13011 MB/s
MemFind        |    15 |  2.27 @   2336 MB/s |  4.36 @   4484 MB/s
MemFind        |    63 |  5.72 @   5889 MB/s |  8.72 @   8975 MB/s
MemFind        |  1024 |  8.66 @   8921 MB/s | 15.26 @  15705 MB/s
MemFind        | 16384 |  9.52 @  10220 MB/s | 15.74 @  16205 MB/s
MemFindNot     |    15 | (n/a)               |  4.11 @   4231 MB/s
MemFindNot     |    63 | (n/a)               |  8.74 @   8995 MB/s
MemFindNot     |  1024 | (n/a)               | 15.49 @  15948 MB/s
MemFindNot     | 16384 | (n/a)               | 15.75 @  16223 MB/s
```

### clang 22.1.8 on Raspberry Pi 5, Cortex-A76 @ 2.4 GHz, glibc 2.43, Linux

```
function / bpc |  size |   CRT               |  neon
---------------+-------+---------------------+--------------------
MemCompare     |    15 |  1.25 @   2860 MB/s |  1.67 @   3813 MB/s
MemCompare     |    63 |  4.66 @  10659 MB/s |  3.86 @   8826 MB/s
MemCompare     |  1024 | 10.57 @  24202 MB/s |  9.51 @  21773 MB/s
MemCompare     | 16384 | 11.71 @  26792 MB/s | 10.11 @  23147 MB/s
MemCompareI    |    15 |  0.20 @    452 MB/s |  1.02 @   2324 MB/s
MemCompareI    |    63 |  0.21 @    479 MB/s |  2.08 @   4752 MB/s
MemCompareI    |  1024 | (slow)              |  3.02 @   6912 MB/s
MemCompareI    | 16384 | (slow)              |  3.04 @   6969 MB/s
MemIsEqual     |    15 |  1.25 @   2860 MB/s |  2.14 @   4905 MB/s
MemIsEqual     |    63 |  4.52 @  10354 MB/s |  5.73 @  13104 MB/s
MemIsEqual     |  1024 | 10.46 @  23931 MB/s |  8.82 @  20199 MB/s
MemIsEqual     | 16384 | 11.60 @  26555 MB/s |  9.12 @  20866 MB/s
MemFind        |    15 |  1.25 @   2860 MB/s |  3.00 @   6866 MB/s
MemFind        |    63 |  3.15 @   7207 MB/s |  5.70 @  13041 MB/s
MemFind        |  1024 |  9.36 @  21417 MB/s | 12.05 @  27573 MB/s
MemFind        | 16384 |  9.80 @  22420 MB/s | 12.74 @  29171 MB/s
MemFindNot     |    15 | (n/a)               |  3.00 @   6866 MB/s
MemFindNot     |    63 | (n/a)               |  5.57 @  12743 MB/s
MemFindNot     |  1024 | (n/a)               | 13.04 @  29857 MB/s
MemFindNot     | 16384 | (n/a)               | 13.85 @  31701 MB/s
```

### clang 21.1.8 on OrangePi RV2, SpacemiT X60 @ 1.6 GHz, glibc 2.41, Linux

compiled with `-march=rv64gv`

```
function / bpc |  size |   CRT               |   rvv
---------------+-------+---------------------+--------------------
MemCompare     |    15 |  0.13 @    198 MB/s |  0.27 @    408 MB/s
MemCompare     |    63 |  0.60 @    918 MB/s |  1.12 @   1713 MB/s
MemCompare     |  1024 |  1.84 @   2814 MB/s |  4.75 @   7255 MB/s
MemCompare     | 16384 |  1.89 @   2889 MB/s |  4.77 @   7272 MB/s
MemCompareI    |    15 |  0.08 @    117 MB/s |  0.11 @    172 MB/s
MemCompareI    |    63 |  0.08 @    118 MB/s |  0.47 @    721 MB/s
MemCompareI    |  1024 | (slow)              |  1.95 @   2982 MB/s
MemCompareI    | 16384 | (slow)              |  1.95 @   2978 MB/s
MemIsEqual     |    15 |  0.13 @    196 MB/s |  0.26 @    394 MB/s
MemIsEqual     |    63 |  0.60 @    909 MB/s |  1.08 @   1654 MB/s
MemIsEqual     |  1024 |  1.84 @   2809 MB/s |  4.58 @   6986 MB/s
MemIsEqual     | 16384 |  1.90 @   2896 MB/s |  4.59 @   7009 MB/s
MemFind        |    15 |  0.33 @    511 MB/s |  0.36 @    544 MB/s
MemFind        |    63 |  0.73 @   1118 MB/s |  1.50 @   2284 MB/s
MemFind        |  1024 |  1.48 @   2258 MB/s |  6.43 @   9815 MB/s
MemFind        | 16384 |  1.58 @   2403 MB/s |  6.46 @   9862 MB/s
MemFindNot     |    15 | (n/a)               |  0.36 @    544 MB/s
MemFindNot     |    63 | (n/a)               |  1.50 @   2284 MB/s
MemFindNot     |  1024 | (n/a)               |  6.43 @   9816 MB/s
MemFindNot     | 16384 | (n/a)               |  6.46 @   9862 MB/s
```
