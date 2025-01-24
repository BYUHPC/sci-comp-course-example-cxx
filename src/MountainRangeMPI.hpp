#pragma once
#include <mpl/mpl.hpp>
#include "MountainRange.hpp"



namespace {
    // Read an element of type T from a certain offset (in bytes) in an mpl::file
    template <class T>
    T read_at_all(auto &f, auto offset) {
        std::remove_const_t<T> ret;
        f.read_at_all(offset, ret);
        return ret;
    }
}



/* MountainRangeMPI splits r, h, and g across processes more or less evenly. Halo cells are stored at the borders
 * between processes. As an example, a mountain range of size 10 might be split data across processes thus:
 *
 * proc A:       proc C:
 * 0 1 2 3 4     7 8 9
 *       proc B:
 *       3 4 5 6 7 8
 *
 * Process A is in charge of updating cells 0-3, process B is in charge of updating cells 4-7, and process C is in
 * charge of updating cells 8-9. Because updating a cell in g depends on the adjacent cells in the array, halos are also
 * stored--process A stores cell 4, process B cells 3 and 8, and process C cell 7. These halos are updated on each
 * iteration to keep the grid consistent between processes--for example, at the end of an iteration, process A will
 * receive the value in process B's cell 4 and store it in its own cell 4.
 *
 * The MPI within the class is completely self-contained--users don't need to explicitly make any MPI calls.
 */
class MountainRangeMPI: public MountainRange {
    // MPI-related members (initialized at the bottom of this file)
    static mpl::communicator comm_world;
    static const int comm_rank;
    static const int comm_size;



    // Determine which cells this process is in charge of updating
    auto this_process_cell_range() const {
        return mr::split_range(cells, comm_rank, comm_size);
    }



    // Read a MountainRange from an mpl::file
    MountainRangeMPI(mpl::file &&f): MountainRange(read_at_all<decltype(ndims)>(f, 0),
                                                   read_at_all<decltype(cells)>(f, sizeof(ndims)),
                                                   read_at_all<decltype(t)>(    f, sizeof(ndims)+sizeof(cells)),
                                                   3, 3) { // initialize r and h to minimum size; they're resized below
        // Figure out which cells this process is in charge of
        auto [first, last] = this_process_cell_range();
        if (first > 0)     first -= 1; // include left halo
        if (last  < cells) last  += 1; // include right halo

        // Resize the vectors
        r.resize(last-first);
        h.resize(last-first);
        g.resize(last-first);

        // Figure out read offsets
        auto r_offset = header_size + sizeof(value_type) * first;
        auto h_offset = r_offset + sizeof(value_type) * cells;

        // Read
        auto layout = mpl::vector_layout<value_type>(r.size());
        f.read_at(r_offset, r.data(), layout);
        f.read_at(h_offset, h.data(), layout);

        // Update g
        step(0);
    }



public:
    // Read a MountainRange from a file with MPI I/O, handling errors gracefully
    MountainRangeMPI(const char *filename) try: MountainRangeMPI(mpl::file(comm_world, filename,
                                                                 mpl::file::access_mode::read_only)) {
                                           } catch (const mpl::io_failure &e) {
                                               handle_read_failure(filename);
                                           }



    // Write a MountainRange to a file with MPI I/O, handling errors gracefully
    void write(const char *filename) const override try {
        // Open file write-only
        auto f = mpl::file(comm_world, filename, mpl::file::access_mode::create|mpl::file::access_mode::write_only);

        // Write header
        f.write_all(ndims);
        f.write_all(cells);
        f.write_all(t);

        // Figure out which part of r and h this process is in charge of writing
        auto [first, last] = this_process_cell_range(); // https://tinyurl.com/byusc-structbind
        auto layout = mpl::vector_layout<value_type>(last-first);
        auto r_offset = header_size + sizeof(value_type) * first;
        auto h_offset = r_offset + sizeof(value_type) * cells;
        auto halo_offset = comm_rank == 0 ? 0 : 1;

        // Write body
        f.write_at(r_offset, r.data()+halo_offset, layout);
        f.write_at(h_offset, h.data()+halo_offset, layout);

        // Handle errors
    } catch (const mpl::io_failure &e) {
        handle_write_failure(filename);
    }
 


    // Steepness derivative
    value_type dsteepness() const override {
        // Local and global dsteepness holders
        value_type global_ds, local_ds = 0;

        // Iterate over this process's cells
        for (size_t i=1; i<r.size()-1; i++) local_ds += ds_cell(i);

        // Sum the ds from all processes and return it
        comm_world.allreduce(std::plus<>(), local_ds, global_ds);
        return global_ds;
    }



private:
    // Swap halo cells between processes to keep simulation consistent between processes
    void exchange_halos(auto &x) {
        // Halos and first/last active cells
        auto       &first_halo      = x[0];
        auto       &last_halo       = x[x.size()-1];
        const auto &first_real_cell = x[1];
        const auto &last_real_cell  = x[x.size()-2];

        // Tags for sends and receives
        auto left_tag  = mpl::tag_t{0}, right_tag = mpl::tag_t{1}; // direction of data flow is indicated

        // Figure out where we are globally so we can know whether we need to send data
        auto [global_first, global_last] = this_process_cell_range(); // https://tinyurl.com/byusc-structbind

        // Exchange halos with the process to the left if there is such a process
        if (global_first > 0) {
            comm_world.sendrecv(first_real_cell, comm_rank-1, left_tag,   // send
                                first_halo,      comm_rank-1, right_tag); // receive
        }
        // Exchange halos with the process to the right if this process has a real end halo
        if (global_last < cells) {
            comm_world.sendrecv(last_real_cell,  comm_rank+1, right_tag,  // send
                                last_halo,       comm_rank+1, left_tag);  // receive
        }
    }



public:
    // Iterate from t to t+dt in one step
    value_type step(value_type dt) override {
        auto [global_first, global_last] = this_process_cell_range(); // https://tinyurl.com/byusc-structbind

        // Update h
        for (size_t i=0; i<h.size(); i++) update_h_cell(i, dt);

        // Update g
        for (size_t i=1; i<g.size()-1; i++) update_g_cell(i);
        exchange_halos(g);

        // Enforce boundary condition
        if (global_first == 0)    g[0]          = g[1];
        if (global_last == cells) g[g.size()-1] = g[g.size()-2];

        // Increment and return t
        t += dt;
        return t;
    }
};



// Initialize static MPI-related members
mpl::communicator MountainRangeMPI::comm_world = mpl::environment::comm_world();
const int MountainRangeMPI::comm_rank = mpl::environment::comm_world().rank();
const int MountainRangeMPI::comm_size = mpl::environment::comm_world().size();
