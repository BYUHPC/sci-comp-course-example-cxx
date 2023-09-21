class MountainRangeOpenMP: public MountainRangeSharedMem {
public:
    MountainRangeOpenMP(auto &&...args): MountainRangeSharedMem(args...) { // https://tinyurl.com/byusc-parpack
        step(0); // initialize g
    }



    value_type step(value_type time_step) {
        // Update h
        #pragma omp parallel for
        for (size_t i=0; i<h.size(); i++) h[i] += time_step * g[i];
        // Update g
        #pragma omp parallel for
        for (size_t i=0; i<g.size(); i++) g[i] = g_cell(i);
        // Increment time step
        t += time_step;
    }



    value_type dsteepness() {
        value_type ds = 0;
        #pragma omp parallel for reduction(+:ds)
        for (size_t i=0; i<h.size(); i++) ds += ds_cell(i, time_step);
        return ds / h.size();
    }
};
