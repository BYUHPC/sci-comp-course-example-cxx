#ifndef MOUNTAIN_RANGE_OPEN_MP_H
#define MOUNTAIN_RANGE_OPEN_MP_H
#include "MountainRangeSharedMem.hpp"



class MountainRangeOpenMP: public MountainRangeSharedMem {
public:
    MountainRangeOpenMP(auto &&...args): MountainRangeSharedMem(args...) { // https://tinyurl.com/byusc-parpack
        step(0); // initialize g
    }

    value_type dsteepness() {
        auto ds = ds_section(0, h.size());
        return ds / h.size();
    }

    value_type step(value_type time_step) {
        update_h_section(0, h.size(), time_step);
        update_g_section(0, g.size());
        t += time_step;
        return t;
    }
};



#endif
