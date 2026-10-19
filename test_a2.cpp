// test_a2.cpp
// =====================================================================
// TEST A2: heterogeneous load (1 heavy ~800 us + 3 light ~50 us).
// Period 2000 us, 1000 frames × 10 runs.
//
// Metrics:
//   - commit gap: max(t_commit_i) - min(t_commit_i) per frame
//   - jitter of the main thread
//
// v2: LCG-based load (deterministic, not optimized away).
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
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <limits>
#include <numeric>
#include <vector>

// ---------------------------------------------------------------------
// LCG load: deterministic, CPU-bound, not optimized away.
// ---------------------------------------------------------------------
void heavy_work() {
    std::uint64_t x = 1;
    for (std::uint64_t i = 0; i < 4'000'000ULL; ++i) {
        x = x * 1664525ULL + 1013904223ULL;
    }
    (void)x;
}

void light_work() {
    std::uint64_t x = 1;
    for (std::uint64_t i = 0; i < 200'000ULL; ++i) {
        x = x * 1664525ULL + 1013904223ULL;
    }
    (void)x;
}

// ---------------------------------------------------------------------
// Commit gap statistics
// ---------------------------------------------------------------------
struct SyncStats {
    double mean_gap_us = 0;
    double p50_gap_us = 0;
    double p99_gap_us = 0;
    double max_gap_us = 0;
    std::size_t n = 0;
};

SyncStats compute_commit_gap(
    const std::vector<std::vector<std::int64_t>>& commit_ts)
{
    SyncStats s;
    if (commit_ts.empty() || commit_ts[0].empty()) return s;

    std::size_t n_ticks = commit_ts[0].size();
    for (auto& v : commit_ts) n_ticks = std::min(n_ticks, v.size());

    std::vector<double> gaps;
    gaps.reserve(n_ticks);

    for (std::size_t k = 0; k < n_ticks; ++k) {
        std::int64_t t_min = std::numeric_limits<std::int64_t>::max();
        std::int64_t t_max = std::numeric_limits<std::int64_t>::min();
        for (auto& v : commit_ts) {
            t_min = std::min(t_min, v[k]);
            t_max = std::max(t_max, v[k]);
        }
        gaps.push_back((t_max - t_min) / 1000.0);
    }

    s.n = gaps.size();
    s.mean_gap_us = std::accumulate(gaps.begin(), gaps.end(), 0.0) / gaps.size();
    std::sort(gaps.begin(), gaps.end());
    s.max_gap_us = gaps.back();
    s.p50_gap_us = gaps[static_cast<std::size_t>(gaps.size() * 0.50)];
    s.p99_gap_us = gaps[static_cast<std::size_t>(gaps.size() * 0.99)];
    return s;
}

void print_sync(const char* name, const SyncStats& s) {
    std::printf("%-10s mean_gap=%8.2f | p50_gap=%8.2f | "
        "p99_gap=%9.2f | max_gap=%10.2f | n=%zu\n",
        name, s.mean_gap_us, s.p50_gap_us,
        s.p99_gap_us, s.max_gap_us, s.n);
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
    const auto PERIOD = std::chrono::microseconds(2000);

    std::vector<SyncStats> motor_runs, naive_runs;

    for (int run = 0; run < N_RUNS; ++run) {
        std::printf("\n--- Run %d/%d ---\n", run + 1, N_RUNS);

        // ===== Motor =====
        {
            std::atomic<double> shared{ 0.0 };
            std::vector<std::vector<std::int64_t>> ts_motor(N_MEAS);
            for (auto& v : ts_motor) v.reserve(N_TICKS);

            motor::Motor m(N_MEAS, N_TICKS, PERIOD, /*seq=*/false);
            for (std::size_t i = 0; i < N_MEAS; ++i) {
                m.add(
                    [i](std::uint64_t, int) {
                        if (i == 0) heavy_work(); else light_work();
                    },
                    [&shared, &ts_motor, i](std::uint64_t, int) {
                        shared.fetch_add(1.0);
                        auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::steady_clock::now().time_since_epoch()).count();
                        ts_motor[i].push_back(now);
                    }
                );
            }
            m.run();
            auto s = compute_commit_gap(ts_motor);
            motor_runs.push_back(s);
            print_sync("Motor", s);
        }

        // ===== Baseline =====
        {
            std::atomic<double> shared{ 0.0 };
            std::vector<std::vector<std::int64_t>> ts_naive(N_MEAS);
            for (auto& v : ts_naive) v.reserve(N_TICKS);

            baseline::NaiveMotor nb(N_TICKS, PERIOD);
            for (std::size_t i = 0; i < N_MEAS; ++i) {
                nb.add(
                    [i](std::uint64_t, int) {
                        if (i == 0) heavy_work(); else light_work();
                    },
                    [&shared, &ts_naive, i](std::uint64_t, int) {
                        shared.fetch_add(1.0);
                        auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            std::chrono::steady_clock::now().time_since_epoch()).count();
                        ts_naive[i].push_back(now);
                    }
                );
            }
            nb.run();
            auto s = compute_commit_gap(ts_naive);
            naive_runs.push_back(s);
            print_sync("Baseline", s);
        }
    }

    // Aggregate
    auto avg = [](const std::vector<SyncStats>& runs) {
        SyncStats a;
        if (runs.empty()) return a;
        a.n = runs.size();
        for (auto& r : runs) {
            a.mean_gap_us += r.mean_gap_us;
            a.p50_gap_us += r.p50_gap_us;
            a.p99_gap_us += r.p99_gap_us;
            a.max_gap_us += r.max_gap_us;
        }
        double k = 1.0 / runs.size();
        a.mean_gap_us *= k; a.p50_gap_us *= k;
        a.p99_gap_us *= k; a.max_gap_us *= k;
        return a;
        };

    SyncStats mc = avg(motor_runs);
    SyncStats nc = avg(naive_runs);

    std::puts("");
    std::puts("=================================================================");
    std::puts("   TEST A2: heterogeneous load (avg over 10 runs)");
    std::puts("=================================================================");
    print_sync("Motor", mc);
    print_sync("Baseline", nc);
    std::puts("");

    double imp = (nc.mean_gap_us - mc.mean_gap_us) / nc.mean_gap_us * 100.0;
    std::printf("Commit atomicity improvement: %.1f%%\n", imp);
    if (imp > 50.0) std::puts("VERDICT: Motor is MUCH more atomic.");
    else if (imp > 10.0) std::puts("VERDICT: Motor is more atomic.");
    else std::puts("VERDICT: No significant improvement.");

    // CSV
    {
        std::ofstream f("test_a2_results.csv");
        f << "version,metric,mean,p50,p99,max,n\n";
        f << "Motor,commit_gap_us," << mc.mean_gap_us << ","
            << mc.p50_gap_us << "," << mc.p99_gap_us << ","
            << mc.max_gap_us << "," << mc.n << "\n";
        f << "Baseline,commit_gap_us," << nc.mean_gap_us << ","
            << nc.p50_gap_us << "," << nc.p99_gap_us << ","
            << nc.max_gap_us << "," << nc.n << "\n";
        std::printf("\n  -> saved: test_a2_results.csv\n");
    }

#ifdef _WIN32
    timeEndPeriod(1);
#endif
    return 0;
}