namespace {
    template <class T>
    auto try_read_vector(std::istream &s, auto size) {
        auto x = std::vector<T>(size);
        try_read_bytes(s, x.data(), x.size());
        return x;
    }
};



MountainRange::MountainRange(std::istream &&s):
        n{/* begin immediately invoked lambda */[&s]{
            auto ndims = try_read_bytes<size_type>(s);
            if (ndims != 1) throw std::logic_error("this implementation only handles one-dimensional mountain ranges");
            return try_read_bytes<decltype(n)>(s);
        }()/*  end immediately invoked lambda */},
        t{try_read_bytes<decltype(t)>(s)},
        r{try_read_vector<value_type>(s, n)},
        h{/* begin immediately invoked lambda */[&s]{
            auto h = try_read_vector<value_type>(s, n);
            if (s.peek() != traits::eof()) throw std::logic_error("extra bytes found after the end of height");
            return h;
        }()/*  end immediately invoked lambda */},
        g(h.size()) {}



class MountainRangeSharedMem: public MountainRange {
public:
    using MountainRange::MountainRange;

    MountainRangeSharedMem(const char *const filename): MountainRange(std::ifstream(filename)) {}



    bool write(const char *const filename) {
        auto s = std::ofstream(filename);
        size_type ndims = 1, n = h.size();
                // Write header
        return (try_write_bytes(s, &ndims, &n, &t) &&
                // Write uplift rate
                try_write_bytes(s, r.data(), n) &&
                // Write height
                try_write_bytes(s, h.data(), n));
    }
};