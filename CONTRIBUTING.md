# Contributing — N-Body Galaxy Simulator

This is a four-person Parallel Computing course project.
Every team member should contribute meaningful work and
understand the code they submit.

## Suggested Workstreams

1. Physics and correctness:
   Direct solver, initialization, numerical validation.

2. Parallel programming:
   OpenMP, thread scheduling, race-free execution.

3. Barnes-Hut:
   Quadtree, opening angle, performance comparisons.

4. Visualization and evaluation:
   raylib interface, benchmarks, plots, documentation.

These are suggested responsibilities, not claims about
who has already completed particular contributions.

## Git Workflow

Start from the latest main branch:

```bash
git switch main
git pull --ff-only
git switch -c feature/my-work
```

After implementing and testing changes:

```bash
git add .
git commit -m "Describe implemented changes"
git push -u origin feature/my-work
gh pr create --fill
```

Request a teammate's review before merging.

## Development Principles

- Keep main stable.
- Never commit compiled binaries or virtual environments.
- Do not overwrite another member's code without review.
- Validate changes before submitting pull requests.
- Preserve meaningful commit history.
- Report genuine measurements, not estimated results.
- Document limitations and known issues.

## Academic Integrity

The course instructor prohibits AI-generated project
report content.

The academic project report must be written by the
students based on their own work, understanding,
measurements, and research.

Repository documentation is not a substitute for
the required academic report.
