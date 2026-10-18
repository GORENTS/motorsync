// test_b.cpp
// =====================================================================
// ТЕСТ B: детерминизм.
//
// Каждое измерение обновляет общее значение НЕКОММУТАТИВНОЙ операцией:
//     shared = (shared * 3 + id + 1) % MODULUS
//
// Порядок применения операций влияет на финальный результат.
//
// Три варианта:
//   1. Motor-seq  : барьер + commit в порядке 0→1→2→3.
//   2. Motor-racy : барьер, но commit гонкой.
//   3. Baseline   : без барьера, commit гонкой.
//
// Ожидание:
//   Motor-seq  — 10/10 прогонов дают ОДИН хеш (детерминизм).
//   Motor-racy — разные хеши (недетерминизм).
//   Baseline   — разные хеши (недетерминизм).
// =====================================================================

#ifdef _WIN32
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  pragma comment(lib, "winmm.lib")
#endif

#include "motor_sync.hpp"
#include "baseline.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------
// Нагрузка (лёгкая, чтобы кадр укладывался в период)
// ---------------------------------------------------------------------
void light_work() {
    volatile double x = 0.0;
    for (int i = 0; i < 3000; ++i) x += std::sin(i * 0.0001);
    (void)x;
}

// ---------------------------------------------------------------------
// Некоммутативная операция обновления общего состояния
// ---------------------------------------------------------------------
constexpr std::uint64_t MODULUS = 1000000007ULL;

inline std::uint64_t update(std::uint64_t v, int id) {
    return (v * 3ULL + static_cast<std::uint64_t>(id) + 1ULL) % MODULUS;
}

// Атомарный CAS-commit (безопасно без блокировки, но порядок не определён)
inline void atomic_commit(std::atomic<std::uint64_t>& shared, int id) {
    std::uint64_t old = shared.load(std::memory_order_acquire);
    std::uint64_t neu;
    do {
        neu = update(old, id);
    } while (!shared.compare_exchange_weak(
        old, neu,
        std::memory_order_acq_rel,
        std::memory_order_acquire));
}

// ---------------------------------------------------------------------
// MAIN
// ---------------------------------------------------------------------
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    timeBeginPeriod(1);
#endif

    constexpr std::uint64_t N_TICKS = 1000;
    constexpr std::size_t   N_MEAS = 4;
    constexpr int           N_RUNS = 10;
    const auto PERIOD = std::chrono::microseconds(500);  // 0.5 ms

    std::vector<std::uint64_t> motor_seq_hashes;
    std::vector<std::uint64_t> motor_racy_hashes;
    std::vector<std::uint64_t> baseline_hashes;

    for (int run = 0; run < N_RUNS; ++run) {
        std::printf("\n--- Run %d/%d ---\n", run + 1, N_RUNS);

        // ===== Motor-seq =====
        {
            std::atomic<std::uint64_t> shared{ 1 };
            motor::Motor m(N_MEAS, N_TICKS, PERIOD, /*seq=*/true);
            for (std::size_t i = 0; i < N_MEAS; ++i) {
                m.add(
                    // work
                    [](std::uint64_t, int) { light_work(); },
                    // commit: sequential — только один поток в commit
                    [&shared](std::uint64_t, int id) {
                        std::uint64_t v = shared.load(std::memory_order_acquire);
                        v = update(v, id);
                        shared.store(v, std::memory_order_release);
                    }
                );
            }
            m.run();
            motor_seq_hashes.push_back(shared.load());
            std::printf("  Motor-seq    hash = %llu\n",
                (unsigned long long)shared.load());
        }

        // ===== Motor-racy =====
        {
            std::atomic<std::uint64_t> shared{ 1 };
            motor::Motor m(N_MEAS, N_TICKS, PERIOD, /*seq=*/false);
            for (std::size_t i = 0; i < N_MEAS; ++i) {
                m.add(
                    [](std::uint64_t, int) { light_work(); },
                    [&shared](std::uint64_t, int id) {
                        atomic_commit(shared, id);
                    }
                );
            }
            m.run();
            motor_racy_hashes.push_back(shared.load());
            std::printf("  Motor-racy   hash = %llu\n",
                (unsigned long long)shared.load());
        }

        // ===== Baseline =====
        {
            std::atomic<std::uint64_t> shared{ 1 };
            baseline::NaiveMotor nb(N_TICKS, PERIOD);
            for (std::size_t i = 0; i < N_MEAS; ++i) {
                nb.add(
                    [](std::uint64_t, int) { light_work(); },
                    [&shared](std::uint64_t, int id) {
                        atomic_commit(shared, id);
                    }
                );
            }
            nb.run();
            baseline_hashes.push_back(shared.load());
            std::printf("  Baseline     hash = %llu\n",
                (unsigned long long)shared.load());
        }
    }

    // ===== Анализ: сколько уникальных хешей? =====
    auto count_unique = [](const std::vector<std::uint64_t>& v) {
        std::vector<std::uint64_t> sorted = v;
        std::sort(sorted.begin(), sorted.end());
        sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
        return sorted.size();
        };

    std::size_t u_motor_seq = count_unique(motor_seq_hashes);
    std::size_t u_motor_racy = count_unique(motor_racy_hashes);
    std::size_t u_baseline = count_unique(baseline_hashes);

    std::puts("");
    std::puts("=================================================================");
    std::puts("     TEST B: DETERMINISM (same input -> same output?)");
    std::puts("=================================================================");
    std::puts("");
    std::printf("Motor-seq    : %zu unique hashes out of %d runs\n",
        u_motor_seq, N_RUNS);
    std::printf("Motor-racy   : %zu unique hashes out of %d runs\n",
        u_motor_racy, N_RUNS);
    std::printf("Baseline     : %zu unique hashes out of %d runs\n",
        u_baseline, N_RUNS);
    std::puts("");

    if (u_motor_seq == 1) {
        std::puts("Motor-seq  : DETERMINISTIC (all runs identical).");
    }
    else {
        std::puts("Motor-seq  : NOT deterministic.");
    }
    if (u_motor_racy == 1) {
        std::puts("Motor-racy : DETERMINISTIC.");
    }
    else {
        std::puts("Motor-racy : NOT deterministic (race to commit).");
    }
    if (u_baseline == 1) {
        std::puts("Baseline   : DETERMINISTIC.");
    }
    else {
        std::puts("Baseline   : NOT deterministic (no barrier).");
    }

    // ===== CSV =====
    {
        std::ofstream f("test_b_results.csv");
        f << "run,motor_seq_hash,motor_racy_hash,baseline_hash\n";
        for (int i = 0; i < N_RUNS; ++i) {
            f << (i + 1) << ","
                << motor_seq_hashes[i] << ","
                << motor_racy_hashes[i] << ","
                << baseline_hashes[i] << "\n";
        }
        std::printf("\n  -> saved: test_b_results.csv\n");
    }

#ifdef _WIN32
    timeEndPeriod(1);
#endif
    return 0;
}