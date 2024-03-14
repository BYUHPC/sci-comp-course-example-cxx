#pragma once
#include <cstring>
#include <charconv>
#include <vector>
#include <array>
#include <ranges>
#include <thread>
#include <semaphore>
#include <atomic>
#include <barrier>
#include "MountainRange.hpp"



namespace {
    // Create a vector of threads, each of which will run F(thread_id) in a while loop until F() returns false
    auto looping_threadpool(auto thread_count, auto F) {
        std::vector<std::jthread> threads;
        threads.reserve(thread_count);
        for (size_t tid=0; tid<thread_count; tid++) {
            threads.emplace_back([F, tid]{
                while (F(tid));
            });
        }
        return threads;
    }
};



class MountainRangeThreaded: public MountainRange {
    // Members
    bool continue_iteration;
    const size_type nthreads;
    std::barrier<> ds_barrier, step_barrier;
    std::vector<std::jthread> ds_workers, step_workers;
    std::atomic<value_type> ds_aggregator;
    value_type iter_time_step;



    // Helper
    auto this_thread_cell_range(auto tid) {
        return mr::split_range(cells, tid, nthreads);
    }



public:
    inline static const std::string help_message =
            "Set the environment variable SOLVER_NUM_THREADS to a positive integer to set thread count (default 1).";

    // Run base constructor, then build threading infrastructure
    MountainRangeThreaded(auto &&...args): MountainRange(args...), // https://tinyurl.com/byusc-parpack
            continue_iteration{true},
            nthreads{[]{ // https://tinyurl.com/byusc-lambdai
                size_type nthreads = 1;
                auto nthreads_str = std::getenv("SOLVER_NUM_THREADS");
                if (nthreads_str != nullptr) std::from_chars(nthreads_str, nthreads_str+std::strlen(nthreads_str), nthreads);
                return nthreads;
            }()},
            ds_barrier(nthreads+1),   // worker threads plus main thread
            step_barrier(nthreads+1), // worker threads plus main thread
            ds_workers(looping_threadpool(nthreads, [this](auto tid){ // https://tinyurl.com/byusc-lambda
                ds_barrier.arrive_and_wait();
                if (!continue_iteration) return false;
                auto [first, last] = this_thread_cell_range(tid);
                value_type ds_local = 0;
                for (size_t i=first; i<last; i++) ds_local += ds_cell(i);
                ds_aggregator += ds_local;
                ds_barrier.arrive_and_wait();
                return true;
            })),
            step_workers(looping_threadpool(nthreads, [this](auto tid){ // https://tinyurl.com/byusc-lambda
                step_barrier.arrive_and_wait();
                if (!continue_iteration) return false;
                auto [first, last] = this_thread_cell_range(tid);
                for (size_t i=first; i<last; i++) update_h_cell(i, iter_time_step);
                step_barrier.arrive_and_wait(); // h has to be completely updated before g update can start
                for (size_t i=first; i<last; i++) update_g_cell(i);
                step_barrier.arrive_and_wait();
                return true;
            })) {
        step(0); // initialize g
    }

    // Destructor tells threads to exit
    ~MountainRangeThreaded() {
        continue_iteration = false;
        ds_barrier.arrive_and_wait();   // signal ds_workers to exit
        step_barrier.arrive_and_wait(); // signal step_workers to exit
    }

    // Steepness derivative
    value_type dsteepness() {
        ds_aggregator = 0;
        ds_barrier.arrive_and_wait(); // launch workers
        ds_barrier.arrive_and_wait(); // wait for workers to finish this iteration
        return ds_aggregator;
    }

    // Iterate from t to t+time_step in one step
    value_type step(value_type dt) {
        iter_time_step = dt;
        step_barrier.arrive_and_wait(); // signal workers to start updating h
        step_barrier.arrive_and_wait(); // signal workers to start updating g
        step_barrier.arrive_and_wait(); // wait for workers to finish this iteration
        t += dt;
        return t;
    }
};