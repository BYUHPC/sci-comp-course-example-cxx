#ifdef MPI_VERSION
#include <mpl/mpl.hpp>
#endif



// Compile with -DUSE_OPENMP for OpenMP version, -DUSE_THREAD for pthread version, etc.
#if defined(USE_OPENMP)
#include "MountainRangeOpenMP.hpp"
using MtnRange = MountainRangeOpenMP;
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
    enum class stream { stdout, stderr };
    template <stream S=stream::stdout>
    void print(auto && ...args) {
#ifdef MPI_VERSION
        if (mpl::environment::comm_world().rank() > 0) return; // only print in the main thread
#endif
        if constexpr (S==stream::stdout) {
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
        print<stream::stderr>("Exactly two arguments must be supplied.");
        help();
        return 2;
    }
    auto infile = argv[1];
    auto outfile = argv[2];

    // Run
    bool successful_read = false; // until proven otherwise
    try {
        // Read from infile
        auto m = MtnRange(infile);
        print("Successfully read ", infile);
        successful_read = true;
        // Solve
        m.solve();
        print("Solved; simulation time: ", m.sim_time());
        // Write to outfile
        m.write(outfile);
        print("Successfully wrote ", outfile);
        return 0;

    // Handle errors
    } catch (const std::logic_error &e) {
        print<stream::stderr>(e.what());
    } catch(const std::exception &e) {
        print<stream::stderr>("Unrecognized error: ", e.what());
    }
    return 1;
}
