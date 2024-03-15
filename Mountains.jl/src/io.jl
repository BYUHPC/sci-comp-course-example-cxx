function Base.show(io::IO, mime::MIME"text/plain", m::MountainRange{T, N}) where {T, N}
    # Print type information and header
    sizestr = N > 1 ? join(string.(size(m)), "×") : string(length(m))*"-element"
    t = simtime(m)
    print(io, "$sizestr MountainRange{$T, $N} with simulation time $t and height: ")
    # Print height array
    show(io, mime, m.h)
end



"""
    MountainRangeReadException(msg::String, m::MountainRange)

Reading a `MountainRange` from an `IO` failed.

Contains members `msg::String` and `m::MountainRange`.

`m` is included for debugging purposes--it likely will not be in a valid state and should be
inspected but not used.

`m` will contain all the data that was successfully read in. Any fields that weren't
successfully read will be set to `typemax(T)`, where `T` is the element type or index type
of `m` depending on the field; the exception is the size, which defaults to `(one(I),)`. The
uplift rate, height, and growth rate will be `Vector{T}`s of length 1 if the size isn't read
in successfully.

# Examples

```jldoctest
julia> try
           io = IOBuffer()
           m = MountainRange(io)
       catch e
           println(e.msg)
           println(size(e.m))
       end
stream didn't contain enough data
(1,)
```
"""
struct MountainRangeReadException <: Exception
    msg::String
    m::MountainRange
end



"""
    invertaxes(x::AbstractArray)

Convert an array between C and Fortran ordering.

Equivalent to `transpose(A)` for 1- and 2-dimensional arrays.
"""
invertaxes(x::AbstractArray{T, N}) where {T, N} = permutedims(x, ntuple(n->N-n+1, N))



"""
    MountainRange([T=$defaultT, I=$defaultI,] io::IO; checkstreamlength=io isa IOStream)

Create a `MountainRange{T}` by reading binary from `io`.

The number of dimensions of the `MountainRange` is determined by reading the first element
of the binary stream.

Throws a `MountainRangeReadException` if the stream doesn't contain enough data, if the data
seems obviously corrupt, or if the stream contains more data than necessary and
`checkstreamlength` is `true`.

See also [`write`](@ref) for the binary format of a `MountainRange`.
"""
function MountainRange(T::Type, I::Type, io::IO; checkstreamlength=io isa IOStream)
    # Initialize all elements so that information can still be conveyed on failure
    N = 1
    dims = (one(I),)
    t = typemax(T)
    r, h = ntuple(n->[typemax(T)], 2)
    makerange() = MountainRange(t, (invertaxes(x) for x in (r, h))...)
    try
        # Read header
        N = read(io, I)
        if N < 1 || N > 16
            throw(MountainRangeReadException("dimension must be ∈[1, 16]; got $N", makerange()))
        end
        dims = ntuple(n->read(io, I), N)
        t = read(io, T)
        # Read body
        r, h = ntuple(n->[typemax(T) for i in CartesianIndices(reverse(dims))], 2)
        foreach(x->read!(io, x), (r, h))
        # Return the MountainRange on success, or throw an exception
        m = makerange()
        if checkstreamlength && !eof(io)
            throw(MountainRangeReadException("stream contains unread data", m))
        end
        return m
    catch e
        m = makerange()
        s = join(string.(size(m)), "×")
        if e isa EOFError
            throw(MountainRangeReadException("stream doesn't contain enough data", m))
        elseif (e isa OutOfMemoryError)
            throw(MountainRangeReadException("size requested ($s) won't fit in memory", m))
        elseif (e isa ErrorException && e.msg == "invalid Array size")
            throw(MountainRangeReadException("invalid size ($s) requested", m))
        else
            rethrow(e)
        end
    end
end

MountainRange(io::IO; kw...) = MountainRange(defaultT, defaultI, io; kw...)



"""
    write([I=$defaultI,] io, m::MountainRange{T, N})

Serialize a `MountainRange` to `io`.

The serialization format is as follows, with no separation between values:

1. `N` as an `I`
1. The size of `m` as `N` `I`s
1. The simulation time as a `T`
1. The uplift rate, `length(m)` `T`s in C array order
1. The height in the same manner
"""
function Base.write(I::Type{<:Integer}, io::IO, m::MountainRange)
    return (write(io, I(ndims(m))) +
            write(io, I.(size(m))...) +
            write(io, simtime(m)) +
            write(io, (invertaxes(x) for x in (m.r, m.h))...))
end

Base.write(io::IO, m::MountainRange) = write(defaultI, io, m)