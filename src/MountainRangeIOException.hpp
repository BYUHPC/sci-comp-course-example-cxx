#ifndef MOUNTAIN_RANGE_READ_EXCEPTION_H
#define MOUNTAIN_RANGE_READ_EXCEPTION_H
#include <ios>



// Exception class for failures in mountain range constructors taking a filename and MountainRange::write
// Inherits from std::ios_base::failure for convenience in run_solver.hpp
class MountainRangeIOException: public std::ios_base::failure {
    std::string message;

public:
    MountainRangeIOException(auto &&message): std::ios_base::failure(message), message(message) {}

    const char *what() const noexcept {
        return message.c_str();
    }
};



#endif
