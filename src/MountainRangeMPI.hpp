#ifndef MOUNTAIN_RANGE_MPI_H
#define MOUNTAIN_RANGE_MPI_H
#include <array>
#include <tuple>
#include <mpi.h>
#include "MountainRange.hpp"
#include "MountainRangeIOException.hpp"



// Anonymous namespace with convenience wrappers for MPI_File_{read,write}_at
namespace {
    // Run MPI_File_read_at or MPI_File_write_at, throwing a MountainRangeIOException on read or write failure
    enum ReadOrWrite { Read, Write };
    template <ReadOrWrite RW>
    bool try_mpi_file_rw_at(auto f, auto offset, auto *data, size_t size) {
        auto error = [](auto &&...args){
            if constexpr (RW == Read) return MPI_File_read_at(args...);
            else                      return MPI_File_write_at(args...);
        }(f, offset, data, sizeof(*data)*size, MPI_BYTE, MPI_STATUS_IGNORE);
        if (error) {
            throw MountainRangeIOException(std::string("failed MPI_File_") + (RW == Read ? "read" : "write") + "_at");
            return false;
        }
        return true;
    }

    // Read into an address
    bool try_mpi_file_read_at(auto &&...args) {
        return try_mpi_file_rw_at<Read>(args...);
    }

    // Write from an address
    bool try_mpi_file_write_at(auto &&...args) {
        return try_mpi_file_rw_at<Write>(args...);
    }

    // Read and return a value
    template <class T>
    T try_mpi_file_read_at(auto f, auto offset) {
        T data;
        try_mpi_file_read_at(f, offset, &data, 1);
        return data;
    }
}



/* MountainRangeMPI is blah
 *
 * TODO: explain how data is split across processes
 * 
 * The MPI within the class is completely self-contained; the only explicit interaction with MPI required on the part of
 * the user are calls to MPI_Init and MPI_Finalize.
 */
class MountainRangeMPI: public MountainRange {
    // Members
    static constexpr const size_t header_size = sizeof(size_type) * 2 + sizeof(value_type);
    const size_type n;                         // total size of the mountain range
    const int mpi_size, mpi_rank;              // MPI size and rank
    const size_type global_first, global_last, // absolute first/last cells that this process manages
                    first, last;               // relative (to r, h, and g) first/last cells that this process manages



public:
    // Constructors
    // Initialize from all members, including supertype members; used in the filename constructor
    MountainRangeMPI(const auto &r, const auto &h, auto t, auto n, auto mpi_size, auto mpi_rank,
                     auto global_first, auto global_last, auto first, auto last):
            MountainRange(r, h, t), n{n}, mpi_size{mpi_size}, mpi_rank{mpi_rank},
            global_first{global_first}, global_last{global_last}, first{first}, last{last} {
        step(0);
    }

    // Instantiate a MountainRangeMPI by reading from a file
    MountainRangeMPI(const char *const filename): // TODO: there's a lot going on here, explain std::move and std::make_from_tuple
            MountainRangeMPI(std::move(std::make_from_tuple<MountainRangeMPI>([filename]{ // ...and the immediately evaluated lambda
        // Figure out size and rank
        int mpi_size, mpi_rank;
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
        // Make sure the specified file is at least 12 bytes
        check_file_size(filename);
        // Open input file
        MPI_File f;
        auto open_error = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &f);
        if (open_error) throw MountainRangeIOException(std::string("could not open ") + filename);
        // Read header
        auto ndims = try_mpi_file_read_at<size_type>(f, 0);
        if (ndims != 1) throw MountainRangeIOException("this implementation only accepts 1-D mountain ranges");
        auto n = try_mpi_file_read_at<size_type>(f, sizeof(size_type));
        auto t = try_mpi_file_read_at<value_type>(f, sizeof(size_type)*2);
        // Determine which section of the grid this process is responsible for
        auto [global_first, global_last] = divided_cell_range(n, mpi_rank, mpi_size);
        size_t first = mpi_rank == 0 || global_first == global_last ? 0 : 1;
        size_t last = first + global_last - global_first;
        size_t subsize = last - first;
        if (mpi_rank > 0 && first != last) subsize += 1; // left edge
        if (global_last != n) subsize += 1;              // right edge
        // Read body
        std::vector<value_type> r(subsize), h(subsize);
        auto r_offset = header_size + sizeof(value_type) * global_first;
        auto h_offset = r_offset + sizeof(value_type) * n;
        try_mpi_file_read_at(f, r_offset, r.data()+first, last-first);
        try_mpi_file_read_at(f, h_offset, h.data()+first, last-first);
        // Return tuple to delegate to helper constructor
        return std::make_tuple(r, h, t, n, mpi_size, mpi_rank, global_first, global_last, first, last);
    }()))) {}

    value_type dsteepness() {
        auto local_ds = ds_section(first, last);
        value_type ds;
        MPI_Allreduce(&local_ds, &ds, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        return ds / n;
    }

private:
    void exchange_halos(auto &x) {
        // Convenience wrapper for MPI_Sendrecv
        auto mpi_send_recv_one_value = [](auto *send, auto *recv, int neighbor, int send_tag=0, int recv_tag=0){
            MPI_Sendrecv(send, sizeof(*send), MPI_BYTE, neighbor, send_tag,
                         recv, sizeof(*recv), MPI_BYTE, neighbor, recv_tag,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        };
        // Exchange left halo
        if (first != last && mpi_rank > 0) {
            mpi_send_recv_one_value(x.data()+1,    x.data(),    mpi_rank-1, 0, 1);
        }
        // Exchange right halo
        if (first != last && global_last != n) {
            mpi_send_recv_one_value(&(x.back())-1, &(x.back()), mpi_rank+1, 1, 0);
        }
    }

public:
    value_type step(value_type time_step) {
        // Update h
        update_h_section(first, last, time_step);
        exchange_halos(h);
        // Update g
        update_g_section(first, last);
        exchange_halos(g);
        // Increment and return t
        t += time_step;
        return t;
    }

    bool write(const char *const filename) const {
        MPI_File f;
        auto error = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
        if (error) throw MountainRangeIOException(std::string("could not open ") + filename);
        // First process writes header
        bool good_write = true; // until proven otherwise
        size_type ndims = 1;
        if (mpi_rank == 0) {
            good_write &= try_mpi_file_write_at(f, 0,                   &ndims, 1) &&
                          try_mpi_file_write_at(f, sizeof(size_type),   &n,     1) &&
                          try_mpi_file_write_at(f, sizeof(size_type)*2, &t,     1);
        }
        // All processes write their portion of the body
        auto r_offset = header_size + sizeof(value_type) * global_first;
        auto h_offset = r_offset + sizeof(value_type) * n;
        if (first != last) good_write &= try_mpi_file_write_at(f, r_offset, r.data()+first, last-first) &&
                                         try_mpi_file_write_at(f, h_offset, h.data()+first, last-first);
        return good_write;
    }
};



#endif
