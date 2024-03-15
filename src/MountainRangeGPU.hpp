#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <execution>
#include "MountainRange.hpp"



// Helper function to get iterators to first and last indices of an array
// Use NVidia-specific code if it's available, otherwise use std::ranges
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
    // Delegate construction to MountainRange
    using MountainRange::MountainRange;



    // Steepness derivative
    value_type dsteepness() override {
        // Get iterators to first and last cells to be reduced
        auto [first, last] = index_range(h); // https://tinyurl.com/byusc-structbind

        // Sum ds_cell for each interior cell
        return std::transform_reduce(std::execution::par_unseq, first+1, last-1, value_type{0}, // initial value
                                     [](auto a, auto b){ return a + b; },                       // reduce
                                     [h=h.data(), g=g.data()](auto i){                          // transform
                                         return (h[i-1] - h[i+1]) * (g[i-1] - g[i+1]) / 2;
                                     }) / cells; // https://tinyurl.com/byusc-lambda
    }



    // Iterate from t to t+dt in one step
    value_type step(value_type dt) override {
        // Get iterators to first and last cells to be updated
        auto [first, last] = index_range(h); // https://tinyurl.com/byusc-structbind

        // Update h
        std::for_each(std::execution::par_unseq, first+1, last-1,
                      [h=h.data(), g=g.data(), dt](auto i){
                          h[i] += dt * g[i];
                      }); // https://tinyurl.com/byusc-lambda

        // Update g
        std::for_each(std::execution::par_unseq, first, last,
                      [n=cells, r=r.data(), h=h.data(), g=g.data()](auto i){
                          auto left  = i-1;
                          auto right = i+1;
                          if (i==0) {
                              left  = i;
                              right = i + 2;
                          }
                          if (i==n-1) {
                              left = i - 2;
                              right = i;
                          } 
                          auto L = (h[left] + h[right]) / 2 - h[i];
                          g[i] = r[i] - pow(h[i], 3) + L;
                      }); // https://tinyurl.com/byusc-lambda

        // Update and return simulation time
        t += dt;
        return t;
    }
};