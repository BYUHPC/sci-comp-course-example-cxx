#include <mpi.h>
#include "MountainRangeMPI.hpp"
#include "run_solver.hpp"



int main(int argc, char **argv) {
    // MPI setup
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    // Run the solver, only printing status messages in the first process
    bool verbose = rank == 0;
    auto ret = run_solver<MountainRangeMPI>(argc, argv, verbose);
    // MPI teardown
    MPI_Finalize();
    return ret;
}
