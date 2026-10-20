# ============================================================
# N-Body Galaxy Simulator — Makefile (macOS / Apple Silicon)
# ============================================================
# Targets:
#   make          → build 2D simulator (./nbody)
#   make 3d       → build 3D simulator (./nbody3d)
#   make all      → build both
#   make test     → build & run OpenMP test
#   make clean    → remove compiled binaries
#   make run      → build + launch 2D
#   make run3d    → build + launch 3D
# ============================================================

CXX      := clang++
CXXFLAGS := -O3 -std=c++17 -ffast-math -Xpreprocessor -fopenmp

OMP_INC  := $(shell brew --prefix libomp)/include
OMP_LIB  := $(shell brew --prefix libomp)/lib
RAY_INC  := $(shell brew --prefix raylib)/include
RAY_LIB  := $(shell brew --prefix raylib)/lib

INCLUDES := -I$(OMP_INC) -I$(RAY_INC)
LDFLAGS  := -L$(OMP_LIB) -L$(RAY_LIB) -lomp -lraylib \
            -framework OpenGL -framework Cocoa \
            -framework IOKit -framework CoreVideo

TEST_FLAGS := -O2 -std=c++17 -Xpreprocessor -fopenmp \
              -I$(OMP_INC) -L$(OMP_LIB) -lomp

# ── default target ────────────────────────────────────────
.PHONY: all 2d 3d test run run3d clean

all: 2d 3d

2d: nbody

3d: nbody3d

nbody: src/nbody.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)
	@echo "✓ Built: ./nbody (2D)"

nbody3d: src/nbody3d.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS)
	@echo "✓ Built: ./nbody3d (3D)"

test:
	mkdir -p .build
	$(CXX) $(TEST_FLAGS) tests/test_omp.cpp -o .build/test_omp
	./.build/test_omp

run: nbody
	./nbody

run3d: nbody3d
	./nbody3d

clean:
	rm -f nbody nbody3d .build/test_omp
	@echo "✓ Cleaned"
