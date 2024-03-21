#!/usr/bin/env julia



# This script is used to generate sample mountain ranges



using Mountains, Plots



# Return an array containing a chirp of height 1
function chirp(startfreq, endfreq, duration, rate)
    t = range(0, duration, length=duration*rate)
    k = (endfreq-startfreq)/duration
    return @. sin(2π*t*(startfreq+t*k/2))
end



# Return an array with a gaussian hump of height 1
function gaussianhump(length, pos=(length+1)/2, sigma=length/8)
    return @. exp(-((1:length)-pos)^2/2sigma^2)
end



for (msize, r, h) in (("tiny",  chirp(0, 15, 1, 100), range(-0.1, 0.1, length=100)),
                      ("small", 25*chirp(0, 100, 1, 10000).+gaussianhump(10000).*200,
                                0.01*chirp(0, 100, 1, 10000)))
    # Built and write the initial mountain range
    m = MountainRange(r, h)
    write("1d-$msize-in.mr", m)

    # Allocate arrays to store results for plotting
    H = [deepcopy(m.h)]
    S = [steepness(m)]
    DS = [dsteepness(m)]

    # Solve, storing the results of the first few iterations
    while dsteepness(m) > eps(Float64)
        step!(m)
        if steepness(m)-S[end]>2e-4
            push!(H, deepcopy(m.h))
            push!(S, steepness(m))
            push!(DS, dsteepness(m))
        end
    end

    # Write the solved mountain range
    write("1d-$msize-out.mr", m)

    # Normalize tracked values
    RN = r ./ max(abs.(m.r)...)
    Hmax = max((max(abs.(x)...) for x in H)...)
    HN = [x ./ Hmax for x in H]
    SN = S ./ max(abs.(S)...)
    DSN = DS ./ max(abs.(DS)...)

    # Animate the simulation's progression over time
    @gif for i in eachindex(HN)
        plot(begin
                 plot(HN[i], ylim=(-1, 1), grid=false, axis=false, ticks=1:0, color=:blue, legendposition=:right, label="height")
                 plot!(RN, color=:red, label="rate")
             end,
             begin
                 plot(SN[begin:i], xlim=(1, length(SN)), ylim=(0, 1), grid=false, axis=false, ticks=1:0, color=:green, legendposition=:right, label="steepness")
                 plot!(DSN[begin:i], color=:magenta, label="derivative")
             end, layout=(2, 1))
    end every Int(round(length(HN)/50))
end