// baseline.hpp
// =====================================================================
// Naive implementation: no barrier, commit right after work.
// Version 5: supports zero period (Test C).
// =====================================================================
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>
#include <vector>

namespace baseline {

    using MeasurementFn = std::function<void(std::uint64_t tick, int id)>;

    class NaiveMotor {
        std::vector<std::thread> threads_;
        std::uint64_t max_ticks_;
        std::chrono::microseconds period_;
        std::vector<MeasurementFn> works_;
        std::vector<MeasurementFn> commits_;

    public:
        NaiveMotor(std::uint64_t max_ticks, std::chrono::microseconds period)
            : max_ticks_(max_ticks), period_(period) {
        }

        void add(MeasurementFn work, MeasurementFn commit) {
            works_.push_back(std::move(work));
            commits_.push_back(std::move(commit));
        }

        void run() {
            const bool use_period = period_.count() > 0;
            const auto spin_window = std::chrono::microseconds(100);

            for (std::size_t i = 0; i < works_.size(); ++i) {
                threads_.emplace_back([this, i, use_period, spin_window] {
                    auto next = std::chrono::steady_clock::now();

                    for (std::uint64_t t = 0; t < max_ticks_; ++t) {
                        if (use_period) next += period_;
                        works_[i](t, static_cast<int>(i));
                        commits_[i](t, static_cast<int>(i));

                        if (use_period) {
                            auto sleep_target = next - spin_window;
                            if (sleep_target > std::chrono::steady_clock::now()) {
                                std::this_thread::sleep_until(sleep_target);
                            }
                            while (std::chrono::steady_clock::now() < next) {}
                        }
                    }
                    });
            }
            for (auto& th : threads_) if (th.joinable()) th.join();
            threads_.clear();
        }
    };

} // namespace baseline