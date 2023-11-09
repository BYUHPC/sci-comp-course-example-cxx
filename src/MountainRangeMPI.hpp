#ifndef MOUNTAIN_RANGE_MPI_H
#define MOUNTAIN_RANGE_MPI_H
#include <array>
#include <mpl/mpl.h>
#include "MountainRange.hpp"



/* MountainRangeMPI splits r, h, and g across processes more or less evenly. Halo cells are stored at the borders
 * between processes. As an example, a mountain range of size 10 might be split data across processes thus:
 *
 * proc A:       proc C:
 * 0 1 2 3 4     7 8 9
 *       proc B:
 *       3 4 5 6 7 8
 *
 * Process A is in charge of updating cells 0-3, process B is in charge of updating cells 4-7, and process C is in
 * charge of updating cells 8-9. Because updaing a cell in g depends on the adjacent cells in the array, halos are also
 * stored--process A stores cell 4, process B cells 3 and 8, and process C cell 7. These halos are updated on each
 * iteration to keep the grid consistent between processes--for example, at the end of an iteration, process A will
 * receive the value in process B's cell 4 and store it in its own cell 4.
 *
 * The MPI within the class is completely self-contained--users don't need to explicitly make any MPI calls.
 */
class MountainRangeMPI: public MountainRange {
    // Convenience member
    mpl::cartesian_communicator comm_world;

    // Which cells this process should iterate over
    auto local_cell_range() const {
        auto [global_first, global_last] = this_process_cell_range();
        size_t first = global_first == 0 ? 0        : 1;
        size_t last =  global_last  == n ? r.size() : r.size()-1;
        return std::array{first, last};
    }

public:
    // Constructor
    MountainRangeMPI(auto && ...args): MountainRange(args...), comm_world{mpl::environment::comm_world()} {
        step(0); // initialize g
    }

    // Steepness derivative
    value_type dsteepness() {
        value_type global_ds, local_ds = 0;
        // Iterate over this process's cells
        auto [first, last] = local_cell_range();
        for (size_t i=first; i<last; i++) local_ds += ds_cell(i);
        // Sum the ds from all processes and return it
        comm_world::allreduce(std::plus<>(), local_ds, global_ds);
        return global_ds;
    }

private:
    // Swap halo cells between processes to keep simulation consistent between processes
    void exchange_halos(auto &x) {
        // Halos and first/last active cells
        const auto &first_halo      = x[0],
                   &first_real_cell = x[1],
                   &last_real_cell  = x[x.size()-2],
                   &last_halo       = x[x.size()-1];
        // Tags for sends and receives
        auto left_tag  = mpl::tag{0}, right_tag = mpl::tag{1};
        // Figure out where we are globally so we can know whether to send data left or right
        auto [global_first, global_last] = this_process_cell_range<true>(); // include halos
        // Exchange halos with the process to the left if there is such a process
        if (global_first > 0) {
            comm_world::sendrecv(first_real_cell, comm_world.rank()-1, left_tag,   // send
                                 first_halo,      comm_world.rank()-1, right_tag); // receive
        }
        // Exchange halos with the process to the right if there is such an active process
        if (global_last  < n) {
            comm_world::sendrecv(last_real_cell,  comm_world.rank()+1, right_tag,  // send
                                 last_halo,       comm_world.rank()+1, left_tag);  // receive
        }
    }

public:
    // Iterate from t to t+time_step in one step
    value_type step(value_type time_step) {
        auto [first, last] = local_cell_range();
        // Update h
        for (size_t i=first; i<last; i++) h[i] += time_step * g[i];
        // exchange_halos(h); // don't need to exchange h halos since there's no inter-cell dependency
        // Update g
        for (size_t i=first; i<last; i++) g[i] = g_cell(i);
        exchange_halos(g);
        // Increment and return t
        t += time_step;
        return t;
    }
};



#endif
