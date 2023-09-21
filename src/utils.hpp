#include <array>



namespace mtn_utils {
    auto divided_cell_range(auto length, auto rank, auto nprocs) {
        auto ceil_div = [](auto num, auto den){ return num / den + (num % den != 0); };
        auto cells_per_proc = ceil_div(length, nprocs);
        auto first = std::min(cells_per_proc*rank, length);
        auto last = std::min(first+cells_per_proc, length);
        return std::array{first, last};
    }
};
