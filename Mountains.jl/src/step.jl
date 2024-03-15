export step!



"""
    growthrate!(g, h, r)

Update `g` to ``g=r-h³+∇²h`` in place and return it.
"""
function growthrate!(g::AbstractArray{<:Real, N}, h::AbstractArray{<:Real, N},
                     r::AbstractArray{<:Real, N}) where N
    @inbounds @fastmath @simd for i in interiorindices(h)
        g[i] = r[i]-h[i]^3+laplacian(h, i)
    end
    # Enforce boundary condition
    g[begin] = g[begin+1]
    g[end]   = g[end-1]
    return g
end

"""
    growthrate(h, r)

Equivalent to `growthrate!(similar(h), h, r)`.
"""
growthrate(h, r) = growthrate!(zeros(size(h)...), h, r)



"""
    step!(m::MountainRange, dt=$defaultdt)

Step `m` forward in time by `dt` in one step and return `m`.
"""
function step!(m::MountainRange, dt=defaultdt)
    @. @fastmath m.h += dt*m.g
    growthrate!(m.g, m.h, m.r)
    m.t[] += dt
    return m
end