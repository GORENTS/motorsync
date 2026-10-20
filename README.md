# MotorSync: Synchronization Library for Frame-Based Simulations

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build](https://github.com/stepanyan-gor/motorsync/actions/workflows/build.yml/badge.svg)](https://github.com/stepanyan-gor/motorsync/actions)

## What is MotorSync?

MotorSync is a C++17 library that implements **synchronized frame-based simulation** based on the principles of **motional ontology** — a framework where reality is modeled as a sequence of discrete frames updated by a "motor measurement" process.

The library provides:

- **Field-like shared data** — atomic shared state.
- **ExternalInput** — queue for non-computable external commands (analog of "consciousness").
- **TickBarrier** — barrier that synchronizes all measurements on each frame.
- **Motor** — processor that: (1) runs all measurements in parallel, (2) waits at the barrier, (3) applies external input, (4) holds the target period, (5) optionally performs sequential commit.

## Minimal Example

```cpp
#include "motor_sync.hpp"

int main() {
    // 4 measurements, 1000 frames, target period 2000 us, sequential commit
    motor::Motor m(4, 1000, std::chrono::microseconds(2000), /*seq=*/true);

    for (int i = 0; i < 4; ++i) {
        m.add(
            [](std::uint64_t, int) { /* work: simulate */ },
            [](std::uint64_t, int) { /* commit: write to shared */ }
        );
    }

    m.run();   // 1000 frames, all measurements synchronized
}
```

## Key Guarantees

- **Work before commit.** All work-phases complete before any commit-phase begins (`TickBarrier` includes the main thread, `n_measurements + 1`).
- **Deterministic commit order.** With `sequential_commit=true`, commits happen strictly in order `0, 1, ..., N-1` (verified by Test B: 1 unique hash out of 10 runs).
- **Period accuracy.** With `target_period > 0`, each frame duration is held with `sleep_until` + spin-wait. Precision ~1 μs on Windows with `timeBeginPeriod(1)`; ~10 μs on Linux/macOS without.
- **Max throughput.** With `target_period == 0`, no sleep/spin is performed: the Motor runs at maximum speed (Test C).

## Key Results

### Test A2: Heterogeneous Load

1 heavy measurement (~800 μs) + 3 light (~50 μs). Period 2000 μs, 1000 frames × 10 runs.

| Metric | Motor | Baseline | Improvement |
|--------|-------|----------|-------------|
| commit gap mean (μs) | 38.5 | 1062.5 | **96.3%** |
| commit gap p50 (μs) | 30.7 | 828.9 | **96.3%** |
| commit gap p99 (μs) | 180.9 | 4878.9 | 96.3% |

**Conclusion:** With heterogeneous load, the barrier gives a **96% improvement in commit atomicity** (data is committed almost simultaneously).

### Test B: Determinism

Non-commutative updates, 1000 frames, 10 runs.

| Variant | Unique hashes out of 10 | Verdict |
|---------|-------------------------|---------|
| **Motor-seq** | **1** | **Deterministic** |
| Motor-racy | 10 | Non-deterministic |
| Baseline | 10 | Non-deterministic |

**Conclusion:** For determinism, a barrier alone is insufficient — a **fixed commit order** is also required.

### Test C: Throughput (No Period)

100 000 frames, 4 light measurements, `target_period = 0` (no sleep/spin).

| Version | Frames per second (fps) |
|---------|-------------------------|
| Motor | ~333 000 |
| Baseline | ~11 000 000 |

**Conclusion:** The synchronization barrier incurs a **significant overhead** (~33× slower) when the work is extremely light and no period is set.
This is the cost of context switching (mutex/condvar) on every frame. 
The barrier is designed for scenarios where **atomicity and determinism matter more than raw throughput** (e.g., Test A2). 
If maximum throughput is needed without synchronization, use standard atomic operations instead.

## Quick Start

### Build with CMake

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

Then run any benchmark:

```bash
./test_a2
./test_b
./test_c
```

### Build manually (GCC / Clang)

```bash
g++ -std=c++17 -O2 -pthread -Iinclude benchmarks/test_a2.cpp -o test_a2
./test_a2
```

### Build with Visual Studio

Visual Studio cannot compile two `.cpp` files with `main()` in one project (C++ linker limitation: `error LNK2005: _main already defined`). Two options:

1. **Use CMake** (recommended) — it builds each benchmark into a separate `.exe` automatically:
   ```
   cmake -B build
   cmake --build build --config Release
   ```
2. **Create separate projects** — one per benchmark (`test_a2`, `test_b`, `test_c`), each with the corresponding `.cpp` + `include/` headers.

### Windows note

For accurate timing on Windows, the benchmarks call `timeBeginPeriod(1)`. This is a **global OS setting** that increases timer resolution to 1 ms but also increases power consumption. It is enabled only for the duration of the benchmark. On Linux/macOS, precision is lower (~10 μs without real-time patches).

## Repository Structure

```
motorsync/
├── include/
│   ├── motor_sync.hpp
│   └── baseline.hpp
├── benchmarks/
│   ├── test_a2.cpp        # Heterogeneous load
│   ├── test_b.cpp         # Determinism
│   └── test_c.cpp         # Throughput (no period)
├── results/               # CSV results
├── docs/
│   ├── math_model.md      # Mathematical model (theorems)
│   └── *.pdf              # Philosophy book, physics article (Russian)
├── .github/workflows/
│   └── build.yml          # GitHub Actions: build on Linux/Windows/macOS
├── CMakeLists.txt
├── README.md
└── LICENSE
```

## Interactive Example: World Model

Run world_model to see a synchronized 2D simulation with external input (press 1, 2, 3 to kick the balls).

## Theory Behind MotorSync

MotorSync is based on **motional ontology**:

- **Reality** is a sequence of discrete 3D frames.
- **Time** is a frame counter, not a fundamental dimension.
- **Motor measurement** is the process that creates the next frame from the current one.
- **Fields** are passive data inside a frame.
- **Measurements** are active processes that update fields.
- **Synchronization** of all measurements and fields is necessary for a stable universe.
- **Determinism** requires both a synchronization barrier and a fixed commit order.

See [`docs/math_model.md`](docs/math_model.md) for the formal model and theorems.

## Applications

- **Simulations with heterogeneous load** — physics + AI + network in one frame.
- **Game servers** with deterministic simulation.
- **World models / predictive coding** — synchronization of modalities (video, audio, text).
- **Distributed systems** — consensus via a shared barrier.
- **Neuroscience** — synchronization of brain rhythms.
- **Quantum computing** — quantum parallelism.

## License

MIT License. See [LICENSE](LICENSE).

## Citation

```
Stepanyan, G. (2026). MotorSync: Synchronization Library for Frame-Based Simulations.
GitHub: https://github.com/stepanyan-gor/motorsync
```

## Contact

Gor Stepanyan — stepanyan-gor@yandex.ru

---

# MotorSync: Библиотека синхронизации для покадровых симуляций

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build](https://github.com/stepanyan-gor/motorsync/actions/workflows/build.yml/badge.svg)](https://github.com/stepanyan-gor/motorsync/actions)

## Что такое MotorSync?

MotorSync — библиотека C++17, реализующая **синхронизированную покадровую симуляцию** на основе принципов **моторной онтологии** — модели, в которой реальность представлена как последовательность дискретных кадров, обновляемых процессом «моторного измерения».

Библиотека предоставляет:

- **Общие атомарные данные** (аналог «поля»).
- **ExternalInput** — очередь внешних команд (аналог «сознания»).
- **TickBarrier** — барьер синхронизации всех измерений на каждом кадре.
- **Motor** — процессор: work → barrier → commit (опционально sequential) → период.

## Минимальный пример

```cpp
#include "motor_sync.hpp"

int main() {
    motor::Motor m(4, 1000, std::chrono::microseconds(2000), true);

    for (int i = 0; i < 4; ++i) {
        m.add(
            [](std::uint64_t, int) { /* work */ },
            [](std::uint64_t, int) { /* commit */ }
        );
    }

    m.run();
}
```

## Гарантии

- **Work до commit.** Все work-фазы завершаются до начала любой commit-фазы.
- **Порядок commit.** При `sequential_commit=true` коммиты идут строго `0,1,...,N-1` (Тест B: 1 хеш из 10).
- **Точность периода.** При `target_period > 0` — `sleep_until` + spin-wait. ~1 мкс на Windows с `timeBeginPeriod(1)`; ~10 мкс на Linux/macOS.
- **Максимальная пропускная способность.** При `target_period == 0` — без sleep/spin (Тест C).

## Результаты

- **Тест A2 (разная нагрузка):** 95–97% улучшение commit atomicity.
- **Тест B (детерминизм):** Motor-seq даёт 100% воспроизводимость.
- **Тест C (пропускная способность):** накладные расходы барьера — см. `results/test_c_results.csv`.

## Сборка

### CMake (рекомендуется)

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
./test_a2
./test_b
./test_c
```

### Visual Studio

Visual Studio не может собрать два `.cpp` с `main()` в одном проекте. Варианты:

1. **Использовать CMake** (рекомендуется) — каждый тест собирается в отдельный `.exe`.
2. **Отдельные проекты** — по одному на каждый тест.

### Примечание для Windows

Бенчмарки вызывают `timeBeginPeriod(1)` для точности таймера. Это глобальная настройка ОС (влияет на энергопотребление), включена только на время теста. На Linux/macOS точность ниже (~10 мкс).

## Структура

```
motorsync/
├── include/       # motor_sync.hpp, baseline.hpp
├── benchmarks/    # test_a2.cpp, test_b.cpp, test_c.cpp
├── results/       # CSV
├── docs/          # math_model.md, *.pdf
├── .github/workflows/build.yml
├── CMakeLists.txt
├── README.md
└── LICENSE
```

## Теория

MotorSync основан на **моторной онтологии**:
- Реальность — последовательность 3D кадров.
- Время — счётчик кадров.
- Моторное измерение — процесс, создающий следующий кадр.
- Поля — пассивные данные.
- Измерения — активные процессы.
- Синхронизация измерений и полей — условие стабильности.
- Детерминизм = барьер + порядок commit.

Формальная модель и теоремы — в [`docs/math_model.md`](docs/math_model.md).

## Применения

- Симуляции с разнородной нагрузкой (физика + ИИ + сеть).
- Игровые серверы с детерминизмом.
- World models / predictive coding.
- Распределённые системы.
- Нейронаука.
- Квантовые вычисления.

## Лицензия

MIT. См. [LICENSE](LICENSE).

## Контакты

Гор Степанян — stepanyan-gor@yandex.ru