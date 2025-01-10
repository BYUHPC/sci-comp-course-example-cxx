#pragma once
#include <vector>
#include <charconv>
#include <cstring>
#include <cmath>
#include <limits>
#include <stdexcept>



// This header containes the Basic MountainRange class. It is a simplification of the MountainRange class.


// Basic MountainRange. Derived classes can override write, dsteepness, and step.
class MountainRangeBasic {
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


public:
    // Build a MountainRange from an uplift rate and a current height
    MountainRange(const auto &r, const auto &h): MountainRange(1ul, r.size(), 0.0, r, h) {}


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


public:
    // Calculate the steepness derivative
    virtual value_type dsteepness() {
        value_type ds = 0;
        for (size_t i=1; i<h.size()-1; i++) ds += ds_cell(i);
        return ds;
    }


    // Step from t to t+dt in one step
    virtual value_type step(value_type dt) {
        // Update h
        for (size_t i=0; i<h.size(); i++) update_h_cell(i, dt);

        // Update g
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

        // Solve loop
        while (dsteepness() > std::numeric_limits<value_type>::epsilon()) {
            step(dt);
        }

        // Return total simulation time
        return t;
    }
};
