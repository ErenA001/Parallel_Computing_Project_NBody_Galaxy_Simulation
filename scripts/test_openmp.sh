#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

mkdir -p .build

clang++ -O2 -std=c++17 \
  -Xpreprocessor -fopenmp \
  -I"$(brew --prefix libomp)/include" \
  -L"$(brew --prefix libomp)/lib" \
  tests/test_omp.cpp \
  -o .build/test_omp \
  -lomp

./.build/test_omp
