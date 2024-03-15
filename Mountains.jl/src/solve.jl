export solve!



"""
    solve!(m::MountainRange, dt=$defaultdt)

Step `m` through time in steps of `dt` until `dsteepness(m)` falls below machine eps.
"""
function solve!(m::MountainRange{T}, dt=defaultdt) where T
    while dsteepness(m) > eps(T)
        step!(m, dt)
    end
    return m
end