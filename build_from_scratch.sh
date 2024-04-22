#!/bin/sh

echo "Building the project..."

if cmake -B build $@; then
    echo "finished cmake config"
else
    echo "cmake failed!"
    exit 1
fi

if ./build.sh; then
    echo "build ready"
else 
    echo "build failed!"
    exit 2
fi

