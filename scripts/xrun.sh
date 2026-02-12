#!/usr/bin/env bash

set -euo pipefail
#set -x

# setup on ArchLinux:
#   sudo pacman -Syu --needed gcc clang lld                                                  # gcc & clang
#   sudo pacman -Syu --needed aarch64-linux-gnu-glibc riscv64-linux-gnu-gcc qemu-user-static # arm64 & rv64 + qemu
#   sudo pacman -Syu --needed mingw-w64-gcc wine                                             # mingw + wine
#   sudo pacman -Syu --needed wasi-libc wasi-compiler-rt wasmtime                            # wasm + wasi

# setup on Ubuntu 24.04:
#   sudo apt install --no-install-recommends g++ clang lld                                                # gcc & clang
#   sudo apt install --no-install-recommends g++-aarch64-linux-gnu g++-riscv64-linux-gnu qemu-user-static # arm64 & rv64 + qemu
#   sudo apt install --no-install-recommends g++-mingw-w64-x86-64-posix wine                              # mingw + wine
#   sudo apt install --no-install-recommends wasi-libc libclang-rt-dev-wasm32                             # wasm + wasi
# then get wasmtime binary:
#   sudo apt install --no-install-recommends curl ca-certificates xz-utils
#   curl -sfL https://github.com/bytecodealliance/wasmtime/releases/download/v36.0.1/wasmtime-v26.0.1-$(uname -m)-linux.tar.xz | tar xJ --strip-components=1 wasmtime-v36.0.1-$(uname -m)-linux/wasmtime
#   sudo mv wasmtime /usr/local/bin/

# setup on macOS:
#   xcode-select --install
# for building/running wasm use homebrew:
#   brew install llvm wasi-libc wasi-runtimes wasmtime
# and use mainstream clang:
#   CLANG=/opt/homebrew/opt/llvm/bin/clang ./xrun.sh ...

declare INPUT=${1}; shift
declare ARGS=($@)
          
declare CFLAGS=("-O2 -Wall -Wextra -Werror -ffp-contract=off")
declare LDFLAGS=("-lm")

declare OS_VALUES=("linux macos mingw wasi")
declare ARCH_VALUES=("x64 rv64 arm64 wasm32")
declare CC_VALUES=("gcc clang")

declare HOST_OS=$(uname -s)
declare HOST_ARCH=$(uname -m)
[[ ${HOST_ARCH} == "x86_64"  ]] && HOST_ARCH="x64"
[[ ${HOST_ARCH} == "riscv64" ]] && HOST_ARCH="rv64"
[[ ${HOST_ARCH} == "aarch64" ]] && HOST_ARCH="arm64"

declare DISTRIB_ID
[[ -f /etc/lsb-release ]] && source /etc/lsb-release
[[ -f /etc/os-release ]] && source /etc/os-release && [[ ${ID_LIKE:-} == "arch" ]] && DISTRIB_ID="Arch"

function arg()
{
    for x in ${ARGS[@]+"${ARGS[@]}"}; do [[ ${x} == ${1} ]] && return 0; done
    return 1
}

function xrun()
{
  declare OS=$1
  declare CC=$2
  declare ARCH=$3

  for x in ${OS_VALUES[@]};   do arg "${x}" && [[ $x != ${OS}   ]] && return 0; done
  for x in ${CC_VALUES[@]};   do arg "${x}" && [[ $x != ${CC}   ]] && return 0; done
  for x in ${ARCH_VALUES[@]}; do arg "${x}" && [[ $x != ${ARCH} ]] && return 0; done

  echo ===== ${OS} ${ARCH} ${CC} =====

  declare OUTPUT=$(basename ${INPUT%.*})_${OS}_${CC}_${ARCH}.exe

  declare BUILD=()
  declare RUN=()

  declare NORUN=0
  arg "norun" && NORUN=1

  if [[ ${OS} == "linux" ]]; then

    declare ARCH_VALUE
    [[ ${ARCH} == "x64"   ]] && ARCH_VALUE="x86_64"
    [[ ${ARCH} == "rv64"  ]] && ARCH_VALUE="riscv64"
    [[ ${ARCH} == "arm64" ]] && ARCH_VALUE="aarch64"

    declare TARGET="${ARCH_VALUE}-linux-gnu"
    [[ ${DISTRIB_ID:-} == "Arch" && ${HOST_ARCH} == "arm64" ]] && TARGET="${ARCH_VALUE}-unknown-linux-gnu"

    [[ ${CC} == "gcc"   ]] && BUILD+=("${TARGET}-${GCC:-gcc}")
    [[ ${CC} == "clang" ]] && BUILD+=("${CLANG:-clang} -fuse-ld=lld -target ${TARGET}")

    if [[ ${ARCH} != ${HOST_ARCH} ]]; then
      [[ ${DISTRIB_ID:-} == "Arch" ]] && BUILD+=("--sysroot=/usr/${TARGET}")
      BUILD+=("-static")
      RUN+=("qemu-${ARCH_VALUE}-static")
      [[ ${ARCH} == "rv64" ]] && RUN+=("-cpu rv64,v=true,vlen=128,elen=64,vext_spec=v1.0,rvv_ta_all_1s=true,rvv_ma_all_1s=true")
      qemu-${ARCH_VALUE}-static --version | head -1
    elif [[ ${SDE:-} != "" ]]; then
      RUN+=("sde64" "${SDE}" "--")
      sde64 -version | head -1
    fi

  elif [[ ${OS} == "macos" ]]; then

    BUILD+=("${CLANG:-clang}")
    [[ ${ARCH} == "x64"   ]] && BUILD+=("-arch x86_64")
    [[ ${ARCH} == "arm64" ]] && BUILD+=("-arch arm64")

    # cannot run arm64 on x64
    # to be able to run x64 on arm64 install rosetta with following command:
    # softwareupdate --install-rosetta --agree-to-license
    [[ ${ARCH} == "arm64" && ${HOST_ARCH} == "x64" ]] && NORUN=1

  elif [[ ${OS} == "mingw" ]]; then

    [[ ${CC} == "gcc"   ]] && BUILD+=("x86_64-w64-mingw32-${GCC:-gcc}")
    [[ ${CC} == "clang" ]] && BUILD+=("${CLANG:-clang} -fuse-ld=lld -target x86_64-w64-mingw32")

    BUILD+=("-static")
    RUN+=("env WINEDEBUG=-all wine")
    env WINEDEBUG=-all wine --version

  elif [[ ${OS} == "wasi" ]]; then

    BUILD+=("${CLANG:-clang} -mbulk-memory -msimd128 -fuse-ld=lld -target wasm32-wasi")
    [[ ${DISTRIB_ID:-} == "Arch" ]] && BUILD+=("--sysroot=/usr/share/wasi-sysroot")
    [[ ${HOST_OS} == "Darwin"    ]] && BUILD+=("--sysroot=/opt/homebrew/share/wasi-sysroot")

    RUN+=("wasmtime")
    wasmtime --version

  fi

  arg "c"    && BUILD+=("-x c")
  arg "c++"  && BUILD+=("-x c++")
  arg "sse4" && BUILD+=("-msse4.2")
  BUILD+=(${CFLAGS})
  for x in ${ARGS[@]+"${ARGS[@]}"}; do [[ ${x} == -* ]] && BUILD+=(${x}); done
  BUILD+=(${INPUT})
  BUILD+=(${LDFLAGS})

  ${BUILD[0]} --version | head -1
  ${BUILD[@]} -o ${OUTPUT}

  if [[ ${NORUN} == 1 ]]; then
    echo "[compile only]"
  else
    ${RUN[@]:-} ./${OUTPUT}
  fi
}

if [[ ${HOST_OS} == "Linux" ]]; then
  xrun linux gcc   x64
  xrun linux clang x64
  xrun linux gcc   arm64
  xrun linux clang arm64
  xrun linux gcc   rv64
  xrun linux clang rv64
  xrun mingw gcc   x64
  xrun mingw clang x64
  xrun wasi  clang wasm32
elif [[ ${HOST_OS} == "Darwin" ]]; then
  xrun macos clang x64
  xrun macos clang arm64
  xrun wasi  clang wasm32
fi

echo === OK ===
