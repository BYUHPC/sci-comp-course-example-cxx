#ifndef MTN_UTILS_H
#define MTN_UTILS_H
#include <array>



// Miscellaneous utility functions
namespace mtn_utils {
    // Divide the range [0, length) into more-or-less evenly sized chunks [0, a), [a, b), ... [z, length) based on the
    // current thread or process's rank and global count (nprocs)
    auto divided_cell_range(auto length, auto rank, auto nprocs) {
        auto ceil_div = [](auto num, auto den){ return num / den + (num % den != 0); };
        auto cells_per_proc = ceil_div(length, nprocs);
        auto first = std::min(cells_per_proc*rank, length);
        auto last = std::min(first+cells_per_proc, length);
        return std::array{first, last};
    }

    // Get the cells to the left and right of i, given our boundary conditions and an array size
    constexpr auto neighbor_cells(auto i, auto n) {
        auto left  = i>0   ? i-1 : i;
        auto right = i<n-1 ? i+1 : i;
        return std::array{left, right};
    }
};



#endif
