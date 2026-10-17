# Benchmark Data

This directory contains actual measurements generated
by the N-Body Galaxy Simulator.

## Results

CSV files are stored in `results/`.

Recorded metrics:

- threads
- ms_per_step
- speedup
- efficiency
- build_ms
- force_ms

## Reproducibility

For meaningful performance comparisons, use the same:

- Algorithm
- Particle count
- Simulation scenario
- Hardware
- Compiler configuration
- Benchmark settings

Barnes-Hut is an approximation and should not be
treated as numerically identical to the direct solver.

Measurements may change with system load, temperature,
thread scheduling, and power configuration.

New CSV exports are written to `benchmarks/results/`.
