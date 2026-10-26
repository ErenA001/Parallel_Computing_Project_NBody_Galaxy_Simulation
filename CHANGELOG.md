# Changelog

All notable changes to this project are documented here.

---

## [Unreleased]

### Added
- `CMakeLists.txt` — cross-platform build for Windows (MSVC / MinGW), macOS and Linux;
  fetches raylib automatically when it is not installed
- CI: CMake build matrix on Ubuntu, Windows and macOS with OpenMP test via `ctest`
- README: Windows / Linux / CMake installation and run instructions
- `src/nbody3d.cpp` — 3D Barnes-Hut octree simulator with perspective camera,
  orbit controls, and full 3D disk/sphere initial conditions for all five scenarios
- `scripts/build_macos_3d.sh` — build script for the 3D simulator
- `scripts/run_macos_3d.sh` — convenience launcher for the 3D simulator
- `Makefile` — unified build system (`make`, `make 3d`, `make all`, `make test`, `make clean`)
- `.github/workflows/build.yml` — GitHub Actions CI: builds both simulators and
  runs the OpenMP test on every push and pull request to `main`
- `.github/ISSUE_TEMPLATE/bug_report.md` — structured bug report template
- `.github/ISSUE_TEMPLATE/feature_request.md` — structured feature request template
- `CHANGELOG.md` — this file

### Changed
- `.gitignore` / `.gitattributes` — ignore CMake `build/` directories, normalize CMake file line endings
- `.gitignore` — consolidated and cleaned up; now also ignores `/nbody3d` binary

---

## [1.0.0] — 2026-10-17

### Added
- Initial 2D N-Body Galaxy Simulator (`src/nbody.cpp`)
  - Naive O(N²) direct summation
  - Barnes-Hut quadtree O(N log N) approximation
  - OpenMP parallel force evaluation (static + dynamic scheduling)
  - Five interactive scenarios: Galaxy Collision, Single Spiral Galaxy,
    Triple Galaxy Merger, Cold Collapse, Mini Solar System
  - Interactive raylib UI with camera controls, HUD, and quadtree overlay
  - Runtime benchmarking with CSV export
- `scripts/build_macos.sh` — macOS build script (2D)
- `scripts/run_macos.sh` — macOS launch script (2D)
- `scripts/test_openmp.sh` — OpenMP environment verification
- `tests/test_omp.cpp` — OpenMP thread count test
- `benchmarks/results/` — recorded benchmark CSV files
- `docs/` — screenshots and project structure documentation
- `README.md`, `CONTRIBUTING.md`
- `.github/pull_request_template.md`
