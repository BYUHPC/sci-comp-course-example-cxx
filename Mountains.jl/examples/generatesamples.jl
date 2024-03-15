#!/usr/bin/env julia



using Mountains



function chirp(startfreq, endfreq, duration, rate)
    t = range(0, duration, length=duration*rate)
    k = (endfreq-startfreq)/duration
    return @. sin(2π*t*(startfreq+t*k/2))
end



function gaussianhump(length, pos=(length+1)/2, sigma=length/8)
    return @. exp(-((1:length)-pos)^2/2sigma^2)
end



for (msize, r, h) in (("tiny",  chirp(0, 15, 1, 100), range(-0.1, 0.1, length=100)),
                      ("small", 25*chirp(0, 100, 1, 10000).+gaussianhump(10000).*200,
                                0.01*chirp(0, 100, 1, 10000)))
    m = MountainRange(r, h)
    write("1d-$msize-in.mr", m)
    solve!(m)
    write("1d-$msize-out.mr", m)
end