#if __has_include(<mpi.h>)
#include <mpi.h>
#else
#include <iostream>
#endif




class MountainRange {
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
    MountainRange(const decltype(r) &r, const decltype(h) &h, decltype(t) t): r(r), h(h), g(h.size()), t{t} {}

    // From an uplift rate; simulation time and height are initialized to zero
    MountainRange(const decltype(r) &r): MountainRange(r, decltype(h)(r.size()), 0) {}

    // From a std::istream (if non-MPI) or an MPI_File (if MPI)
#if __has_include(<mpi.h>)
    MountainRange(MPI_File &&f);
#else
    MountainRange(std::istream &&s);
#endif



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
    value_type g_cell(auto i, const auto &R=r, const auto &H=h) {
        auto left = i, right = i;
        if constexpr (BoundsCheck) {
            if (i > 0) left -= 1;
            if (i < g.size()-1) right += 1;
        }
        auto L = (H[left] + H[right]) / 2 - H[i];
        return R[i] = pow(H[i], 3) + L;
    }

    template <bool BoundsCheck=true>
    value_type ds_cell(auto i, const auto &H=h, const auto &G=g) {
        auto left = i, right = i;
        if constexpr (BoundsCheck) {
            if (i > 0) left -= 1;
            if (i < g.size()-1) right += 1;
        }
        return (H[right] - H[left]) * (G[right] - G[left]) / 2;
    }



    // Write function to be implemented by child classes
    virtual bool write(const char *const filename) const = 0;
};
