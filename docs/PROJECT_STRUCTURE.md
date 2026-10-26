# Project Structure

## src/

C++17 implementations of the N-Body galaxy simulator.

- `nbody.cpp` — 2D simulator: Barnes-Hut quadtree, OpenMP, raylib.
- `nbody3d.cpp` — 3D simulator: Barnes-Hut octree, perspective camera,
  full 3D disk/sphere initial conditions for all five scenarios.

Both implementations are contained in a single self-contained C++ source file.
Refactoring into separate modules should be handled in a reviewed change.

## tests/

OpenMP environment test (`test_omp.cpp`).

## benchmarks/results/

Actual measurement files exported from the simulator.

## docs/media/

Application screenshots and performance charts.

## scripts/

macOS build, application launch, and OpenMP test scripts.

- `build_macos.sh` — compile 2D simulator
- `build_macos_3d.sh` — compile 3D simulator
- `run_macos.sh` — launch 2D simulator (builds if needed)
- `run_macos_3d.sh` — launch 3D simulator (builds if needed)
- `test_openmp.sh` — compile and run OpenMP thread test

Cross-platform (Windows / macOS / Linux): use `CMakeLists.txt` in the repository root
(`cmake -S . -B build && cmake --build build --config Release`).

On macOS you can also use `make`, `make 3d`, `make all`, etc. (see `Makefile`).

## .github/

- `workflows/build.yml` — GitHub Actions CI: macOS Makefile build plus a CMake
  build matrix (Ubuntu, Windows, macOS) with the OpenMP test, on every push and PR to `main`.
- `ISSUE_TEMPLATE/` — bug report and feature request templates.
- `pull_request_template.md` — PR review checklist.

## Private and Local Files

The `.gitignore` excludes machine-specific binaries (`/nbody`, `/nbody3d`),
backups, credentials, environment files, and recordings.

Important: ignored files are not necessarily removed
from older Git commits. Previously exposed credentials
must be rotated, and history cleanup must be coordinated
with collaborators.
