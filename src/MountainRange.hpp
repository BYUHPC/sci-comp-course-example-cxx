#pragma once
#include <vector>
#include <fstream>
#include <filesystem>
#include <charconv>
#include <format>
#include <cstring>
#include <cmath>
#include <limits>
#include "binary_io.hpp"



// This header containes the base MountainRange class, which can be compiled serial or OpenMP threaded



// Namespace for split_range, which is used by both std::jthread and MPI implementations
namespace mr {
    // Divide [0, n) evenly among size processes, returning the range appropriate for rank [0, size).
    // Example: divide 100 cells among 3 threads, ignoring the first and last cells since they aren't updated:
    //   - split_range(100, 0, 3) -> [0, 34]
    //   - split_range(100, 1, 3) -> [34, 67]
    //   - split_range(100, 2, 3) -> [67, 100]
    auto split_range(auto n, auto rank, auto size) {
        auto n_per_proc = n / size;
        decltype(rank) extra = n % size;
        auto first = n_per_proc * rank + std::min(rank, extra);
        auto last = first + n_per_proc;
        if (rank < extra) {
            last += 1;
        }
        return std::array{first, last};
    }
}



// Base MountainRange. Derived classes can override write, dsteepness, and step.
class MountainRange {

public:
    using size_type  = size_t;
    using value_type = double;



protected:
    // Parameters and members
    static constexpr const value_type default_dt = 0.01;
    static constexpr const size_t header_size = sizeof(size_type) * 2 + sizeof(value_type);
    const size_type ndims, cells;
    value_type t;
    std::vector<value_type> r, h, g;



public:
    // Accessors
    auto size()         const { return cells; }
    auto sim_time()     const { return t; }
    auto &uplift_rate() const { return r; }
    auto &height()      const { return h; }



protected:
    // Basic constructor
    MountainRange(auto ndims, auto cells, auto t, const auto &r, const auto &h): ndims{ndims}, cells{cells}, t{t},
                                                                                 r(r), h(h), g(h) {
        if (ndims != 1) handle_wrong_dimensions();
        step(0); // initialize g
    }



    // Error handlers for I/O constructors
    static void handle_wrong_dimensions() {
        throw std::logic_error("Input file is corrupt or multi-dimensional, which this implementation doesn't support");
    }

    static void handle_write_failure(const char *const filename) {
        throw std::logic_error("Failed to write to " + std::string(filename));
    }

    static void handle_read_failure(const char *const filename) {
        throw std::logic_error("Failed to read from " + std::string(filename));
    }



    // Read in a MountainRange from a stream
    MountainRange(std::istream &&s): ndims{try_read_bytes<decltype(ndims)>(s)},
                                     cells{try_read_bytes<decltype(cells)>(s)},
                                     t{    try_read_bytes<decltype(t    )>(s)},
                                     r(cells),
                                     h(cells),
                                     g(cells) {
        // Handle nonsense
        if (ndims != 1) handle_wrong_dimensions();

        // Read in r and h
        try_read_bytes(s, r.data(), r.size());
        try_read_bytes(s, h.data(), h.size());

        // Initialize g
        step(0);
    }



public:
    // Build a MountainRange from an uplift rate and a current height
    MountainRange(const auto &r, const auto &h): MountainRange(1ul, r.size(), 0.0, r, h) {}



    // Read a MountainRange from a file, handling read errors gracefully
    MountainRange(const char *filename) try: MountainRange(std::ifstream(filename)) {
                                        } catch (const std::ios_base::failure &e) {
                                            handle_read_failure(filename);
                                        } catch (const std::filesystem::filesystem_error &e) {
                                            handle_read_failure(filename);
                                        }



    // Write a MountainRange to a file, handling write errors gracefully
    virtual void write(const char *filename) const {
        // Open the file
        auto f = std::ofstream(filename);

        try {
            // Write the header
            try_write_bytes(f, &ndims, &cells, &t);

            // Write the body
            try_write_bytes(f, r.data(), r.size());
            try_write_bytes(f, h.data(), h.size());

        // Handle write failures
        } catch (const std::filesystem::filesystem_error &e) {
            handle_write_failure(filename);
        } catch (const std::ios_base::failure &e) {
            handle_write_failure(filename);
        }
    }



protected:
    // Helpers for step and dsteepness
    constexpr void update_g_cell(auto i) {
        auto L = (h[i-1] + h[i+1]) / 2 - h[i];
        g[i] = r[i] - pow(h[i], 3) + L;
    }

    constexpr void update_h_cell(auto i, auto dt) {
        h[i] += g[i] * dt;
    }

    constexpr value_type ds_cell(auto i) const {
        return ((h[i-1] - h[i+1]) * (g[i-1] - g[i+1])) / 2 / (cells - 2);
    }

private:
    // Read checkpoint interval from environment
    value_type get_checkpoint_interval() const {
        value_type checkpoint_interval = 0;
        auto INTVL = std::getenv("INTVL");
        if (INTVL != nullptr) std::from_chars(INTVL, INTVL+std::strlen(INTVL), checkpoint_interval);
        return checkpoint_interval;
    }

    // Determine if a checkpoint should occur on this iteration
    constexpr bool should_perform_checkpoint(auto checkpoint_interval, auto dt) const {
        return checkpoint_interval > 0 && fmod(t+dt/5, checkpoint_interval) < 2*dt/5;
    }


public:
    // Calculate the steepness derivative
    virtual value_type dsteepness() const {
        value_type ds = 0;
        #pragma omp parallel for reduction(+:ds)
        for (size_t i=1; i<h.size()-1; i++) ds += ds_cell(i);
        return ds;
    }



    // Step from t to t+dt in one step
    virtual value_type step(value_type dt) {
        // Update h
        #pragma omp parallel for
        for (size_t i=0; i<h.size(); i++) update_h_cell(i, dt);

        // Update g
        #pragma omp parallel for
        for (size_t i=1; i<g.size()-1; i++) update_g_cell(i);

        // Enforce boundary condition
        g[0] = g[1];
        g[g.size()-1] = g[g.size()-2];

        // Increment time step
        t += dt;
        return t;
    }



    // Step until dsteepness() falls below 0, checkpointing along the way
    value_type solve(value_type dt=default_dt) {
        auto checkpoint_interval = get_checkpoint_interval();

        // Solve loop
        while (dsteepness() > std::numeric_limits<value_type>::epsilon()) {
            step(dt);

            // Checkpoint if requested
            if (should_perform_checkpoint(checkpoint_interval, dt)) {
                auto check_file_name = std::format("chk-{:07.2f}.wo", t).c_str();
                write(check_file_name);
            }
        }

        // Return total simulation time
        return t;
    }
};
