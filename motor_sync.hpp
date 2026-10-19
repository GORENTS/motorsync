// motor_sync.hpp
// =====================================================================
// MotorSync: synchronization library for frame-based simulations (C++17).
// Version 7.
//
// Modes:
//   - sequential_commit=false: barrier, commit in race (Test A2).
//   - sequential_commit=true:  barrier + commit strictly in order 0→1→...→N-1
//                              (guarantees determinism, Test B).
//
// Period = 0 disables sleep/spin (used by Test C for throughput).
// =====================================================================
#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace motor {

    /**
     * @class TickBarrier
     * @brief Barrier for N participants.
     *
     * The last arriving thread releases all others. Order of arrival
     * does not matter; all N participants must call wait() before any
     * can proceed.
     */
    class TickBarrier {
        std::mutex mtx_;
        std::condition_variable cv_;
        const std::size_t n_;
        std::size_t count_ = 0;
        std::size_t generation_ = 0;

    public:
        explicit TickBarrier(std::size_t n) : n_(n) {}

        void wait() {
            std::unique_lock<std::mutex> lk(mtx_);
            const std::size_t gen = generation_;
            if (++count_ == n_) {
                count_ = 0;
                ++generation_;
                cv_.notify_all();
                return;
            }
            cv_.wait(lk, [&] { return generation_ != gen; });
        }
    };

    using MeasurementFn = std::function<void(std::uint64_t tick, int id)>;

    /**
     * @class Motor
     * @brief Processor that builds frames with synchronized measurements.
     *
     * @guarantee All work-phases complete before any commit-phase begins
     *            (TickBarrier includes main thread, n_measurements + 1).
     * @guarantee With sequential_commit=true, commits happen strictly in
     *            order 0,1,...,N-1 (deterministic, verified by Test B).
     * @guarantee If target_period > 0, each frame duration is held with
     *            sleep_until + spin-wait (precision ~1 us on Windows with
     *            timeBeginPeriod(1); ~10 us on Linux/macOS without).
     * @guarantee If target_period == 0, no sleep/spin is performed: the
     *            Motor runs at maximum throughput (Test C).
     */
    class Motor {
        std::vector<MeasurementFn> works_;
        std::vector<MeasurementFn> commits_;
        std::vector<std::thread> threads_;
        TickBarrier barrier_;
        std::uint64_t max_ticks_;
        std::chrono::microseconds target_period_;
        bool sequential_commit_;

        // Sequential commit state
        std::mutex commit_mtx_;
        std::condition_variable commit_cv_;
        std::size_t next_committer_ = 0;
        std::size_t n_measurements_;

    public:
        Motor(std::size_t n_measurements,
            std::uint64_t max_ticks,
            std::chrono::microseconds target_period,
            bool sequential_commit = false)
            : barrier_(n_measurements + 1),
            max_ticks_(max_ticks),
            target_period_(target_period),
            sequential_commit_(sequential_commit),
            n_measurements_(n_measurements) {
        }

        void add(MeasurementFn work, MeasurementFn commit) {
            works_.push_back(std::move(work));
            commits_.push_back(std::move(commit));
        }

        void run() {
            for (std::size_t i = 0; i < works_.size(); ++i) {
                threads_.emplace_back([this, i] {
                    for (std::uint64_t t = 0; t < max_ticks_; ++t) {
                        works_[i](t, static_cast<int>(i));
                        barrier_.wait();

                        if (sequential_commit_) {
                            std::unique_lock<std::mutex> lk(commit_mtx_);
                            commit_cv_.wait(lk, [&] {
                                return next_committer_ == i;
                                });
                            commits_[i](t, static_cast<int>(i));
                            next_committer_ = (i + 1) % n_measurements_;
                            commit_cv_.notify_all();
                        }
                        else {
                            commits_[i](t, static_cast<int>(i));
                        }
                    }
                    });
            }

            auto next_target = std::chrono::steady_clock::now();
            const auto spin_window = std::chrono::microseconds(100);
            const bool use_period = target_period_.count() > 0;

            for (std::uint64_t t = 0; t < max_ticks_; ++t) {
                if (use_period) next_target += target_period_;
                barrier_.wait();

                if (use_period) {
                    auto sleep_target = next_target - spin_window;
                    if (sleep_target > std::chrono::steady_clock::now()) {
                        std::this_thread::sleep_until(sleep_target);
                    }
                    while (std::chrono::steady_clock::now() < next_target) {}
                }
            }

            for (auto& th : threads_) if (th.joinable()) th.join();
            threads_.clear();
        }
    };

} // namespace motor