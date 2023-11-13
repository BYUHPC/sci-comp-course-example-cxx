#ifndef MOUNTAIN_RANGE_GPU_H
#define MOUNTAIN_RANGE_GPU_H
#include <cstdint>
#include <algorithm>
#include <numeric>
#include <execution>
#include "MountainRange.hpp"



// Helper function to get iterators to first and last indices of an array
#if __has_include(<thrust/iterator/counting_iterator.h>)
#include <thrust/iterator/counting_iterator.h>
namespace {
    auto index_range(const auto &x) {
        return std::make_tuple(thrust::counting_iterator(0ul), thrust::counting_iterator(x.size()));
    }
};
#else
#include <ranges>
namespace {
    auto index_range(const auto &x) {
        auto interior = std::ranges::views::iota(0ul, x.size());
        return std::make_tuple(interior.begin(), interior.end());
    }
};
#endif



class MountainRangeGPU: public MountainRange {
public:
    // Delegate constructor to MountainRange
    MountainRangeGPU(auto && ...args): MountainRange(args...) { // https://tinyurl.com/byusc-parpack
        step(0); // initialize g
    }

    // Steepness derivative
    value_type dsteepness() {
        auto [first, last] = index_range(h); // https://tinyurl.com/byusc-structbind
        auto ds = ds_cell(0);
        auto ds_bounds_check = [n=h.size()](auto h, auto g, auto i){
            return ds_cell<false>(h, g, n, i);
        };
        ds += std::transform_reduce(std::execution::par_unseq, first+1, last-1, value_type{0},
                                    [](auto a, auto b){ return a + b; },                   // reduce
                                    [n=h.size(), h=h.data(), g=g.data()](auto i){ // transform
                                        return (h[i+1] - h[i-1]) * (g[i+1] - g[i-1]) / 2 / n;
                                        //return ds_bounds_check(h, g, i);
                                        //return ds_cell<false>(h, g, size, i); // false turns off bounds checking
                                    }); // https://tinyurl.com/byusc-lambda
        ds += ds_cell(h.size()-1);
        return ds;
    }

    // Iterate from t to t+time_step in one step
    value_type step(value_type time_step) {
        // Update h
        auto [first, last] = index_range(h); // https://tinyurl.com/byusc-structbind
        std::for_each(std::execution::par_unseq, first, last,
                      [h=h.data(), g=g.data(), time_step](auto i){
                          h[i] += time_step * g[i];
                      }); // https://tinyurl.com/byusc-lambda
        // Update g
        g[0] = g_cell(0);
        std::for_each(std::execution::par_unseq, first+1, last-1,
                      [r=r.data(), h=h.data(), g=g.data(), size=h.size()](auto i){
                          g[i] = g_cell<false>(r, h, size, i); // false turns off bounds checking
                      }); // https://tinyurl.com/byusc-lambda
        g[g.size()-1] = g_cell(g.size()-1);
        // Update simulation time
        t += time_step;
        return t;
    }
};



#endif
