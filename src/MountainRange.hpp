#include <vector>
#include <charconv>
#include <cstring>
#include <cmath>
#include <format>
#include <iostream>
// Need to include mpi.h if an MPI compiler is being used
#ifdef MPI_VERSION
#include <mpi.h>
#endif



class MountainRange {
public:
    using value_type = double;
    using size_type = size_t;
protected:
    static constexpr const value_type dt = 0.01;



    // Members and accessors
    const size_type N, n;            // number of dimensions and size
    value_type t;                    // simulation time
    const std::vector<value_type> r; // uplift rate
    std::vector<value_type> h, g;    // height and growth rate

public:
    size_t     size()         const { return n; }

    value_type sim_time()     const { return t; }

    auto       &uplift_rate() const { return r; }

    auto       &height()      const { return h; }



    // Constructors
    // From an uplift rate, height, and time
    MountainRange(const decltype(r) &r, const decltype(h) &h, decltype(t) t): N{1}, n{r.size()}, r(r), h(h), g(h.size()), t{t} {}

    // From an uplift rate; simulation time and height are initialized to zero
    MountainRange(const decltype(r) &r): MountainRange(r, decltype(h)(r.size()), 0) {}

    // "Virtual" I/O constructors: from a std::istream (if non-MPI) or an MPI_File (if MPI)
#ifdef MPI_VERSION
    MountainRange(MPI_File &&f);
#else
    MountainRange(std::istream &&s);
#endif



    // Write function to be implemented by child classes
    virtual bool write(const char *const filename) const = 0;



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

    virtual value_type dsteepness() = 0; // can't be const because threads



    // Helpers for step and dsteepness
    template <bool BoundsCheck=true>
    constexpr static value_type g_cell(const auto r, const auto h, auto size, auto i) {
        auto left = i-1, right = i+1;
        if constexpr (BoundsCheck) {
            left =  i == 0      ? i : left;
            right = i == size-1 ? i : right;
        }
        auto L = (h[left] + h[right]) / 2 - h[i];
        return r[i] - pow(h[i], 3) + L;
    }

    template <bool BoundsCheck=true>
    constexpr static value_type ds_cell(const auto h, const auto g, auto size, auto i) {
        auto left = i-1, right = i+1;
        if constexpr (BoundsCheck) {
            left =  i == 0      ? i : left;
            right = i == size-1 ? i : right;
        }
        return (h[right] - h[left]) * (g[right] - g[left]) / 2;
    }

    template <bool BoundsCheck=true>
    constexpr value_type g_cell(auto i) const { return g_cell<BoundsCheck>(r, h, h.size(), i); }

    template <bool BoundsCheck=true>
    constexpr value_type ds_cell(auto i) const { return ds_cell(h, g, h.size(), i); }
};
