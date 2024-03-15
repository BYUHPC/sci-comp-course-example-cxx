export solve!

using DifferentialEquations



"""
    solve!(m::MountainRange)

Step `m` through time until the derivative of `m`'s steepness falls to zero.
"""
function solve!(m::MountainRange{T, N}) where {T, N}
    callback = ContinuousCallback((u, t, integrator)->dsteepness(u, growthrate(u, m.r)),
                                  terminate!)
    prob = ODEProblem{true}((g, h, r, t)->growthrate!(g, h, r), m.h, (0.0, typemax(T)), m.r,
                             callback=callback, save_on=false, save_start=false)
    sol = solve(prob)
    m.t[] = last(sol.t)
    m.h .= last(sol.u)
    growthrate!(m.g, m.h, m.r)
    return m
end