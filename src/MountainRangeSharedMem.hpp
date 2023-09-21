#include <fstream>
#include "binary_io.hpp"
#include "MountainRange.hpp"



namespace {
    template <class T>
    auto try_read_vector(std::istream &s, auto size) {
        auto x = std::vector<T>(size);
        try_read_bytes(s, x.data(), x.size());
        return x;
    }
};



MountainRange::MountainRange(std::istream &&s):
        N{[&s]{
            auto N = try_read_bytes<size_type>(s);
            if (N != 1) throw std::logic_error("this implementation only supports one-dimensional mountain ranges");
            return N;
        }()},
        n{try_read_bytes<decltype(n)>(s)},
        t{try_read_bytes<decltype(t)>(s)},
        r(try_read_vector<value_type>(s, n)),
        h(/* begin immediately invoked lambda */[this, &s]{
            auto h = try_read_vector<value_type>(s, n);
            //if (s.peek() != EOF) throw std::logic_error("extra bytes found after the end of height");
            return h;
        }()/*  end immediately invoked lambda */),
        g(h.size()) {}



class MountainRangeSharedMem: public MountainRange {
public:
    using MountainRange::MountainRange;

    MountainRangeSharedMem(const char *const filename): MountainRange(std::ifstream(filename)) {}



    bool write(const char *const filename) const {
        auto s = std::ofstream(filename);
                // Write header
        return (try_write_bytes(s, &N, &n, &t) &&
                // Write uplift rate
                try_write_bytes(s, r.data(), n) &&
                // Write height
                try_write_bytes(s, h.data(), n));
    }
};