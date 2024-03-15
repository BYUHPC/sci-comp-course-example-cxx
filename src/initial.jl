#!/usr/bin/env julia

using DifferentialEquations



# This is an implementation of the same problem as solved in initial.cpp, but using Julia's
# DifferentialEquations library. It's not optimized, instead emphasizing understandability.



# Barebones mountain range struct with uplift rate, height, and simulation time
struct MountainRange
    r::Vector{Float64} # uplift rate
    h::Vector{Float64} # current height
    t::Ref{Float64}    # simulation time
end



# Get the result of `r-h³+∇²h`
function dhdt(h, r, t)
    # Allocate growth rate array
    g = zeros(length(h))

    # Set each interior cell of g
    for i in firstindex(h)+1:lastindex(h)-1
        L = (h[i-1]+h[i+1])/2 - h[i]
        g[i] = r[i] - h[i]^3 + L
    end

    # Enforce bounday condition
    g[begin] = g[begin+1]
    g[end]   = g[end-1]

    # Return g
    return g
end



# Return `2/X ∫∇h⋅∇ḣ dx`, where X is the length of h.
function dsteepness(h, r)
    # Calculate growth rate
    g = dhdt(h, r, 0)

    # Calculate ds
    return sum(function(i)
        return (h[i+1] - h[i-1]) * (g[i+1] - g[i-1]) / 2
    end, firstindex(h)+1:lastindex(h)-1)/(length(h)-2)
end



# Solve and return a mountain range using ODEProblem
function solve!(m::MountainRange)
    # Callback that tells the solver when it's done (when dsteepness crosses zero)
    terminationcondition(h, t, integrator) = dsteepness(h, m.r)
    cb = ContinuousCallback(terminationcondition, terminate!)

    # Define and solve the problem
    timespan = (0.0, typemax(eltype(m.r)))
    prob = ODEProblem(dhdt, m.h, timespan, m.r, callback=cb)
    sol = solve(prob)

    # Update and return m
    m.h .= last(sol.u)
    m.t[] += last(sol.t)
    return m
end



# Run the same problem as initial.cpp
function main()
    # Simulation parameters
    n = 1000
    plateau = 251:500
    r = zeros(n)
    r[plateau] .= 1
    h = zeros(n)
    h[begin] = 1
    t = 0.0

    # Set up and solve the mountain range, printing the simulation time
    m = MountainRange(r, h, t)
    solve!(m)
    println(m.t[])
end



# If this is the file being executed, run main
if abspath(PROGRAM_FILE) == @__FILE__
    main()
end
