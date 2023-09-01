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



namespace {
    // Read in an environment variable as a size_t, returning default if key is of any format other than "^[0-9]+$"
    size_t getenv_as_size_t(auto key, size_t default_value=1) {
        auto val = std::getenv(key);
        if (val == nullptr) return default_value;
        size_t result;
        auto [ptr, error] = std::from_chars(val, val+std::strlen(val), result);
        return ptr == val+std::strlen(val) ? result : default_value;
    }
}



class MountainRangeThreaded: public MountainRangeSharedMem {
    // Members
    const size_t nthreads = getenv_as_size_t("SOLVER_NUM_THREADS");
    CoordinatedLoopingThreadpool ds_workers, step_workers;
    std::atomic<value_type> ds_aggregator;
    std::barrier<> step_barrier, ds_barrier;
    value_type iter_time_step;



    // Per-thread step and ds
    constexpr auto ds_this_thread(auto tid) {
        auto [first, last] = divided_cell_range(h.size(), tid, nthreads);
        ds_aggregator += ds_section(first, last);
        ds_barrier.arrive_and_wait();
    }

    constexpr void step_this_thread(auto tid) {
        auto [first, last] = divided_cell_range(h.size(), tid, nthreads);
        update_h_section(first, last, iter_time_step);
        step_barrier.arrive_and_wait();
        update_g_section(first, last);
    }



public:
    // Constructor
    MountainRangeThreaded(auto &&...args): MountainRangeSharedMem(args...),
                                           ds_workers([this](auto tid){
                                               auto [first, last] = divided_cell_range(h.size(), tid, nthreads);
                                               //std::cerr << first << ", " << last << std::endl;
                                               //std::cout << "Calculating ds" << std::endl;
                                               ds_barrier.arrive_and_wait();
                                               ds_aggregator += ds_section(first, last);
                                               ds_barrier.arrive_and_wait();
                                           }, std::views::iota(0ul, nthreads)),
                                           step_workers([this](auto tid){
                                               auto [first, last] = divided_cell_range(h.size(), tid, nthreads);
                                               //std::cout << "Updating h" << std::endl;
                                               step_barrier.arrive_and_wait();
                                               update_h_section(first, last, iter_time_step);
                                               step_barrier.arrive_and_wait();
                                               //std::cout << "Updating g" << std::endl;
                                               update_g_section(first, last);
                                               step_barrier.arrive_and_wait();
                                           }, std::views::iota(0ul, nthreads)),
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
