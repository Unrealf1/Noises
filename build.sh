#!/bin/sh

echo "Building the project..."

if [ ! -d "./build" ]; then
    ./build_from_scratch.sh $@
fi

num_procs=$(nproc)
echo "building using ${num_procs} workers"
if cmake --build build -j${num_procs}; then
    echo "build ready"
else 
    echo "build failed!"
    exit 2
fi

