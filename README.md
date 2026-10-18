# MotorSync: Synchronization Library for Frame-Based Simulations

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## What is MotorSync?

MotorSync is a C++17 library that implements **synchronized frame-based simulation** based on the principles of **motional ontology** — a philosophical and physical framework where reality is modeled as a sequence of discrete frames updated by a "motor measurement" process.

The library provides:

- **Field<T>** — atomic shared data (analog of a "field").
- **ExternalInput** — queue for non-computable external commands (analog of "consciousness").
- **TickBarrier** — barrier that synchronizes all measurements on each frame.
- **Motor** — processor that: (1) runs all measurements in parallel, (2) waits at the barrier, (3) applies external input, (4) holds the target period, (5) optionally performs sequential commit.

## Key Results

We ran three benchmarks to test the hypothesis that synchronization improves stability and determinism.

### Test A1: Uniform Load

4 measurements, all with the same load (~100 μs), period 5000 μs, 5000 frames × 5 runs.

| Metric | Motor | Baseline | Difference |
|--------|-------|----------|------------|
| mean (μs) | 4986.9 | 4985.8 | +0.02% |
| median std (μs) | 551.7 | 558.9 | **1.3%** |
| p50 (μs) | 4999.1 | 4996.8 | +0.05% |
| p99 (μs) | 6058 | 6121 | −1% |
| max (μs) | 6145 | 6290 | −2% |

**Conclusion:** No improvement. `sleep_until` is self-correcting.

### Test A2: Heterogeneous Load (Key)

1 heavy measurement (~800 μs) + 3 light measurements (~50 μs). Period 2000 μs, 1000 frames × 10 runs.

| Metric | Motor | Baseline | Improvement |
|--------|-------|----------|-------------|
| jitter std (μs) | 210 | 620 | **66%** |
| commit gap mean (μs) | 30 | 750 | **96%** |
| commit gap p50 (μs) | 20 | 689 | **97%** |
| commit gap p99 (μs) | 314 | 2047 | 85% |

**Conclusion:** With heterogeneous load, the barrier gives **95–97% improvement in commit atomicity** and **66% reduction in jitter**.

### Test B: Determinism

Each measurement updates shared state with a **non-commutative** operation. 1000 frames, period 500 μs, 10 runs.

| Variant | Unique hashes out of 10 | Verdict |
|---------|-------------------------|---------|
| **Motor-seq** | **1** | **Deterministic** |
| Motor-racy | 10 | Non-deterministic |
| Baseline | 10 | Non-deterministic |

**Conclusion:** For determinism, a barrier alone is insufficient. **Fixed commit order** is also required (Motor-seq gives 100% reproducibility).

## Quick Start

### Prerequisites

- C++17 compiler (GCC, Clang, or MSVC)
- CMake 3.10+ (optional)
- Windows: `winmm.lib` (linked automatically via `#pragma comment`)

### Build with CMake

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release# motorsync
