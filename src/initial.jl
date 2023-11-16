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



# Update dh to hold the result of `r-h³+∇²h`
function dhdt!(dh, h, r, t)
    # Laplacian array; first and last cells are special cases due to boundary condition
    L = [(h[begin+1]-h[begin])/2;
         @views (h[begin:end-2].+h[begin+2:end])./2 .- h[begin+1:end-1];
         (h[end-1]-h[end])/2]
    # Update dh in place
    dh .= r .- h.^3 .+ L
    # No need to return anything since dh has been mutated
    return nothing
end



# Return `2/X ∫∇h⋅∇ḣ dx`, where X is the length of h.
function dsteepness(h, r)
    # Calculate growth rate
    g = similar(h)
    dhdt!(g, h, r, 0)
    # Calculate ds
    return sum(function(i)
        left  = max(i-1, firstindex(h))
        right = min(i+1,  lastindex(h))
        return (h[right]-h[left]) * (g[right]-g[left]) / 2
    end, eachindex(h))/length(h)
end



# Solve and return a mountain range using ODEProblem
function solve!(m::MountainRange)
    cb = ContinuousCallback((h, t, integrator)->dsteepness(h, m.r), terminate!)
    timebounds = (0.0, typemax(eltype(m.r)))
    prob = ODEProblem{true}(dhdt!, m.h, timebounds, m.r, callback=cb)
    sol = solve(prob)
    m.h .= last(sol.u)
    m.t[] += last(sol.t)
    return m
end



# Run the same problem as initial.cpp
function main()
    n = 1000
    plateau = 251:500
    r = zeros(n)
    r[plateau] .= 1
    m = MountainRange(r, zeros(n), 0)
    solve!(m)
    println(m.t[])
end



# If this is the file being executed, run main
if abspath(PROGRAM_FILE) == @__FILE__
    main()
end
