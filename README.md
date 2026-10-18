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
cmake --build . --config Release
```

### Build manually

```bash
g++ -std=c++17 -O2 -pthread benchmarks/test_a2.cpp -o test_a2
./test_a2
```

### Run benchmarks

```bash
./test_a1
./test_a2
./test_b
```

Results are saved to `test_*_results.csv`.

## Repository Structure

```
motorsync/
├── include/
│   ├── motor_sync.hpp      # Motor, TickBarrier, Field, ExternalInput
│   └── baseline.hpp        # Naive implementation (no barrier)
├── benchmarks/
│   ├── test_a1.cpp         # Uniform load
│   ├── test_a2.cpp         # Heterogeneous load
│   └── test_b.cpp          # Determinism
├── results/                # CSV results from benchmarks
├── docs/
│   ├── motor_ontology.pdf  # Philosophical book (Russian)
│   ├── physics_article.pdf # Scientific article (Russian)
│   └── help-memo.pdf       # Full reference v15.1 (Russian)
├── CMakeLists.txt
└── README.md
```

## Theory Behind MotorSync

MotorSync is based on **motional ontology**, a framework where:

- **Reality** is a sequence of discrete 3D frames.
- **Time** is a frame counter, not a fundamental dimension.
- **Motor measurement** is the process that creates the next frame from the current one.
- **Fields** are passive data (coordinates, masses, charges, spins) inside a frame.
- **Measurements** are active processes that update fields.
- **Synchronization** of all measurements and fields is necessary for a stable universe.
- **Determinism** requires both a synchronization barrier and a fixed commit order.

### Key Insight

> The barrier ensures that all measurements finish their work before the next frame begins. The fixed commit order ensures that shared data is updated in a deterministic sequence. Together, they make the simulation **atomic** and **reproducible**.

## Practical Applications

- Simulations with heterogeneous load (physics + AI + network).
- Game servers with deterministic simulation.
- World models / predictive coding: synchronization of modalities (video, audio, text).
- Distributed systems: consensus via a shared barrier.
- Neuroscience: synchronization of brain rhythms.
- Quantum computing: quantum parallelism.

## License

MIT License. See [LICENSE](LICENSE).

## Citation

If you use MotorSync in your research, please cite:

```
Stepanyan, G. (2026). MotorSync: Synchronization Library for Frame-Based Simulations.
GitHub repository: https://github.com/stepanyan-gor/motorsync
```

## Contact

Gor Stepanyan — stepanyan-gor@yandex.ru

---

# MotorSync: Библиотека синхронизации для покадровых симуляций

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Что такое MotorSync?

MotorSync — библиотека C++17, реализующая **синхронизированную покадровую симуляцию** на основе принципов **моторной онтологии** — философско-физической модели, в которой реальность представлена как последовательность дискретных кадров, обновляемых процессом «моторного измерения».

Библиотека предоставляет:

- **Field<T>** — атомарные общие данные (аналог «поля»).
- **ExternalInput** — очередь внешних команд (аналог «сознания»).
- **TickBarrier** — барьер синхронизации всех измерений на каждом кадре.
- **Motor** — процессор: (1) запускает все измерения параллельно, (2) ждёт барьер, (3) применяет внешний ввод, (4) держит целевой период, (5) опционально выполняет последовательный commit.

## Ключевые результаты

Мы провели три бенчмарка для проверки гипотезы о том, что синхронизация улучшает стабильность и детерминизм.

### Тест A1: Одинаковая нагрузка

4 измерения, все с одинаковой нагрузкой (~100 мкс), период 5000 мкс, 5000 кадров × 5 прогонов.

| Метрика | Motor | Baseline | Разница |
|---------|-------|----------|---------|
| mean (мкс) | 4986.9 | 4985.8 | +0.02% |
| median std (мкс) | 551.7 | 558.9 | **1.3%** |
| p50 (мкс) | 4999.1 | 4996.8 | +0.05% |
| p99 (мкс) | 6058 | 6121 | −1% |
| max (мкс) | 6145 | 6290 | −2% |

**Вывод:** улучшения нет. `sleep_until` самокорректируется.

### Тест A2: Разная нагрузка (ключевой)

1 тяжёлое измерение (~800 мкс) + 3 лёгких (~50 мкс). Период 2000 мкс, 1000 кадров × 10 прогонов.

| Метрика | Motor | Baseline | Улучшение |
|---------|-------|----------|-----------|
| jitter std (мкс) | 210 | 620 | **66%** |
| commit gap mean (мкс) | 30 | 750 | **96%** |
| commit gap p50 (мкс) | 20 | 689 | **97%** |
| commit gap p99 (мкс) | 314 | 2047 | 85% |

**Вывод:** при разной нагрузке барьер даёт **95–97% улучшение атомарности commit** и **66% снижение jitter**.

### Тест B: Детерминизм

Каждое измерение обновляет общее состояние **некоммутативной** операцией. 1000 кадров, период 500 мкс, 10 прогонов.

| Вариант | Уникальных хешей из 10 | Вердикт |
|---------|-------------------------|---------|
| **Motor-seq** | **1** | **Детерминирован** |
| Motor-racy | 10 | Недетерминирован |
| Baseline | 10 | Недетерминирован |

**Вывод:** для детерминизма недостаточно только барьера. Нужен ещё **фиксированный порядок commit** (Motor-seq даёт 100% воспроизводимость).

## Быстрый старт

### Требования

- Компилятор C++17 (GCC, Clang или MSVC)
- CMake 3.10+ (опционально)
- Windows: `winmm.lib` (линкуется автоматически через `#pragma comment`)

### Сборка через CMake

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Сборка вручную

```bash
g++ -std=c++17 -O2 -pthread benchmarks/test_a2.cpp -o test_a2
./test_a2
```

### Запуск бенчмарков

```bash
./test_a1
./test_a2
./test_b
```

Результаты сохраняются в `test_*_results.csv`.

## Структура репозитория

```
motorsync/
├── include/
│   ├── motor_sync.hpp      # Motor, TickBarrier, Field, ExternalInput
│   └── baseline.hpp        # Наивная реализация (без барьера)
├── benchmarks/
│   ├── test_a1.cpp         # Одинаковая нагрузка
│   ├── test_a2.cpp         # Разная нагрузка
│   └── test_b.cpp          # Детерминизм
├── results/                # CSV результаты бенчмарков
├── docs/
│   ├── motor_ontology.pdf  # Философская книга (русский)
│   ├── physics_article.pdf # Научная статья (русский)
│   └── help-memo.pdf       # Полная справка v15.1 (русский)
├── CMakeLists.txt
└── README.md
```

## Теория, лежащая в основе MotorSync

MotorSync основан на **моторной онтологии** — модели, в которой:

- **Реальность** — последовательность дискретных 3D кадров.
- **Время** — счётчик кадров, а не фундаментальное измерение.
- **Моторное измерение** — процесс, создающий следующий кадр из текущего.
- **Поля** — пассивные данные (координаты, массы, заряды, спины) внутри кадра.
- **Измерения** — активные процессы, обновляющие поля.
- **Синхронизация** всех измерений и полей необходима для стабильной вселенной.
- **Детерминизм** требует как барьера синхронизации, так и фиксированного порядка commit.

### Ключевая идея

> Барьер гарантирует, что все измерения завершили работу до начала следующего кадра. Фиксированный порядок commit гарантирует, что общие данные обновляются в детерминированной последовательности. Вместе они делают симуляцию **атомарной** и **воспроизводимой**.

## Практическое применение

- Симуляции с разнородной нагрузкой (физика + ИИ + сеть).
- Игровые серверы с детерминированной симуляцией.
- World models / predictive coding: синхронизация модальностей (видео, звук, текст).
- Распределённые системы: консенсус через общий барьер.
- Нейронаука: синхронизация мозговых ритмов.
- Квантовые вычисления: квантовый параллелизм.

## Лицензия

MIT License. См. [LICENSE](LICENSE).

## Цитирование

Если вы используете MotorSync в своих исследованиях, пожалуйста, цитируйте:

```
Stepanyan, G. (2026). MotorSync: Synchronization Library for Frame-Based Simulations.
GitHub repository: https://github.com/stepanyan-gor/motorsync
```

## Контакты

Гор Степанян — stepanyan-gor@yandex.ru
