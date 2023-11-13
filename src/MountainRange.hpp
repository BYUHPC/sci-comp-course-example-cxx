#ifndef MOUNTAIN_RANGE_H
#define MOUNTAIN_RANGE_H
#include <vector>
#include <charconv>
#include <cstring>
#include <cmath>
#include <format>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "binary_io.hpp"
#include "utils.hpp"
// Need to include MPL if an MPI compiler is being used
#ifdef MPI_VERSION
#include <mpl/mpl.hpp>
#endif



class MountainRange {
public:
    using value_type = double;
    using size_type = size_t;
protected:
    static constexpr const value_type dt = 0.01;
    static constexpr const size_t header_size = sizeof(size_type) * 2 + sizeof(value_type);



    // Members and accessors
    const size_type N, n;               // size
    value_type t;                    // simulation time
    std::vector<value_type> r, h, g; // height and growth rate
public:
    auto size()         const { return n; }
    auto sim_time()     const { return t; }
    auto &uplift_rate() const { return r; }
    auto &height()      const { return h; }



    // Constructors
    // From an uplift rate, height, and time
    MountainRange(const decltype(r) &r, const decltype(h) &h, decltype(t) t): N{1}, n{r.size()}, t{t},
                                                                              r(r), h(h), g(h.size()) {}

    // From an uplift rate; simulation time and height are initialized to zero
    MountainRange(const decltype(r) &r): MountainRange(r, decltype(h)(r.size()), 0) {}



    // Handle problems reading and writing
private:
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



    // MPI I/O section: if an MPI compiler is being used, include the following
    // See the note at the top of MountainRangeMPI.hpp for details
#ifdef MPI_VERSION
protected:
    // Figure out which cells this process in in charge of
    template <bool IncludeHalos=false>
    auto this_process_cell_range() const {
        auto [first, last] = mtn_utils::divided_cell_range(n, mpl::environment::comm_world().rank(),
                                                              mpl::environment::comm_world().size());
                                                              // https://tinyurl.com/byusc-structbind
        if constexpr (IncludeHalos) {
            first = first == 0 ? first : first-1;
            last  = last  == n ? last  : last+1;
        }
        return std::array{first, last};
    }

private:
    // Figure out how big this process should make its arrays
    auto this_process_cell_count() const {
        auto [first, last] = this_process_cell_range<true>(); // includes halos; https://tinyurl.com/byusc-structbind
        return last - first;
    }

    // Read from an mpl::file
    MountainRange(mpl::file &&f): N{[&f]{ // https://tinyurl.com/byusc-lambdai
                                      size_type ret;
                                      f.read_all(ret);
                                      if (ret != 1) handle_wrong_dimensions();
                                      return ret;
                                  }()},
                                  n{[&f]{ // https://tinyurl.com/byusc-lambdai
                                      size_type ret;
                                      f.read_all(ret);
                                      auto expected_file_size = header_size + ret * sizeof(value_type) * 2;
                                      if (expected_file_size != f.size()) handle_wrong_file_size();
                                      return ret;
                                  }()},
                                  t{[&f]{ // https://tinyurl.com/byusc-lambdai
                                      value_type ret;
                                      f.read_all(ret);
                                      return ret;
                                  }()},
                                  r(this_process_cell_count()),
                                  h(this_process_cell_count()),
                                  g(this_process_cell_count()) {
        // Read in this process's portion of r and h
        auto [first, last] = this_process_cell_range<true>(); // include halos; https://tinyurl.com/byusc-structbind
        auto layout = mpl::vector_layout<value_type>(r.size());
        auto r_offset = header_size + sizeof(value_type) * first;
        auto h_offset = r_offset + sizeof(value_type) * n;
        f.read_at(r_offset, r.data(), layout);
        f.read_at(h_offset, h.data(), layout);
    }

public:
    // Constructor taking a file name
    MountainRange(const char *const filename) try: MountainRange(mpl::file(mpl::environment::comm_world(), filename,
                                                                           mpl::file::access_mode::read_only)) {}
                                              catch (const mpl::io_failure &e) {
                                                   handle_read_failure(filename);
                                              }

    // Write to a file with MPI I/O
    void write(const char *const filename) const try {
        auto f = mpl::file(mpl::environment::comm_world(), filename,
                           mpl::file::access_mode::create | mpl::file::access_mode::write_only);
        // Write header
        f.write_all(N);
        f.write_all(n);
        f.write_all(t);
        // Write this process's portion of r and h
        auto [first, last] = this_process_cell_range<false>(); // without halos; https://tinyurl.com/byusc-structbind
        auto layout = mpl::vector_layout<value_type>(last-first);
        auto r_offset = header_size + sizeof(value_type) * first;
        auto h_offset = r_offset + sizeof(value_type) * n;
        auto halo_offset = mpl::environment::comm_world().rank() == 0 ? 0 : 1;
        f.write_at(r_offset, r.data()+halo_offset, layout);
        f.write_at(h_offset, h.data()+halo_offset, layout);
    } catch (const mpl::io_failure &e) {
        handle_write_failure(filename);
    }



    // "Normal" I/O section: if a non-MPI compiler is being used, include the following
#else
private:
    // Read in and return a vector of size count
    template <class T>
    static auto try_read_vector(std::istream &s, auto count) {
        auto v = std::vector<T>(count);
        try_read_bytes(s, v.data(), v.size());
        return v;
    }

    // Read from a std::istream
    MountainRange(std::istream &&s, size_t file_size=0): N{[&s, this]{ // https://tinyurl.com/byusc-lambdai
                                                             auto ret = try_read_bytes<size_type>(s);
                                                             if (ret != 1) handle_wrong_dimensions();
                                                             return ret;
                                                         }()},
                                                         n{[&s, file_size, this]{ // https://tinyurl.com/byusc-lambdai
                                                             auto ret = try_read_bytes<size_type>(s);
                                                             auto expected_size = header_size+sizeof(value_type)*ret*2;
                                                             if (expected_size != file_size) handle_wrong_file_size();
                                                             return ret;
                                                         }()},
                                                         t{try_read_bytes<decltype(t)>(s)},
                                                         r(try_read_vector<value_type>(s, n)),
                                                         h(try_read_vector<value_type>(s, n)),
                                                         g(h.size()) {}

public:
    // Constructor taking a filename
    MountainRange(const char *const filename) try: MountainRange(std::ifstream(filename),
                                                                 std::filesystem::file_size(filename)) {
                                              } catch (const std::ios_base::failure &e) {
                                                  handle_read_failure(filename);
                                              } catch (const std::filesystem::filesystem_error &e) {
                                                  handle_read_failure(filename);
                                              }

    // Write to a file with "normal" I/O
    void write(const char *const filename) const try {
        auto s = std::ofstream(filename);
        // Write header
        try_write_bytes(s, &N, &n, &t);
        // Write body
        try_write_bytes(s, r.data(), r.size());
        try_write_bytes(s, h.data(), h.size());
    } catch (const std::ios_base::failure &e) {
        handle_write_failure(filename);
    }
#endif // end of MPI vs non-MPI I/O section



    // Step the simulation forward in time until the steepness derivative drops below zero
    value_type solve() {
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



    // Step and dsteepness are to be implemented by child classes
    virtual value_type step(decltype(t) time_step) = 0;

    value_type step() { return step(dt); }

    virtual value_type dsteepness() = 0; // can't be const due to our threaded code implementation



    // Helpers for step and dsteepness
    constexpr void update_g_cell(auto i) {
        auto left  = i>0          ? i-1 : i;
        auto right = i<g.size()-1 ? i+1 : i;
        auto L = (h[left] + h[right]) / 2 - h[i];
        g[i] = r[i] - pow(h[i], 3) + L;
    }

    constexpr void update_h_cell(auto i) {
        h[i] += g[i] * dt;
    }

    constexpr auto ds_cell(auto i) {
        auto left  = i>0          ? i-1 : i;
        auto right = i<g.size()-1 ? i+1 : i;
        return (h[right] - h[left]) * (g[right] - g[left]) / 2 / n;
    }
};



#endif
