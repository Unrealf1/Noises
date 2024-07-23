#!/bin/sh

EMROOT=/home/fedor/ExternalRepos/emsdk/upstream/emscripten

./build_from_scratch.sh -DCMAKE_BUILD_TYPE=Release -DENABLE_SANITIZER_ADDRESS=False -DCMAKE_TOOLCHAIN_FILE=$EMROOT/cmake/Modules/Platform/Emscripten.cmake -DWEB_BUILD="/home/fedor/ExternalRepos/emsdk/upstream/emscripten" $@

