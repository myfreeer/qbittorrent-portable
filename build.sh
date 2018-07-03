#!/bin/bash
git submodule update --init --recursive
MACHINE="$(gcc -dumpmachine)"
# echo Building...
mkdir -p "build_${MACHINE}"
cd "build_${MACHINE}"
cmake -GNinja ..
ninja
ninja check
cd ..