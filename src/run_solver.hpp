#ifndef RUN_SOLVER_H
#define RUN_SOLVER_H



/* run_solver<MtnRange>(argc, argv, verbose=true)
 *
 * Create a mountain range of type MtnRange from an infile (argv[1]), solve it, and write it to an outfile (argv[2]).
 *
 * Returns 0 on success, 1 on failure, or 2 on bad usage.
 *
 * Meant to be used along the lines of:
 * int main(int argc, char **argv) {
 *     return run_solver<MountainRangeOpenMP>(argc, argv);
 * }
 */
template <class MtnRange>
int run_solver(int argc, char **argv, bool verbose=true) {
    // Usage
    auto help = [=](){
        std::cout << "Usage: " << argv[0] << " infile outfile" << std::endl
                  << "Read a mountain range from infile, solve it, and write it to outfile." << std::endl;
        if constexpr (requires { MtnRange::help_message; }) std::cout << MtnRange::help_message << std::endl;
        std::cout << "`" << argv[0] << " --help` prints this message." << std::endl;
    }; // https://tinyurl.com/byusc-lambda

    // Parse
    if (argc > 1 && (std::string(argv[1]) == std::string("-h") || std::string(argv[1]) == std::string("--help"))) {
        help();
        return 0;
    }
    if (argc != 3) {
        std::cerr << "Exactly two arguments must be supplied." << std::endl;
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
        if (verbose) std::cout << "Successfully read " << infile << std::endl;
        successful_read = true;
        // Solve
        m.solve();
        if (verbose) std::cout << "Solved; simulation time: " << m.sim_time() << std::endl;
        // Write to outfile
        m.write(outfile);
        if (verbose) std::cout << "Successfully wrote " << outfile << std::endl;
        return 0;

    // Handle errors
    } catch (const std::ios_base::failure &e) { // This includes MountainRangeIOException
        successful_read ? std::cerr << "Failed to write to "  << outfile
                        : std::cerr << "Failed to read from " << infile;
        std::cerr << ": " << e.what() << std::endl;
    } catch(const std::exception &e) {
        std::cerr << "Unrecognized error: " << e.what() << std::endl;
    }
    return 1;
}



#endif
