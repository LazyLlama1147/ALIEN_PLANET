#!/bin/bash

mkdir -p build

# Optional: Only copy if these folders exist
if [ -d "resources/obj" ]; then
    cp -r resources/obj/* build/
fi

if [ -d "resources/shaders" ]; then
    cp -r resources/shaders/* build/
fi

cmake -S . -B build
cd build
make

if [[ $* == *-p* ]] || [[ $* == *--pack* ]]; then
    cd ..
    tar -czvf build.tar.gz build
    cd build
fi

if [[ $* == *-r* ]] || [[ $* == *--run* ]]; then
    ./FINAL_334CS
fi