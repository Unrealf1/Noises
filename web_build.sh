#!/bin/sh


if [ -z "${EMSDK:-}" ]; then
  printf 'Error: Environmental variable EMSDK is not set, do not know how to build for web\n' >&2
  exit 1
fi

EMROOT="${EMSDK}/upstream/emscripten"

./build_from_scratch.sh -DCMAKE_BUILD_TYPE=Release -DENABLE_SANITIZER_ADDRESS=False -DCMAKE_TOOLCHAIN_FILE=$EMROOT/cmake/Modules/Platform/Emscripten.cmake -DWEB_BUILD="${EMROOT}" $@

