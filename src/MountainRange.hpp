#ifndef MOUNTAIN_RANGE_H
#define MOUNTAIN_RANGE_H
#include <cstdint>
#include <vector>
#include <tuple>
#include <cmath>
#include <filesystem>
#include <fstream>
#include "binary_io.hpp"
#include "MountainRangeIOException.hpp"



/* MountainRange is the abstract base class from which all concrete mountain range classes inherit
 *
 * MEMBERS AND ACCESSORS
 * - dt: the time step, a static constant
 * - t: the current simulation time; access with sim_time()
 * - r: the uplift rate, a vector; access with uplift_rate()
 * - h: the current height, a vector; access with height()
 * - g: the current growth rate, a vector
 *
 * PUBLIC CONCRETE METHODS
 * - step(): step the simulation forward in time by dt and return the simulation time after the step
 * - solve(): run step() repeatedly untill dsteepness() drops below zero, returning the final simulation time
 * 
 * PUBLIC VIRTUAL METHODS
 * - step(time_step): step the simulation forward in time by time_step and return the simulation time after the step
 * - dsteepness(): return the derivative of the current steepness of the mountain range
 * - write(filename): write the mountain range to a file in binary, returning true on success or false on failure
 * 
 * C++ does not have virtual constructors so this can't be enforced, but subclasses are meant to implement a constructor
 * that takes a filename (const char *const) as its only argument and reads the binary mountain range from that file.
 * 
 * The last call of subclass constructors should be `step(0)` to initialize g.
 */
class MountainRange {
public:
    using value_type = double;
    using size_type = uint64_t;
protected:
    static constexpr const value_type dt = 0.01;



    // Members and accessors
    value_type t;                    // simulation time
    const std::vector<value_type> r; // uplift rate
    std::vector<value_type> h, g;    // height and growth rate

public:
    value_type  sim_time()    const { return t; }
    const auto &uplift_rate() const { return r; }
    const auto &height()      const { return h; }



    // Constructors
    // From an uplift rate, height, and time
    MountainRange(const decltype(r) &r, const decltype(h) &h, decltype(t) t): r(r), h(h), g(h.size()), t{t} {}

    // From an uplift rate; simulation time and height are initialized to zero
    MountainRange(const decltype(r) &r): MountainRange(r, decltype(h)(r.size()), 0) {}



    // Step the simulation forward in time until the steepness derivative drops below zero
    value_type solve() {
        while (dsteepness() >= 0) {
            step();
        }
        return t;
    }




protected:
    // Update functionality
    // enum to indicate whether the cell being operated on is the first, last, or a middle cell
    enum FirstMiddleLast { First, Middle, Last };

    // Update one cell of g, minding boundary conditions; static for use in MountainRangeGPU
    template <FirstMiddleLast FML=Middle>
    constexpr static void update_g_cell(const auto &r, const auto &h, auto &g, auto i) {
        auto L = -h[i];
        if constexpr (FML == First) L += (h[i  ] + h[i+1]) / 2;
        else if      (FML == Last ) L += (h[i-1] + h[i  ]) / 2;
        else                        L += (h[i-1] + h[i+1]) / 2;
        g[i] = r[i] - pow(h[i], 3) + L;
    }

    // Update a section of g, minding boundary conditions
    void update_g_section(auto first, auto last) {
        if (first == last) return;
        [this](auto &&...args){}(r, h, g, first);
        first == 0       ? update_g_cell<First>(r, h, g, first)  : update_g_cell<Middle>(r, h, g, first);
        #pragma omp parallel for
        for (auto i=first+1; i<last-1; i++) {
            update_g_cell<Middle>(r, h, g, i);
        }
        last == h.size() ? update_g_cell<Last >(r, h, g, last-1) : update_g_cell<Middle>(r, h, g, last-1);
    }

    // Update a section of h
    void update_h_section(auto first, auto last, auto time_step=dt) {
        #pragma omp parallel for
        for (auto i=first; i<last; i++) {
            h[i] += time_step * g[i];
        }
    }

public:
    // Step the mountain range forward in time, dt seconds by default
    virtual value_type step(decltype(t) time_step) = 0;
    value_type step() { return step(dt); }



protected:
    // Steepness derivative functionality
    // Get the steepness derivative of one cell, minding boundary conditions; static for use in MountainRangeGPU
    template <FirstMiddleLast FML=Middle>
    constexpr static value_type ds_cell(const auto &h, const auto &g, auto i) {
        auto left = i, right = i;
        if constexpr (FML != First) left  -= 1;
        if constexpr (FML != Last ) right += 1;
        auto ds = (h[right] - h[left]) * (g[right] - g[left]) / 2;
        //std::cout << std::setw(10) << ds << "," << g[i] << ":::";
        return ds;
    }

    // Return the sum of the derivative of steepness of each cell for a section of the mountain range
    value_type ds_section(auto first, auto last) const {
        if (first == last) return 0;
        auto ds = first == 0        ? ds_cell<First>(h, g, first)  : ds_cell<Middle>(h, g, first);
        #pragma omp parallel for reduction(+:ds)
        for (auto i=first+1; i<last-1; i++) {
            ds += ds_cell<Middle>(h, g, i);
        }
        ds     += last  == h.size() ? ds_cell<Last >(h, g, last-1) : ds_cell<Middle>(h, g, last-1);
        return ds;
    }

public:
    // Return dsteepness for the whole mountain range
    virtual value_type dsteepness() = 0;



    // I/O
    // Write the mountain range to a file in binary
    virtual bool write(const char *const filename) const = 0;

protected:
    // Throw a MountainRangeIOException if the input file's size isn't as expected given mountain range length n
    static void check_file_size(const char *const filename, size_t n=0) {
        auto header_size = sizeof(size_type) * 2 + sizeof(value_type);
        try {
            auto file_length = std::filesystem::file_size(std::filesystem::path(filename));
            if (file_length < header_size) {
                throw MountainRangeIOException(std::string(filename) + " is too short to contain a mountain range");
            }
            if (!n) return;
            auto expected_file_length = header_size + sizeof(value_type) * n * 2;
            if (file_length != expected_file_length) {
                throw MountainRangeIOException(std::string(filename) + " is not a valid mountain range data file");
            }
        } catch (const std::filesystem::filesystem_error &e) { // If a file descriptor is being used, for example
            return;
        }
    }



    // Split a mountain range of a given length among nproc processes, with first being the first cell that process
    // "rank" is in charge of and last being the last; rank ranges from 0 to nprocs-1
    constexpr static auto divided_cell_range(auto length, auto rank, auto nprocs) {
        auto ceil_div = [](auto num, auto den){ return num / den + (num % den != 0); };
        auto cells_per_proc = ceil_div(length, nprocs);
        auto first = std::min(cells_per_proc*rank, length);
        auto last = std::min(first+cells_per_proc, length);
        return std::array{first, last};
    }
};



#endif
