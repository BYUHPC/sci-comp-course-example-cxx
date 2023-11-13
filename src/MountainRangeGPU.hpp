#ifndef MOUNTAIN_RANGE_GPU_H
#define MOUNTAIN_RANGE_GPU_H
#include <cstdint>
#include <cmath>
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
        auto ds = std::transform_reduce(std::execution::par_unseq, first, last, value_type{0},
                                        [](auto a, auto b){ return a + b; },                   // reduce
                                        [n=h.size(), h=h.data(), g=g.data()](auto i){ // transform
                                            auto [left, right] = mtn_utils::neighbor_cells(i, n);
                                            return (h[right] - h[left]) * (g[right] - g[left]) / 2 / n;
                                            //return (h[i+1] - h[i-1]) * (g[i+1] - g[i-1]) / 2 / n;
                                        }); // https://tinyurl.com/byusc-lambda
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
        std::for_each(std::execution::par_unseq, first, last,
                      [r=r.data(), h=h.data(), g=g.data(), n=n](auto i){
                          auto [left, right] = mtn_utils::neighbor_cells(i, n);
                          auto L = (h[left] + h[right]) / 2 - h[i];
                          g[i] = r[i] - pow(h[i], 3) + L;
                      }); // https://tinyurl.com/byusc-lambda
        // Update simulation time
        t += time_step;
        return t;
    }
};



#endif
