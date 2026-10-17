<div align="center">

# 🌌 N-BODY GALAXY SIMULATOR

### Parallel Computing · Barnes-Hut · OpenMP · C++17

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus)
![OpenMP](https://img.shields.io/badge/Parallelism-OpenMP-5CE1E6?style=flat-square)
![raylib](https://img.shields.io/badge/Graphics-raylib-8B5CF6?style=flat-square)
![macOS](https://img.shields.io/badge/Platform-macOS-333?style=flat-square&logo=apple)

**An interactive N-body galaxy simulator for exploring parallel algorithms, gravitational interactions, and CPU performance.**

*Screenshots will appear after capturing the actual application.*

</div>

---

## Overview

This project implements a two-dimensional gravitational
N-body simulation using C++17, OpenMP, and raylib.

Its primary purpose is to investigate how algorithm
selection and CPU parallelization influence the
performance of computationally intensive simulations.

The project compares:

- Direct all-pairs gravitational force calculation.
- Barnes-Hut hierarchical force approximation.
- Single-threaded and multi-threaded execution.

The simulator includes interactive visualizations,
performance measurements, and benchmark exports.

> This is an educational computational model using
> normalized units, not a validated astrophysical
> prediction system. CPU threads calculate physics;
> raylib provides graphics.

## Features

- Five interactive simulation scenarios.
- Naive O(N²) gravitational force calculation.
- Barnes-Hut quadtree approximation.
- OpenMP-based parallel force evaluation.
- Adjustable CPU thread count.
- Variable particle counts.
- Interactive camera controls.
- Color visualization modes.
- Quadtree visualization.
- Live simulation statistics.
- Runtime and speedup benchmarking.
- CSV result export.
- Dark-space-themed interface.

## Simulation Scenarios

| Scenario | Description |
|---|---|
| Galaxy Collision | Two interacting disk galaxies |
| Single Spiral Galaxy | Rotating spiral structure |
| Triple Galaxy Merger | Three interacting galaxies |
| Cold Collapse | Gravitational collapse of a particle cloud |
| Mini Solar System | Central mass, planets, and an asteroid belt |

## Algorithms

### 1. Naive Direct Summation

In the direct approach, the force acting on each
particle is calculated by considering all other
particles.

For N particles, computational complexity grows
quadratically:

**O(N²)**

This approach provides a straightforward baseline
and is suitable for studying data parallelism.

The outer per-particle force loop is parallelized
using OpenMP with static scheduling.

### 2. Barnes-Hut Algorithm

Barnes-Hut reduces computational work by organizing
particles into a hierarchical spatial tree.

This implementation uses a two-dimensional quadtree.

Distant particle groups can be approximated using
their combined mass and center of mass.

The opening angle parameter, theta, controls the
accuracy-performance trade-off.

Typical computational complexity is:

**O(N log N)**

This is an expected scaling behavior under favorable
conditions, not a guarantee for every distribution.

The tree is built serially, while force queries are
parallelized using OpenMP dynamic scheduling.

### 3. Numerical Integration

Particle velocity and position are updated using
a fixed timestep and semi-implicit Euler-style
integration.

The force model includes gravitational softening
to reduce numerical singularities.

The simulation uses deterministic initial seeds.
Numerical and approximation errors should be
considered when interpreting the results.

## Parallel Computing

OpenMP distributes independent particle-force
calculations among CPU threads.

The experiment demonstrates:

- Data parallelism.
- Work distribution.
- Static versus dynamic scheduling.
- Parallel speedup.
- Parallel efficiency.
- Serial bottlenecks and Amdahl's law.

## Installation — macOS

### Requirements

- macOS
- Xcode Command Line Tools
- Homebrew
- C++17 compiler
- OpenMP / libomp
- raylib

Apple Silicon is the primary development target.

### Install Dependencies

```bash
xcode-select --install
brew install libomp raylib gh
```

Skip the first command if Command Line Tools
are already installed.

### Clone and Build

```bash
git clone https://github.com/ErenA001/Parallel_Computing_Project_NBody_Galaxy_Simulation.git
cd Parallel_Computing_Project_NBody_Galaxy_Simulation
./scripts/build_macos.sh
./nbody
```

If the repository is already downloaded:

```bash
./scripts/build_macos.sh
./nbody
```

The physics runs on CPU threads. The graphical
interface uses raylib and OpenGL.

Target render FPS: **120**. Actual FPS depends on hardware and workload.

## Controls

| Input | Function |
|---|---|
| Space | Pause / resume |
| A | Naive / Barnes-Hut |
| Up / Down | Change OpenMP threads |
| 1 / M | One / maximum threads |
| Left / Right | Change particle count |
| Z / X | Simulation speed |
| G | Toggle quadtree overlay |
| K / L | Barnes-Hut theta |
| C | Change color mode |
| R | Reset simulation |
| H | Show / hide HUD |
| 0 | Reset camera |
| Esc / Tab | Return to menu |
| F | Fullscreen |
| Q / E | Zoom out / in |
| Mouse wheel | Zoom |
| Mouse drag | Pan camera |

## Screenshots

Screenshots in this repository are captured from
the actual running application.

*Real application screenshots pending.*


## Demo Video

*Real demo video pending.*

## Benchmark and Performance

The application measures execution time per
simulation step and exports results to CSV.

Output columns:

- `threads`
- `ms_per_step`
- `speedup`
- `efficiency`
- `build_ms`
- `force_ms`

### Speedup

S(p) = T(1) / T(p)

### Parallel Efficiency

E(p) = S(p) / p

Measurements should be compared under equivalent
particle counts, algorithms, initial conditions,
and benchmark configurations.

Different CSV workloads must not be treated
as identical experiments.

### Recorded Benchmark Results

The table below is generated from actual
CSV measurements available in this repository.

| CSV | Algorithm | Fastest Tested Threads | Speedup | Step Time |
|---|---|---:|---:|---:|
| [benchmark_bh_100003.csv](benchmark_bh_100003.csv) | Barnes-Hut | 12 | 5.69x | 31.44 ms |
| [benchmark_bh_10002.csv](benchmark_bh_10002.csv) | Barnes-Hut | 12 | 4.55x | 3.02 ms |
| [benchmark_bh_10009.csv](benchmark_bh_10009.csv) | Barnes-Hut | 12 | 4.76x | 3.01 ms |
| [benchmark_bh_20002.csv](benchmark_bh_20002.csv) | Barnes-Hut | 12 | 4.90x | 6.08 ms |
| [benchmark_naive_20002.csv](benchmark_naive_20002.csv) | Naive | 8 | 7.93x | 19.54 ms |
| [benchmark_naive_20009.csv](benchmark_naive_20009.csv) | Naive | 8 | 7.53x | 20.61 ms |

### Measured Speedup

![Measured Speedup](docs/media/speedup.png)

### Measured Runtime

![Measured Runtime](docs/media/step-time.png)



### Interpretation

Adding more threads does not guarantee linear
performance scaling.

Potential limiting factors include:

- Sequential tree construction.
- Thread scheduling overhead.
- Memory-system limitations.
- CPU performance and efficiency core differences.
- Load imbalance.

Measurements may vary with temperature, power
settings, compiler options, and system activity.

## Repository Structure

```text
nbody.cpp
scripts/
  build_macos.sh
docs/
  media/
    menu.png
    simulation.png
    benchmark.png
    demo.mp4
    speedup.png
    step-time.png
benchmark_*.csv
CONTRIBUTING.md
README.md
```

Media files are present only after successful capture.

## Collaboration

The project is intended for a four-member team.

Each member should work on dedicated feature
branches and submit changes through pull requests.

See [CONTRIBUTING.md](CONTRIBUTING.md).

## Academic Integrity

The instructor prohibits AI-generated project
report content.

The required academic project report must be
written independently by the student team using
their own research, work, and measurements.

This README is technical repository documentation,
not the academic project report.

## References

1. Barnes, J., & Hut, P. (1986).
   *A hierarchical O(N log N) force-calculation algorithm.*
   Nature, 324, 446-449.
   https://doi.org/10.1038/324446a0

2. Amdahl, G. M. (1967).
   *Validity of the single processor approach to
   achieving large scale computing capabilities.*
   AFIPS Spring Joint Computing Conference.
   https://doi.org/10.1145/1465482.1465560

3. OpenMP Specifications:
   https://www.openmp.org/specifications/

4. raylib:
   https://www.raylib.com/

---

<div align="center">

**Parallel Computing Course Project**

*Educational and research demonstration*

</div>
