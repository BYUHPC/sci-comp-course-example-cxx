#ifndef MOUNTAIN_RANGE_THREADED_H
#define MOUNTAIN_RANGE_THREADED_H
#include <cstring>
#include <charconv>
#include <vector>
#include <array>
#include <ranges>
#include <thread>
#include <semaphore>
#include <atomic>
#include <barrier>
#include "CoordinatedLoopingThreadpool.hpp"
#include "utils.hpp"
#include "MountainRangeSharedMem.hpp"



class MountainRangeThreaded: public MountainRangeSharedMem {
    // Members
    const size_type nthreads;
    CoordinatedLoopingThreadpool ds_workers, step_workers;
    std::atomic<value_type> ds_aggregator;
    std::barrier<> step_barrier;
    value_type iter_time_step;
public:
    inline static const std::string help_message =
            "Set the environment variable SOLVER_NUM_THREADS to a positive integer to set thread count (default 1).";



    MountainRangeThreaded(auto &&...args): MountainRangeSharedMem(args...), // https://tinyurl.com/byusc-parpack
            nthreads{[]{ // https://tinyurl.com/byusc-lambdai
                size_type nthreads = 1;
                auto nthreads_str = std::getenv("SOLVER_NUM_THREADS");
                if (nthreads_str != nullptr) std::from_chars(nthreads_str, nthreads_str+std::strlen(nthreads_str), nthreads);
                return nthreads;
            }()},
            ds_workers([this](auto tid){ // https://tinyurl.com/byusc-lambda
                auto [first, last] = mtn_utils::divided_cell_range(h.size(), tid, nthreads);
                value_type ds_local = 0;
                for (size_t i=first; i<last; i++) ds_local += ds_cell(i);
                ds_aggregator += ds_local;
            }, std::views::iota(0ul, nthreads)),
            step_workers([this](auto tid){ // https://tinyurl.com/byusc-lambda
                auto [first, last] = mtn_utils::divided_cell_range(h.size(), tid, nthreads);
                for (size_t i=first; i<last; i++) h[i] += iter_time_step * g[i];
                step_barrier.arrive_and_wait(); // h has to be completely updated before g update can start
                for (size_t i=first; i<last; i++) g[i] = g_cell(i);
            }, std::views::iota(0ul, nthreads)),
            step_barrier(nthreads) {
        step(0); // initialize g
    }



    value_type dsteepness() {
        ds_aggregator = 0;
        ds_workers.trigger_sync();
        return ds_aggregator / h.size();
    }



    value_type step(value_type time_step) {
        iter_time_step = time_step;
        step_workers.trigger_sync();
        t += time_step;
        return t;
    }
};



#endif
