#!/bin/sh

./build_from_scratch.sh -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZER_THREAD=True $@

