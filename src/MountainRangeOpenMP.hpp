#ifndef MOUNTAIN_RANGE_OPENMP_H
#define MOUNTAIN_RANGE_OPENMP_H
#include "MountainRange.hpp"



class MountainRangeOpenMP: public MountainRange {
public:
    // Delegate construction to MountainRange
    MountainRangeOpenMP(auto && ...args): MountainRange(args...) { // https://tinyurl.com/byusc-parpack
        step(0); // initialize g
    }

    // Steepness derivative
    value_type dsteepness() {
        value_type ds = 0;
        #pragma omp parallel for reduction(+:ds)
        for (size_t i=0; i<h.size(); i++) ds += ds_cell(i);
        return ds;
    }

    // Iterate from t to t+time_step in one step
    value_type step(value_type time_step) {
        // Update h
        #pragma omp parallel for
        for (size_t i=0; i<h.size(); i++) update_h_cell(i, time_step);
        // Update g
        #pragma omp parallel for
        for (size_t i=0; i<g.size(); i++) update_g_cell(i);
        // Increment time step
        t += time_step;
        return t;
    }
};



#endif
