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
#include "MountainRangeSharedMem.hpp"
#include "CoordinatedLoopingThreadpool.hpp"



class MountainRangeThreaded: public MountainRangeSharedMem {
    // Members
    const size_t nthreads = []{
        auto val = std::getenv("SOLVER_NUM_THREADS");
        if (val == nullptr) return 1ul;
        size_t nt;
        auto [ptr, error] = std::from_chars(val, val+std::strlen(val), nt);
        return ptr == val+std::strlen(val) ? nt : 1ul;
    }(); // https://tinyurl.com/byusc-lambdai
    CoordinatedLoopingThreadpool ds_workers, step_workers;
    std::atomic<value_type> ds_aggregator;
    std::barrier<> step_barrier, ds_barrier;
    value_type iter_time_step;
public:
    inline static const std::string help_message =
            "Set the environment variable SOLVER_NUM_THREADS to a positive integer to set thread count (default 1).";



private:
    // Per-thread step and ds
    constexpr auto ds_this_thread(auto tid) {
        auto [first, last] = divided_cell_range(h.size(), tid, nthreads); // https://tinyurl.com/byusc-structbind
        ds_aggregator += ds_section(first, last);
        ds_barrier.arrive_and_wait();
    }

    constexpr void step_this_thread(auto tid) {
        auto [first, last] = divided_cell_range(h.size(), tid, nthreads); // https://tinyurl.com/byusc-structbind
        update_h_section(first, last, iter_time_step);
        step_barrier.arrive_and_wait();
        update_g_section(first, last);
    }



public:
    // Constructor
    MountainRangeThreaded(auto &&...args): MountainRangeSharedMem(args...), // https://tinyurl.com/byusc-parpack
                                           ds_workers([this](auto tid){
                                               auto [first, last] = divided_cell_range(h.size(), tid, nthreads);
                                               ds_barrier.arrive_and_wait();
                                               ds_aggregator += ds_section(first, last);
                                               ds_barrier.arrive_and_wait();
                                           }, std::views::iota(0ul, nthreads)), // https://tinyurl.com/byusc-lambda
                                           step_workers([this](auto tid){
                                               auto [first, last] = divided_cell_range(h.size(), tid, nthreads);
                                               step_barrier.arrive_and_wait();
                                               update_h_section(first, last, iter_time_step);
                                               step_barrier.arrive_and_wait();
                                               update_g_section(first, last);
                                               step_barrier.arrive_and_wait();
                                           }, std::views::iota(0ul, nthreads)), // https://tinyurl.com/byusc-lambda
                                           step_barrier(nthreads), ds_barrier(nthreads)  {
        step(0);
    }



    // User-facing functions
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
