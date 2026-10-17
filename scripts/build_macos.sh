#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew kurulu degil: https://brew.sh"
  exit 1
fi

for pkg in libomp raylib; do
  if ! brew --prefix "$pkg" >/dev/null 2>&1; then
    echo "$pkg eksik: brew install libomp raylib"
    exit 1
  fi
done

clang++ -O3 -std=c++17 -ffast-math \
  -Xpreprocessor -fopenmp \
  -I"$(brew --prefix libomp)/include" \
  -I"$(brew --prefix raylib)/include" \
  -L"$(brew --prefix libomp)/lib" \
  -L"$(brew --prefix raylib)/lib" \
  nbody.cpp -o nbody -lomp -lraylib \
  -framework OpenGL \
  -framework Cocoa \
  -framework IOKit \
  -framework CoreVideo

echo "Build basarili: ./nbody"
