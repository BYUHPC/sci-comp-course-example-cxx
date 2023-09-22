#ifndef MOUNTAIN_RANGE_MPI_H
#define MOUNTAIN_RANGE_MPI_H
#include <array>
#include <tuple>
#include <mpi.h>
#include "utils.hpp"
#include "MountainRange.hpp"



namespace {
    // Convenience wrappers for MPI I/O
    // Run MPI_File_read_at or MPI_File_write_at, throwing a MountainRangeIOException on read or write failure
    enum ReadOrWrite { Read, Write };
    template <ReadOrWrite RW>
    bool try_mpi_file_rw_at(auto f, auto offset, auto *data, size_t size) {
        auto error = [](auto &&...args){
            if constexpr (RW == Read) return MPI_File_read_at(args...);
            else                      return MPI_File_write_at(args...);
        }(f, offset, data, sizeof(*data)*size, MPI_BYTE, MPI_STATUS_IGNORE);
        if (error) {
            throw std::logic_error(std::string("failed MPI_File_") + (RW == Read ? "read" : "write") + "_at");
            return false;
        }
        return true;
    }

    // Read into an address
    bool try_mpi_file_read_at(auto &&...args) { // https://tinyurl.com/byusc-parpack
        return try_mpi_file_rw_at<Read>(args...);
    }

    // Write from an address
    bool try_mpi_file_write_at(auto &&...args) { // https://tinyurl.com/byusc-parpack
        return try_mpi_file_rw_at<Write>(args...);
    }

    // Read and return a value
    template <class T>
    T try_mpi_file_read_at(auto f, auto offset) {
        T data;
        try_mpi_file_read_at(f, offset, &data, 1);
        return data;
    }



    // Size of header in bytes
    constexpr const size_t header_size = sizeof(MountainRange::size_type) * 2 + sizeof(MountainRange::value_type);



    // Singleton-stype MPI rank and size functions
    static int _mpi_rank = -1;
    auto mpi_rank() {
        if (_mpi_rank > -1) return _mpi_rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &_mpi_rank);
        return _mpi_rank;
    }

    static int _mpi_size = -1;
    auto mpi_size() {
        if (_mpi_size > -1) return _mpi_size;
        MPI_Comm_size(MPI_COMM_WORLD, &_mpi_size);
        return _mpi_size;
    }
}



// Initialize a MountainRange from an open MPI_File
MountainRange::MountainRange(MPI_File &&f):
        N{[&f]{ // https://tinyurl.com/byusc-lambdai
            auto N = try_mpi_file_read_at<size_type>(f, 0);
            if (N != 1) throw std::logic_error("this implementation only supports one dimensional mountain ranges");
            return N;
        }()},
        n{try_mpi_file_read_at<size_type>(f, sizeof(size_type))},
        t{try_mpi_file_read_at<value_type>(f, sizeof(size_type)*2)},
        r{[&f, this]{ // https://tinyurl.com/byusc-lambdai
            auto [first, last] = mtn_utils::divided_cell_range(n, mpi_rank(), mpi_size());
            first = first == 0 ? first : first-1; // left halo
            last = std::min(last+1, n); // right halo
            std::vector<value_type> r(last-first);
            std::cout << "FIRST: " << first << "; LAST: " << last << std::endl;
            try_mpi_file_read_at(f, header_size+sizeof(value_type)*first, r.data(), r.size());
            return r;
        }()},
        h{[&f, this]{ // https://tinyurl.com/byusc-lambdai
            auto [first, last] = mtn_utils::divided_cell_range(n, mpi_rank(), mpi_size());
            first = first == 0 ? first : first-1; // left halo
            last = std::min(last+1, n); // right halo
            std::vector<value_type> h(last-first);
            try_mpi_file_read_at(f, header_size+sizeof(value_type)*(first+n), h.data(), h.size());
            return h;
        }()},
        g(h.size()) {}



/* MountainRangeMPI is blah
 *
 * TODO: explain how data is split across processes
 * 
 * The MPI within the class is completely self-contained; the only explicit interaction with MPI required on the part of
 * the user are calls to MPI_Init and MPI_Finalize.
 */
class MountainRangeMPI: public MountainRange {
public:
    // Constructor
    MountainRangeMPI(const char *const filename):
            MountainRange([filename]{
                MPI_File f;
                auto open_error = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &f);
                if (open_error) throw std::logic_error(std::string("could not open ") + filename);
                return f;
            }()) {
        step(0); // initialize g
    }



    bool write(const char *const filename) const {
        MPI_File f;
        auto error = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &f);
        if (error) throw std::logic_error(std::string("could not open ") + filename);
        // First process writes header
        bool good_write = true; // until proven otherwise
        if (mpi_rank() == 0) good_write &= try_mpi_file_write_at(f, 0,                   &N, 1)
                                        && try_mpi_file_write_at(f, sizeof(size_type),   &n, 1)
                                        && try_mpi_file_write_at(f, sizeof(size_type)*2, &t, 1);
        auto [first, last] = mtn_utils::divided_cell_range(n, mpi_rank(), mpi_size());
        if (first != last) good_write &= try_mpi_file_write_at(f, header_size+sizeof(value_type)*first,
                                                               r.data()+(mpi_rank()!=0), last-first)
                                      && try_mpi_file_write_at(f, header_size+sizeof(value_type)*(first+n),
                                                               h.data()+(mpi_rank()!=0), last-first);
        return good_write;
    }



    value_type dsteepness() {
        value_type ds, local_ds = 0;
        for_each_cell_this_proc([this, &local_ds](auto i){
            local_ds += ds_cell(i);
        });
        MPI_Allreduce(&local_ds, &ds, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
        return ds / n;
    }



    value_type step(value_type time_step) {
        // Halo exchange function
        auto exchange_halos = [this](auto &x) {
            // Convenience wrapper for MPI_Sendrecv
            auto mpi_send_recv_one_value = [](auto *send, auto *recv, int neighbor, int send_tag=0, int recv_tag=0){
                MPI_Sendrecv(send, sizeof(*send), MPI_BYTE, neighbor, send_tag,
                             recv, sizeof(*recv), MPI_BYTE, neighbor, recv_tag,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }; // https://tinyurl.com/byusc-lambda
            // Exchange left halo
            if (x.size() > 2 && mpi_rank() > 0) {
                mpi_send_recv_one_value(x.data()+1,    x.data(),    mpi_rank()-1, 0, 1);
            }
            // Exchange right halo
            if (x.size() > 2 && mtn_utils::divided_cell_range(n, mpi_rank(), mpi_size())[1] < n) {
                mpi_send_recv_one_value(&(x.back())-1, &(x.back()), mpi_rank()+1, 1, 0);
            }
        };
        // Update h
        for_each_cell_this_proc([this, time_step](auto i){
            h[i] += time_step * g[i];
        });
        exchange_halos(h);
        // Update g
        for_each_cell_this_proc([this](auto i){
            g[i] = g_cell(i);
        });
        exchange_halos(g);
        // Increment and return t
        t += time_step;
        return t;
    }



private:
    // Helper function for iteration
    void for_each_cell_this_proc(auto F) {
        size_t first = mpi_rank() != 0,
               last  = n == mtn_utils::divided_cell_range(n, mpi_rank(), mpi_size())[1] ? h.size() : h.size()-1;
        for (auto i=first; i<last; i++) F(i);
    }
};



#endif
