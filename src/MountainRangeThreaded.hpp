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
        std::vector<std::jthread> threads;  // Creates an empty vector of threads.
        threads.reserve(thread_count);  // Creates thread_count threads for use later.  Sets the size of the thread vector.  Sets the capacity if the vector.
        for (size_t tid=0; tid<thread_count; tid++) {
            threads.emplace_back([F, tid]{ // pushes an element to the end of the vector.
                while (F(tid));
            });
        }
        // thread = std::jthread([function, int]{...})
        // jthread constructor is a lambda function.
        // struct is like a class but it just holds data (variables).
        return threads;
    }
};



class MountainRangeThreaded: public MountainRange {  // Inherits from MountainRange, making all public things in MountainRange public in MountainRangeThreaded
    // Threading-related members
    bool continue_iteration; // used to tell the looping threadpool to terminate at the end of the simulation
    const size_type nthreads;
    std::barrier ds_barrier, step_barrier; // creates two barrier objects.
    std::vector<std::jthread> ds_workers, step_workers;  // creates two vectors of threads
    std::atomic<value_type> ds_aggregator; // used to reduce dsteepness from each thread  // I'm not sure how this will be adapted to the wave function, I think dsteepness -> energy.
    value_type iter_dt; // Used to distribute dt to each thread



    // Determine which rows a certain thread is in charge of
    auto this_thread_cell_range(auto tid) {
        return mr::split_range(cells, tid, nthreads);  // chops up the matrix and divies it up to the various threads.
    }



public:
    // Help message to show that SOLVER_NUM_THREADS controls thread counts
    inline static const std::string help_message =
            "Set the environment variable SOLVER_NUM_THREADS to a positive integer to set thread count (default 1).";  // static means create only one variable to be shared with all instances.
// inline makes it so I don't have to go access this memory location each time, the complier just sticks it in wherever I call it (which is expensive unliess I only use it a couple times).



    // Run base constructor, then build threading infrastructure
    // This constructor inherits the constructors from MountainRange and adds the following initializations.
    MountainRangeThreaded(auto &&...args): MountainRange(args...), // https://tinyurl.com/byusc-parpack  // &&..args is a fancy way of saying what ever you pass to MountainRangeThreaded gets passed to MountainRange
            continue_iteration{true},
            nthreads{[]{ // https://tinyurl.com/byusc-lambdai
                size_type nthreads = 1;
                auto nthreads_str = std::getenv("SOLVER_NUM_THREADS");  // Get the environment variable
                if (nthreads_str != nullptr) std::from_chars(nthreads_str, nthreads_str+std::strlen(nthreads_str), nthreads);  // If the environment exists, set nthreads to the value fo the variable.
                return nthreads;
            }()},  // initialize the lambda function and then immediately call it.
            ds_barrier(nthreads+1),   // worker threads plus main thread  // initialize the barrier with 8 worker threads plus the main/parent thread.
            step_barrier(nthreads+1), // worker threads plus main thread
            ds_workers(looping_threadpool(nthreads, [this](auto tid){ // https://tinyurl.com/byusc-lambda
                ds_barrier.arrive_and_wait();  // All the threads got to arrive before continuing
                if (!continue_iteration) return false;  // terminating exit condition (see line 22)
                auto [first, last] = this_thread_cell_range(tid);  // splits the matrix up and divies them out.
                auto gfirst = tid==0 ? 1 : first;  // Gfirst = 1 tf tid==0 else Gfirst = first
                auto glast  = tid==nthreads-1 ? last-1 : last;  // glast = last-1 if tid==nthreads-1 else glast = last
                value_type ds_local = 0;  // aggregating/summation variable
                for (size_t i=gfirst; i<glast; i++) ds_local += ds_cell(i); 
                ds_aggregator += ds_local;  // atomic variable that adds all the results from all the threads.
                ds_barrier.arrive_and_wait();  // wait for all threads to arrive before doing anything continuing.
                return true;
            })),  // looping_threadpool takes in thread_count and a function.  The function is defined here as a lambda from lines 72-83
            step_workers(looping_threadpool(nthreads, [this](auto tid){ // https://tinyurl.com/byusc-lambda
                step_barrier.arrive_and_wait();
                if (!continue_iteration) return false;
                auto [first, last] = this_thread_cell_range(tid);
                auto gfirst = tid==0 ? 1 : first;
                auto glast  = tid==nthreads-1 ? last-1 : last;
                for (size_t i=first; i<last; i++) update_h_cell(i, iter_dt);
                step_barrier.arrive_and_wait(); // h has to be completely updated before g update can start
                for (size_t i=gfirst; i<glast; i++) update_g_cell(i);
                step_barrier.arrive_and_wait();
                return true;
            })) {
        // Initialize g
        step(0);
    }



    // Destructor just tells threads to exit
    ~MountainRangeThreaded() {
        continue_iteration = false;  // 
        ds_barrier.arrive_and_wait();   // signal ds_workers to exit
        step_barrier.arrive_and_wait(); // signal step_workers to exit
    }



    // Steepness derivative
    value_type dsteepness() override {
        // Reset reduction destination
        ds_aggregator = 0;
        
        // Launch workers
        ds_barrier.arrive_and_wait();

        // Wait for workers to finish this iteration
        ds_barrier.arrive_and_wait(); 

        return ds_aggregator;
    }

    // Iterate from t to t+dt in one step
    value_type step(value_type dt) override {
        // Let threads know what the time step this iteration is
        iter_dt = dt;

        // Signal workers to update h
        step_barrier.arrive_and_wait();

        // Signal workers to update g
        step_barrier.arrive_and_wait();

        // Wait for workers to finish this iteration
        step_barrier.arrive_and_wait();

        // Enforce boundary condition
        g[0] = g[1];
        g[cells-1] = g[cells-2];

        // Increment and return dt
        t += dt;
        return t;
    }
};

// A video of a walkthrough of the whole thing would be helpful.
