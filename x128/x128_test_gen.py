#!/usr/bin/env python

from pathlib import Path

values = [ \
  0x0000000000000000,
  0x0000000000000001,
  0x0123456789abcdef,
  0x7fffffffffffffff,
  0x8000000000000000,
  0x8000000000000001,
  0xfedcba9876543210,
  0xffffffffffffffff,
]

def x128(x):
  x &= 2**128 - 1
  lo = x % 2**64
  hi = x >> 64
  return f"{{ 0x{lo:016x}, 0x{hi:016x} }}"

def u128(lo, hi):
  return lo + (hi<<64)

def s128(lo, hi):
  x = u128(lo, hi)
  if x >= 2**127: x -= 2**128
  return x

def clz(x):
  return 128 - x.bit_length()

def ctz(x):
  if x == 0: return 128
  n = 0
  while x&1 == 0:
    n += 1
    x >>= 1
  return n

def popcnt(x):
  n = 0
  for i in range(128):
    n += x&1
    x >>= 1
  return n

def x2base(x, base):
  s = []
  while x:
    num = x % base
    s.append(chr(ord("0") + num if num < 10 else ord("a") + num - 10) )
    x //= base
  return "".join(reversed(s))


with Path(Path(__file__).parent / "x128_test.h").open("w") as f:

  f.write(f"// generated with {Path(__file__).name}\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_set_u64[] = {{\n")
  for x in values:
    f.write(f"    {x128(x)},\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_set_s64[] = {{\n")
  for x in values:
    x |= (2**64-1)<<64 if x>>63 else 0
    f.write(f"    {x128(x)},\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const char* test_x128_signum[] = {{\n")
  for lo in values:
    s = ""
    for hi in values:
      x = s128(lo, hi)
      s += "n" if x < 0 else "p" if x > 0 else "0"
    f.write(f'    "{s}",\n')
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const char* test_x128_cmp_u[8][8][8] = {{\n")
  for alo in values:
    f.write("    {")
    for ahi in values:
      f.write(" { ")
      for blo in values:
        s = ""
        for bhi in values:
          a = u128(alo, ahi)
          b = u128(blo, bhi)
          s += "n" if a < b else "p" if a > b else "0"
        f.write(f'"{s}", ')
      f.write("},")
    f.write("},\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const char* test_x128_cmp_s[8][8][8] = {{\n")
  for alo in values:
    f.write("    {")
    for ahi in values:
      f.write(" { ")
      for blo in values:
        s = ""
        for bhi in values:
          a = s128(alo, ahi)
          b = s128(blo, bhi)
          s += "n" if a < b else "p" if a > b else "0"
        f.write(f'"{s}", ')
      f.write("},")
    f.write("},\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_min_u[8][8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for blo in values:
        f.write("            {\n")
        for bhi in values:
          a = u128(alo, ahi)
          b = u128(blo, bhi)
          f.write(f"                {x128(min(a,b))},\n")
        f.write("            },\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_min_s[8][8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for blo in values:
        f.write("            {\n")
        for bhi in values:
          a = s128(alo, ahi)
          b = s128(blo, bhi)
          f.write(f"                {x128(min(a,b))},\n")
        f.write("            },\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_max_u[8][8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for blo in values:
        f.write("            {\n")
        for bhi in values:
          a = u128(alo, ahi)
          b = u128(blo, bhi)
          f.write(f"                {x128(max(a,b))},\n")
        f.write("            },\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_max_s[8][8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for blo in values:
        f.write("            {\n")
        for bhi in values:
          a = s128(alo, ahi)
          b = s128(blo, bhi)
          f.write(f"                {x128(max(a,b))},\n")
        f.write("            },\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_not[8][8] = {{\n")
  for lo in values:
    f.write("    {\n")
    for hi in values:
      x = u128(lo, hi)
      f.write(f"        {x128(~x)},\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_or[8][8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for blo in values:
        f.write("            {\n")
        for bhi in values:
          a = s128(alo, ahi)
          b = s128(blo, bhi)
          f.write(f"                {x128(a|b)},\n")
        f.write("            },\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_and[8][8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for blo in values:
        f.write("            {\n")
        for bhi in values:
          a = s128(alo, ahi)
          b = s128(blo, bhi)
          f.write(f"                {x128(a&b)},\n")
        f.write("            },\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_xor[8][8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for blo in values:
        f.write("            {\n")
        for bhi in values:
          a = s128(alo, ahi)
          b = s128(blo, bhi)
          f.write(f"                {x128(a^b)},\n")
        f.write("            },\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_shl[8][8][128+1] = {{\n")
  for lo in values:
    f.write("    {\n")
    for hi in values:
      f.write("        {\n")
      for n in range(128+1):
        x = u128(lo, hi)
        f.write(f"            {x128(x<<n)},\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_shr_u[8][8][128+1] = {{\n")
  for lo in values:
    f.write("    {\n")
    for hi in values:
      f.write("        {\n")
      for n in range(128+1):
        x = u128(lo, hi)
        f.write(f"            {x128(x>>n)},\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")
  
  f.write(f"\n")
  f.write(f"static const x128 test_x128_shr_s[8][8][128+1] = {{\n")
  for lo in values:
    f.write("    {\n")
    for hi in values:
      f.write("        {\n")
      for n in range(128+1):
        x = s128(lo, hi)
        f.write(f"            {x128(x>>n)},\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const size_t test_x128_clz[8][8] = {{\n")
  for lo in values:
    f.write("    { ")
    for hi in values:
      x = u128(lo, hi)
      f.write(f"{clz(x):3d}, ")
    f.write("},\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const size_t test_x128_ctz[8][8] = {{\n")
  for lo in values:
    f.write("    { ")
    for hi in values:
      x = u128(lo, hi)
      f.write(f"{ctz(x):3d}, ")
    f.write("},\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const size_t test_x128_popcnt[8][8] = {{\n")
  for lo in values:
    f.write("    { ")
    for hi in values:
      x = u128(lo, hi)
      f.write(f"{popcnt(x):3d}, ")
    f.write("},\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_abs[8][8] = {{\n")
  for lo in values:
    f.write("    {\n")
    for hi in values:
      x = s128(lo, hi)
      f.write(f"        {x128(abs(x))},\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_neg[8][8] = {{\n")
  for lo in values:
    f.write("    {\n")
    for hi in values:
      x = s128(lo, hi)
      f.write(f"        {x128(-x)},\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_add[8][8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for blo in values:
        f.write("            {\n")
        for bhi in values:
          a = u128(alo, ahi)
          b = u128(blo, bhi)
          f.write(f"                {x128(a+b)},\n")
        f.write("            },\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_sub[8][8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for blo in values:
        f.write("            {\n")
        for bhi in values:
          a = u128(alo, ahi)
          b = u128(blo, bhi)
          f.write(f"                {x128(a-b)},\n")
        f.write("            },\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_mul_64x64[8][8] = {{\n")
  for a in values:
    f.write("    {\n")
    for b in values:
      f.write(f"        {x128(a*b)},\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_mul_128x64[8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for b in values:
        a = u128(alo, ahi)
        f.write(f"            {x128(a*b)},\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_mul[8][8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for blo in values:
        f.write("            {\n")
        for bhi in values:
          a = u128(alo, ahi)
          b = u128(blo, bhi)
          f.write(f"                {x128(a*b)},\n")
        f.write("            },\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const x128 test_x128_div_u[8][8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for blo in values:
        f.write("            {\n")
        for bhi in values:
          a = u128(alo, ahi)
          b = u128(blo, bhi)
          f.write(f"                {x128(0 if b == 0 else a//b)},\n")
        f.write("            },\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"static const x128 test_x128_mod_u[8][8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for blo in values:
        f.write("            {\n")
        for bhi in values:
          a = u128(alo, ahi)
          b = u128(blo, bhi)
          f.write(f"                {x128(0 if b == 0 else a%b)},\n")
        f.write("            },\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const struct {{ x128 q; uint64_t r; }} test_x128_div_u64[8][8][8] = {{\n")
  for alo in values:
    f.write("    {\n")
    for ahi in values:
      f.write("        {\n")
      for b in values:
        a = u128(alo, ahi)
        q = 0 if b == 0 else a//b
        r = 0 if b == 0 else a%b
        f.write(f"            {{ {x128(q)}, 0x{r:016x} }},\n")
      f.write("        },\n")
    f.write("    },\n")
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const char* test_x128_str_u[] = {{\n")
  for base in range(2,36+1):
    u = u128(0x0123456789abcdef, 0xfedcba9876543210)
    f.write(f'    "{x2base(u, base)}",\n')
  f.write(f"}};\n")

  f.write(f"\n")
  f.write(f"static const char* test_x128_str_s[] = {{\n")
  for base in range(2,36+1):
    u = s128(0xfedcba9876543210, 0x0123456789abcdef)
    f.write(f'    "-{x2base(abs(u), base)}",\n')
  f.write(f"}};\n")
