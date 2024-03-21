export solve!

using DifferentialEquations



"""
    solve!(m::MountainRange)

Step `m` through time until `m`'s steepness derivative falls to zero.
"""
function solve!(m::MountainRange{T, N}) where {T, N}
    stoppingcondition(h, args...) = dsteepness(h, growthrate(h, m.r))-eps(T)
    callback = ContinuousCallback(stoppingcondition, terminate!, interp_points=0)
    prob = ODEProblem{true}((g, h, r, t)->growthrate!(g, h, r), m.h, (0.0, 10000), m.r,
                             callback=callback, save_on=false, save_start=false)
    sol = solve(prob)
    m.t[] = last(sol.t)
    m.h .= last(sol.u)
    growthrate!(m.g, m.h, m.r)
    return m
end