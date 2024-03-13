#include <vector>
#include <iostream>
#include "MountainRange.hpp"



int main() {
    // Simulation parameters
    size_t len = 1000;
    double value = 1;
    size_t plateau_start = 250, plateau_end = 500;

    // Construct mountain range
    std::vector<decltype(value)> r(len), h(len);
    std::fill(r.begin()+plateau_start, r.begin()+plateau_end, value);
    auto m = MountainRange(r, h);

    // Solve and return
    std::cout << m.solve() << std::endl;
    return 0;
}
