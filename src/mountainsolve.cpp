#include <iostream>
#ifdef MPI_VERSION
#include <mpl/mpl.hpp>
#endif



// Compile with -DUSE_OPENMP for OpenMP version, -DUSE_THREAD for pthread version, etc.
#if defined(USE_OPENMP)
#include "MountainRange.hpp"
using MtnRange = MountainRange;
#elif defined(USE_THREAD)
#include "MountainRangeThreaded.hpp"
using MtnRange = MountainRangeThreaded;
#elif defined(USE_GPU)
#include "MountainRangeGPU.hpp"
using MtnRange = MountainRangeGPU;
#elif defined(USE_MPI)
#include "MountainRangeMPI.hpp"
using MtnRange = MountainRangeMPI;
#endif



// Print function that will only print in the first process if MPI is being used
namespace {
    enum class to { stdout, stderr };
    template <to S=to::stdout>
    void print(auto && ...args) {
#ifdef MPI_VERSION
        if (mpl::environment::comm_world().rank() > 0) return; // only print in the main thread
#endif
        if constexpr (S==to::stdout) {
            (std::cout << ... << args);
            std::cout << std::endl;
        } else {
            (std::cerr << ... << args);
            std::cerr << std::endl;
        }
    }
};



// Create a mountain range from an infile (argv[1]), solve it, and write it to an outfile (argv[2]).
int main(int argc, char **argv) {
    // Function to print a help message
    auto help = [=](){
        print("Usage: ", argv[0], " infile outfile");
        print("Read a mountain range from infile, solve it, and write it to outfile.");
#ifdef USE_THREAD
        print(MtnRange::help_message);
#endif
        print("`", argv[0], " --help` prints this message.");
    }; // https://tinyurl.com/byusc-lambda



    // Parse
    if (argc > 1 && (std::string(argv[1]) == std::string("-h") || std::string(argv[1]) == std::string("--help"))) {
        help();
        return 0;
    }
    if (argc != 3) {
        print<to::stderr>("Exactly two arguments must be supplied.");
        help();
        return 2;
    }
    auto infile = argv[1];
    auto outfile = argv[2];



    // Run
    try {
        // Read from infile
        auto m = MtnRange(infile);
        print("Successfully read ", infile);

        // Solve
        m.solve();
        print("Solved; simulation time: ", m.sim_time());

        // Write to outfile
        m.write(outfile);
        print("Successfully wrote ", outfile);

        // Return 0 if we made it this far
        return 0;

    // Handle errors
    } catch (const std::logic_error &e) {
        print<to::stderr>(e.what(), "; aborting");
    } catch(const std::exception &e) {
        print<to::stderr>("Unrecognized error: ", e.what(), "; aborting");
    }
    return 1;
}