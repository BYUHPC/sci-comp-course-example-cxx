#include <iostream>
#include <numeric>
#include <ranges>
#include <sstream>
#include "MountainRange.hpp"



// This program compares two mountain ranges to determine whether they represent the same mountain range.
// Usage: mountaindiff range1.mr range2.mr



// Tolerances
const MountainRange::value_type acceptable_time_ratio         = 1.0001,
                                acceptable_height_error_ratio = 0.000001; // TODO: should this increase with time?



int main(int argc, char **argv) {
    // Set the return code to 1 and print a message if condition is false
    int ret = 0;
    auto ensure = [&](bool correct, auto &&...message){ // https://tinyurl.com/byusc-parpack
        if (!correct) {
            (std::cerr << ... << message); // https://tinyurl.com/byucs-foldexp
            std::cerr << std::endl;
            ret = 1;
        }
    };

    // Parse
    const auto help_message = [&]{
        std::stringstream s;
        s << "Usage: " << argv[0] << " expected.mr actual.mr" << std::endl
          << "Compare the mountain ranges in files expected.mr and actual.mr, returning 0 if they seem to represent "
          << "the same mountain range, or printing an error message and returning 1 if not.";
        return s.str();
    }(); // https://tinyurl.com/byusc-lambdai
    if (argc > 1 && (std::string(argv[1]) == std::string("-h") || std::string(argv[1]) == std::string("--help"))) {
        std::cout << help_message << std::endl;
        return 0;
    }
    if (argc != 3) {
        std::cerr << "Exactly two arguments must be supplied" << std::endl << help_message << std::endl;
        return 2;
    }
    auto filename1 = argv[1];
    auto filename2 = argv[2];

    // Read in both hotplates
    const auto m1 = MountainRange(filename1);
    const auto m2 = MountainRange(filename2);
    const auto &r1 = m1.uplift_rate(), &r2 = m2.uplift_rate(), &h1 = m1.height(), &h2 = m2.height();
    auto t1 = m1.sim_time(), t2 = m2.sim_time();

    // Make sure that times are about the same
    auto time_ratio = t1 > 0 || t2 > 0 ? t1 / t2 : 1;
    ensure(time_ratio < acceptable_time_ratio && time_ratio > 1/acceptable_time_ratio,
           "Simulation times (", t1, " and ", t2, ") are not within tolerance");

    // Make sure sizes are the same
    ensure(h1.size() == h2.size(), "Sizes (", h1.size(), " and ", h2.size(), ") are not the same");

    // Make sure that r are equal
    ensure(m1.uplift_rate() == m2.uplift_rate(), "Growth rates are not equal");

    // Make sure that h are about the same
    if (h1.size() == h2.size()) { // no point of comparison if lengths are different
        auto inds = std::views::iota(0ul, h1.size());
        auto m1_height_rms = sqrt(std::transform_reduce(h1.begin(), h1.end(), decltype(m1)::value_type{}, std::plus<>(),
                                                        [](auto h){ return h * h; } // https://tinyurl.com/byusc-lambda
                                 ) / h1.size());
        auto height_difference_rms = sqrt(std::transform_reduce(inds.begin(), inds.end(), decltype(m1)::value_type{},
                                                                std::plus<>(),
                                                                [&](auto i){ return pow(h1[i] - h2[i], 2); }
                                         ) / h1.size());
        auto height_error_ratio = height_difference_rms ? height_difference_rms / m1_height_rms : 0;
        ensure(height_error_ratio < acceptable_height_error_ratio,
               "Heights are not within tolerance (height error ratio is ", height_error_ratio, ")");
    }

    // ret will have been set to 1 if any ensure failed, otherwise it's 0
    return ret;
}
