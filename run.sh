#!/usr/bin/env bash
set -e

if ! sh3 version &>/dev/null; then
    echo "Warning: sh3 not found or does not function correctly, falling back to simple build strategy"
    set -x
    c++ -Wall -Wextra -pedantic -std=gnu++17 -o main src/main.cpp -O3 -flto
    set +x
else
    sh3 build
fi
./main
