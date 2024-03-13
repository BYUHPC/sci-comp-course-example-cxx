#include <vector>
#include <fstream>
#include <filesystem>
#include <charconv>
#include <format>
#include <cstring>
#include <cmath>
#include "binary_io.hpp"



namespace mr {
    // Divide [0, n) evenly among size processes, returning the range appropriate for rank.
    auto split_range(auto n, auto rank, auto size) {
        auto n_per_proc = (n - 2) / size;
        auto extra      = (n - 2) % size;
        auto first = n_per_proc * rank + std::min(rank, extra) + 1;
        auto last = first + n_per_proc;
        if (rank < extra) {
            last += 1;
        }
        if (rank == size - 1) {
            last = n - 1;
        }
        return std::array{first, last};
    }
}



class MountainRange {
public:
    using size_type  = size_t;
    using value_type = double;



protected:
    static constexpr const value_type default_dt = 0.01;
    static constexpr const size_t header_size = sizeof(size_type) * 2 + sizeof(value_type);

    const size_type ndims, cells;
    value_type t;
    std::vector<value_type> r, h, g;



    static void handle_wrong_dimensions() {
        throw std::logic_error("Input file is corrupt or multi-dimensional, which this implementation doesn't support");
    }
    static void handle_wrong_file_size() {
        throw std::logic_error("Input file appears to be corrupt");
    }
    static void handle_write_failure(const char *const filename) {
        throw std::logic_error("Failed to write to " + std::string(filename));
    }
    static void handle_read_failure(const char *const filename) {
        throw std::logic_error("Failed to read from " + std::string(filename));
    }



// Accessors
public:
    auto size()         const { return cells; }
    auto sim_time()     const { return t; }
    auto &uplift_rate() const { return r; }
    auto &height()      const { return h; }




protected:
    // The constructor that all other constructors call
    MountainRange(auto ndims, auto cells, auto t, const auto &r, const auto &h): ndims{ndims}, cells{cells}, t{t},
                                                                                 r(r), h(h), g(h) {
        if (ndims != 1) handle_wrong_dimensions();
        step(0); // initialize g
    }



    MountainRange(std::istream &&s): ndims{try_read_bytes<decltype(ndims)>(s)},
                                     cells{try_read_bytes<decltype(cells)>(s)},
                                     t{    try_read_bytes<decltype(t    )>(s)},
                                     r(cells),
                                     h(cells),
                                     g(cells) {
        if (ndims != 1) handle_wrong_dimensions();
        try_read_bytes(s, r.data(), r.size());
        try_read_bytes(s, h.data(), h.size());
        step(0);
    }

public:
    MountainRange(const auto &r, const auto &h): MountainRange(1ul, r.size(), 0.0, r, h) {}

    MountainRange(const char *filename) try: MountainRange(std::ifstream(filename)) {
                                        } catch (const std::ios_base::failure &e) {
                                            handle_read_failure(filename);
                                        } catch (const std::filesystem::filesystem_error &e) {
                                            handle_read_failure(filename);
                                        }

    virtual void write(const char *filename) {
        auto f = std::ofstream(filename);
        try {
            try_write_bytes(f, &ndims, &cells, &t);
            try_write_bytes(f, r.data(), r.size());
            try_write_bytes(f, h.data(), h.size());
        } catch (const std::filesystem::filesystem_error &e) {
            handle_write_failure(filename);
        } catch (const std::ios_base::failure &e) {
            handle_write_failure(filename);
        }
    }






    virtual value_type dsteepness() {
        value_type ds = 0;
        #pragma omp parallel for reduction(+:ds)
        for (size_t i=0; i<h.size(); i++) ds += ds_cell(i);
        return ds;
    }



    virtual value_type step(value_type dt) {
        // Update h
        #pragma omp parallel for
        for (size_t i=0; i<h.size(); i++) update_h_cell(i, dt);

        // Update g
        #pragma omp parallel for
        for (size_t i=0; i<g.size(); i++) update_g_cell(i);

        // Increment time step
        t += dt;
        return t;
    }

    auto step() {
        return step(default_dt);
    }



    value_type solve(value_type dt=default_dt) {
        // Read checkpoint interval from environment
        value_type checkpoint_interval = 0;
        auto INTVL = std::getenv("INTVL");
        if (INTVL != nullptr) std::from_chars(INTVL, INTVL+std::strlen(INTVL), checkpoint_interval);
        // Solve loop
        while (dsteepness() >= 0) {
            step();
            // Checkpoint if requested
            if (checkpoint_interval > 0 && fmod(t+dt/5, checkpoint_interval) < 2*dt/5) {
                auto check_file_name = std::format("chk-{:07.2f}.wo", t).c_str();
                write(check_file_name);
            }
        }
        return t;
    }



protected:
    // Helpers for step and dsteepness
    constexpr void update_g_cell(auto i) {
        auto L = (h[i-1] + h[i+1]) / 2 - h[i];
        g[i] = r[i] - pow(h[i], 3) + L;
    }

    constexpr void update_h_cell(auto i, auto time_step) {
        h[i] += g[i] * time_step;
    }

    constexpr value_type ds_cell(auto i) const {
        return (h[i-1] - h[i+1]) * (g[i-1] - g[i+1]) / 2 / cells;
    }
};