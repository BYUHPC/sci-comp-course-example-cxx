using Mountains, Test



"""
    randarrays()

Return a tuple of `randn` arrays of `ndims` from 1 to 4 with semi-random sizes.

The length of each array will be within ``[4⁴, 7⁴]`` and its size will be close to square.

# Examples

```jldoctest
julia> size.(randarrays())
((256,), (16, 18), (8, 8, 7), (3, 4, 6, 6))

julia> first.(randarrays())
(0.9352103498074876, -0.8629590964138757, 0.5395087495398293, 1.223069824711314)
```
"""
function randarrays()
    return ntuple(n->randn(ntuple(m->Int(round(4^(4/n)))+rand(-1:2), n)...), 4)
end



@testset "Mountains" begin
    @testset "Functions from utils.jl" begin
        @testset "neighborindices" for x in randarrays(), safe in (true, false)
            for i in CartesianIndices(x)
                for m in 1:ndims(x), n in 1:ndims(x)
                    above, below = (safe ? Mountains.neighborindices(i, x)[m]
                                         : Mountains.neighborindices(i)[m])
                    if m == n
                        @test above[n] == (safe ? max(i[n]-1, firstindex(x, n)) : i[n]-1)
                        @test below[n] == (safe ? min(i[n]+1, lastindex( x, n)) : i[n]+1)
                    else
                        @test above[n] == i[n]
                        @test below[n] == i[n]
                    end
                end
            end
        end

        @testset "interiorindices" for x in randarrays()
            interior = ntuple(n->firstindex(x, n)+1:lastindex(x, n)-1, ndims(x))
            @test CartesianIndices(interior) == Mountains.interiorindices(x)
        end
    end



    @testset "MountainRange class" begin
        @testset "Constructors" for r in randarrays(),
                                    T in (Float16, Float32, Mountains.defaultT)
            r = T.(r)
            h = randn(size(r))
            t = rand()
            @test MountainRange(r, h) == MountainRange(0, r, h)
            @test MountainRange(r) == MountainRange(r, zeros(size(r)))
            m = MountainRange(t, r, h)
            @test eltype(m) == eltype(m.h) == eltype(m.g) == eltype(m.r) == eltype(r)
            @test simtime(m) == T(t)
            @test m.r == r
            r[1] += 1
            @test m.r != r
            @test m.h == T.(h)
            h[1] += 1
            @test m.h != T.(h)
            @test m.g ≈ growthrate(m.h, m.r)
            m = MountainRange(T, size(r))
            @test simtime(m) == 0
            @test m.r == m.h == m.g == zeros(T, size(r)...)
            @test MountainRange(T, size(r)...) == m
            if T == Mountains.defaultT
                @test MountainRange(size(r)) == m
                @test MountainRange(size(r)...) == m
            end
        end

        @testset "AbstractArray interface" for r in randarrays()
            m = step!(MountainRange(r))
            @test size(m) == size(m.h)
            for i in eachindex(m)
                m[i] += rand()
                @test m[i] === m.h[i]
            end
        end

        @testset "Accessors" for r in randarrays()
            m = step!(MountainRange(r))
            @test simtime(m) == simtime(m)
            @test upliftrate(m) == m.r
            @test growthrate(m) == m.g
        end

        @testset "Comparison operators" begin
            m1 = step!(MountainRange(randn(5, 4)))
            m2 = deepcopy(m1)
            @test m1 == m2
            @test m1 ≈ m2
            for elem in (m2.t, (view(x, 2, 3) for x in (m2.r, m2.h, m2.g))...)
                orig = elem[]
                elem[] += 2*eps(Mountains.defaultT)
                @test m1 != m2
                @test m1 ≈ m2
                elem[] += 0.1
                @test !(m1 ≈ m2)
                elem[] = orig
            end
            @test MountainRange(2, 3) != MountainRange(3, 2)
            @test !(MountainRange(5, 6) ≈ MountainRange(6, 5))
        end
    end



    @testset "I/O" for r in randarrays(),
                       (T, I) in ((Float16, Int16), (Float32, UInt16),
                                  (Mountains.defaultT, Mountains.defaultI))
        t = rand(T)
        h = randn(T, size(r))
        inbuffer = IOBuffer()
        outbuffer = IOBuffer()
        write(inbuffer, I(ndims(r)))
        write(inbuffer, I.(size(r))...)
        write(inbuffer, t)
        seek(inbuffer, 0)
        @test_throws Mountains.MountainRangeReadException MountainRange(T, I, inbuffer)
        write(inbuffer, (Mountains.invertaxes(x) for x in (T.(r), h))...)
        seek(inbuffer, 0)
        m = (T == Mountains.defaultT ? MountainRange(inbuffer)
                                     : MountainRange(T, I, inbuffer))
        @test simtime(m) == t
        @test m.r == T.(r)
        @test m.h == h
        I == Mountains.defaultI ? write(outbuffer, m) : write(I, outbuffer, m)
        seek(inbuffer, 0)
        seek(outbuffer, 0)
        @test read(outbuffer) == read(inbuffer)
    end



    @testset "solve! and associated functions" begin
        @testset "steepness and dsteepness" for r in randarrays()
            m = step!(MountainRange(r))
            ∇h = [Mountains.gradient(m.h, i) for i in Mountains.interiorindices(m)]
            ∇g = [Mountains.gradient(m.g, i) for i in Mountains.interiorindices(m)]
            @test steepness(m) ≈ sum(x->sum(x.^2), ∇h)/(length(m)-2)
            @test dsteepness(m) ≈ 2*sum(x->sum(x[1].*x[2]), zip(∇h, ∇g))/(length(m)-2)
        end

        @testset "step!" for r in randarrays(), dt in Mountains.defaultdt.*(1, 0.1)
            m = MountainRange(r)
            I = Mountains.interiorindices(r)
            h = @. m.h+dt*m.g
            ∇²h = [begin
                L = -ndims(h)*h[i]
                for (above, below) in Mountains.neighborindices(i)
                    L += (h[below]+h[above])/2
                end
                L
            end for i in I]
            g = @. m.r[I]-h[I]^3+∇²h
            dt == Mountains.defaultdt ? step!(m) : step!(m, dt)
            @test m.g[I] ≈ g
            @test m.h ≈ h
        end

        @testset "solve!" for r in randarrays() # this will hang sometimes :/
            m = step!(MountainRange(r))
            solve!(m)
            @test simtime(m) > 0
            ds1 = dsteepness(m)
            ds2 = dsteepness(step!(m))
            @test ds1 ≤ 1.1eps(Mountains.defaultT) # The diffeq solver cuts it VERY close
            @test ds1+3abs(ds2-ds1) > eps(Mountains.defaultT) # heuristic
        end
    end
end