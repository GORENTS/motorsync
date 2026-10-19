// test_c.cpp
// =====================================================================
// TEST C: throughput without period (PERIOD = 0).
//
// Question: what is the overhead of the synchronization barrier
// when the Motor runs at maximum speed (no sleep/spin)?
//
// Metric: frames per second (fps). Higher = better throughput.
// Expectation: barrier adds some overhead, but should be small
// compared to the actual work.
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

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <vector>

void light_work() {
    std::uint64_t x = 1;
    for (std::uint64_t i = 0; i < 100'000ULL; ++i) {
        x = x * 1664525ULL + 1013904223ULL;
    }
    (void)x;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    timeBeginPeriod(1);
#endif

    constexpr std::uint64_t N_TICKS = 100000;
    constexpr std::size_t   N_MEAS = 4;
    constexpr int           N_RUNS = 3;
    const auto PERIOD = std::chrono::microseconds(0);   // no period

    std::vector<double> motor_fps_runs, naive_fps_runs;

    for (int run = 0; run < N_RUNS; ++run) {
        std::printf("\n--- Run %d/%d ---\n", run + 1, N_RUNS);

        // ===== Motor =====
        {
            std::atomic<double> shared{ 0.0 };
            motor::Motor m(N_MEAS, N_TICKS, PERIOD, /*seq=*/false);
            for (std::size_t i = 0; i < N_MEAS; ++i) {
                m.add(
                    [](std::uint64_t, int) { light_work(); },
                    [&shared](std::uint64_t, int) { shared.fetch_add(1.0); }
                );
            }

            auto t0 = std::chrono::steady_clock::now();
            m.run();
            auto t1 = std::chrono::steady_clock::now();

            double sec = std::chrono::duration<double>(t1 - t0).count();
            double fps = N_TICKS / sec;
            motor_fps_runs.push_back(fps);
            std::printf("  Motor    : %.1f fps (%.2f s for %llu frames)\n",
                fps, sec, (unsigned long long)N_TICKS);
        }

        // ===== Baseline =====
        {
            std::atomic<double> shared{ 0.0 };
            baseline::NaiveMotor nb(N_TICKS, PERIOD);
            for (std::size_t i = 0; i < N_MEAS; ++i) {
                nb.add(
                    [](std::uint64_t, int) { light_work(); },
                    [&shared](std::uint64_t, int) { shared.fetch_add(1.0); }
                );
            }

            auto t0 = std::chrono::steady_clock::now();
            nb.run();
            auto t1 = std::chrono::steady_clock::now();

            double sec = std::chrono::duration<double>(t1 - t0).count();
            double fps = N_TICKS / sec;
            naive_fps_runs.push_back(fps);
            std::printf("  Baseline : %.1f fps (%.2f s for %llu frames)\n",
                fps, sec, (unsigned long long)N_TICKS);
        }
    }

    // Average
    auto avg = [](const std::vector<double>& v) {
        if (v.empty()) return 0.0;
        double s = 0;
        for (auto x : v) s += x;
        return s / v.size();
        };

    double m_fps = avg(motor_fps_runs);
    double n_fps = avg(naive_fps_runs);

    std::puts("");
    std::puts("=================================================================");
    std::puts("   TEST C: throughput without period (PERIOD = 0)");
    std::puts("=================================================================");
    std::printf("Motor    : %.1f fps (avg over %d runs)\n", m_fps, N_RUNS);
    std::printf("Baseline : %.1f fps (avg over %d runs)\n", n_fps, N_RUNS);
    std::puts("");

    double overhead = (n_fps - m_fps) / n_fps * 100.0;
    std::printf("Barrier overhead (Motor slower by): %.1f%%\n", overhead);
    if (overhead < 5.0) {
        std::puts("VERDICT: Barrier overhead is NEGLIGIBLE (< 5%).");
    }
    else if (overhead < 20.0) {
        std::puts("VERDICT: Barrier overhead is small (5-20%).");
    }
    else {
        std::puts("VERDICT: Barrier overhead is significant (> 20%).");
    }

    {
        std::ofstream f("test_c_results.csv");
        f << "version,fps_avg,n_runs\n";
        f << "Motor," << m_fps << "," << N_RUNS << "\n";
        f << "Baseline," << n_fps << "," << N_RUNS << "\n";
        std::printf("\n  -> saved: test_c_results.csv\n");
    }

#ifdef _WIN32
    timeEndPeriod(1);
#endif
    return 0;
}