#include "MountainRangeGPU.hpp"
#include "run_solver.hpp"



int main(int argc, char **argv) {
    return run_solver<MountainRangeGPU>(argc, argv);
}
