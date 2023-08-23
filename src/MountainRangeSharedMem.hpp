#ifndef MOUNTAIN_RANGE_SHARED_MEM_H
#define MOUNTAIN_RANGE_SHARED_MEM_H
#include <vector>
#include <iostream>
#include <fstream>
#include <tuple>
#include <filesystem>
#include "MountainRange.hpp"
#include "MountainRangeIOException.hpp"
#include "binary_io.hpp"



/* MountainRangeSharedMem contains I/O functionality common to all shared memory mountain range types
 *
 * This constitutes a constructor and write function, both taking a filename as their sole argument.
 */
class MountainRangeSharedMem: public MountainRange {
    // Constructors
    // Helper that constructs from a tuple containing r, h, and t
    MountainRangeSharedMem(std::tuple<decltype(r), decltype(h), decltype(t)> r_h_t):
            MountainRange(std::move(std::get<0>(r_h_t)), std::move(std::get<1>(r_h_t)), std::get<2>(r_h_t)) {}

public:
    // Read a mountain range from a file
    MountainRangeSharedMem(const char *const filename): MountainRangeSharedMem([filename]{
        // Make sure file is at least big enough to contain a header
        check_file_size(filename);
        // Read header
        auto s = std::ifstream(filename);
        auto ndims = try_read_bytes<size_type>(s);
        if (ndims != 1) throw MountainRangeIOException("this solver only handles 1-dimensional mountain ranges");
        auto n = try_read_bytes<size_type>(s);
        auto t = try_read_bytes<value_type>(s);
        // Make sure file size matches the length read in
        check_file_size(filename, n);
        // Read body
        std::vector<value_type> r(n), h(n);
        try_read_bytes(s, r.data(), r.size());
        try_read_bytes(s, h.data(), h.size());
        // Return a tuple to be passed to the helper constructor
        return std::make_tuple(r, h, t);
    }()) {}

    // Inherit super constructor
    using MountainRange::MountainRange;



    // Write the mountain range to a file
    bool write(const char *const filename) const {
        std::ofstream f(filename);
        size_type ndims = 1;
        size_type n = h.size();
        return (try_write_bytes(f, &ndims, &n, &t) &&     // header
                try_write_bytes(f, r.data(), r.size()) && // uplift rate
                try_write_bytes(f, h.data(), h.size()));  // height
    }
};



#endif
