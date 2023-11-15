#!/usr/bin/env julia



using DifferentialEquations



struct MountainRange
    r::Vector{Float64} # uplift rate
    h::Vector{Float64} # current height
    t::Ref{Float64}    # simulation time
end



"""
    dhdt!(dh, h, r, t)

Update `dh` to hold the results of `r-h³+∇²h`
"""
function dhdt!(dh, h, r, t)
    # First cell is different due to boundary condition
    dh[begin] = r[begin] - h[begin]^3 + (h[begin+1]-h[begin])/2
    # Middle cells can be broadcast
    M = CartesianIndices((firstindex(dh)+1:lastindex(dh)-1)) # interior indices
    L = @views (h[begin:end-2] .+ h[begin+2:end])./2 .- h[M]
    @views dh[M] .= r[M] .- h[M].^3 .+ L
    # Last cell is also different
    dh[end] = r[end] - h[end]^3 + (h[end-1]-h[end])/2
    return nothing
end



"""
    dsteepness(h, r)

Return `2/X ∫∇h⋅∇ḣ dx`, where `X` is the length of `h`.
"""
function dsteepness(h, r)
    # Calculate growth rate
    g = zeros(length(h))
    dhdt!(g, h, r, 0)
    # Calculate ds
    return sum(function(i)
        left  = max(i-1, firstindex(h))
        right = min(i+1,  lastindex(h))
        return (h[right]-h[left]) * (g[right]-g[left]) / 2
    end, eachindex(h))/length(h)
end



function solve!(m::MountainRange)
    cb = ContinuousCallback((h, t, integrator)->dsteepness(h, m.r), terminate!)
    timebounds = (0.0, typemax(eltype(m.r)))
    prob = ODEProblem{true}(dhdt!, m.h, timebounds, m.r,
                            callback=cb, save_on=false, save_start=false)
    sol = solve(prob)
    m.h .= last(sol.u)
    m.t[] += last(sol.t)
    return m
end



function main()
    n = 1000
    plateau = 251:500
    r = zeros(n)
    r[plateau] .= 1
    m = MountainRange(r, zeros(n), 0)
    solve!(m)
    println(m.t[])
end



if abspath(PROGRAM_FILE) == @__FILE__
    main()
end
