#!/usr/bin/env python

from pathlib import Path
from urllib import request
from zipfile import ZipFile
from shutil import copyfileobj

ROOT = Path(__file__).resolve().parent
GENERATED = ROOT / "generated"

NSRLvectors = ROOT / "NSRLvectors.zip"
SHAvectors = ROOT / "shabytetestvectors.zip"

HTTP_HEADERS = { "User-Agent": "Mozilla/5.0" }

# "abcd" -> "\xab\xcd"
def chexstr(s):
  return "".join(f"\\x{s[i:i+2]}" for i in range(0, len(s), 2))


def cavp_process(f, out):
  for line in f:
    if line.startswith(b"#"):  continue
    if line.startswith(b"\n"): continue
    if line.startswith(b"["):  continue

    if line.startswith(b"Len"):
      input_length = int(line.split(b"=")[1].strip(), 10) // 8

    if line.startswith(b"Msg"):
      input_hex = line.split(b"=")[1].strip().decode("ascii")
      if input_length == 0:
        assert(input_hex == "00")
        input_hex = ""

    if line.startswith(b"MD"):
      output_hex = line.split(b"=")[1].strip().decode("ascii")

      print("{ ", end="", file=out)
      print('"', chexstr(output_hex), '", ', sep="", end="", file=out)
      print('"', chexstr(input_hex), '", ', sep="", end="", file=out)
      print(input_length, " },", sep="", file=out)

      input_length = None
      input_hex = None
      output_hex = None


if not NSRLvectors.exists():
  req = request.Request("https://s3.amazonaws.com/docs.nsrl.nist.gov/legacy/NSRLvectors.zip", headers=HTTP_HEADERS)
  with request.urlopen(req) as resp:
    with open(NSRLvectors, "wb") as out:
      copyfileobj(resp, out)


if not SHAvectors.exists():
  req = request.Request("https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/documents/shs/shabytetestvectors.zip", headers=HTTP_HEADERS)
  with request.urlopen(req) as resp:
    with open(SHAvectors, "wb") as out:
      copyfileobj(resp, out)


with ZipFile(NSRLvectors) as z:
  hashes = []
  with z.open("vectors/byte-hashes.md5") as f:
    for line in f:
      if line.startswith(b"#"): continue
      if line.startswith(b"<D"): continue
      if line.startswith(b"D>"): continue
      if line.startswith(b"H>"): continue

      hexstr = line.rstrip(b"\n ^").decode("ascii")
      hashes.append(bytes.fromhex(hexstr))

  with open(GENERATED / "md5_nsrl.h", "w") as out:
    for i, h in enumerate(hashes):
      with z.open(f"vectors/byte{i:04}.dat") as msg:

        input_bytes = msg.read()
        input_hex = input_bytes.hex()
        output_hex = h.hex()

      print("{ ", end="", file=out)
      print('"', chexstr(output_hex), '", ', sep="", end="", file=out)
      print('"', chexstr(input_hex), '" , ', sep="", end="", file=out)
      print(len(input_bytes), " },", sep="", file=out)

with ZipFile(SHAvectors) as z:
  for sha in [1, 224, 256, 384, 512]:
    with open(GENERATED / f"sha{sha}_cavp.h", "w") as out:
      with z.open(f"shabytetestvectors/SHA{sha}ShortMsg.rsp") as f:
        cavp_process(f, out)
      with z.open(f"shabytetestvectors/SHA{sha}LongMsg.rsp") as f:
        cavp_process(f, out)
