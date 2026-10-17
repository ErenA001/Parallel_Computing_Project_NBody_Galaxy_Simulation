# Project Structure

## src/

C++17 implementation of the N-Body galaxy simulator,
including gravitational algorithms, OpenMP, and raylib.

The implementation currently remains in one C++ source
file. Refactoring it into modules should be handled in
a separate reviewed change.

## tests/

OpenMP environment test.

## benchmarks/results/

Actual measurement files exported from the simulator.

## docs/media/

Application screenshots and performance charts.

## scripts/

macOS build, application launch, and OpenMP test scripts.

## .github/

Pull request review template.

## Private and Local Files

The `.gitignore` excludes machine-specific binaries,
backups, credentials, environment files, and recordings.

Important: ignored files are not necessarily removed
from older Git commits. Previously exposed credentials
must be rotated, and history cleanup must be coordinated
with collaborators.
