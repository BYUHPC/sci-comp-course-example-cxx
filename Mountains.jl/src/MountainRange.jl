export MountainRange, simtime, upliftrate, growthrate



"""
    MountainRange{T<:AbstractFloat, N} <: AbstractArray{T, N}

`N`-dimensional Mountain range with height determined by ``ḣ=r-h³+∇²h``.

Indexing a `MountainRange` returns its height at the given point.

See LINK for how the crude orogenesis simulation is carried out.

See also [`simtime`](@ref), [`upliftrate`](@ref), [`growthrate`](@ref), [`steepness`](@ref),
[`dsteepness`](@ref), [`step!`](@ref), and [`solve!`](@ref) for functions that operate on
`MountainRange`s.

# Examples

```jldoctest
julia> m = MountainRange([i+0.1j for i in 1:4, j in 1:5])
4×5 MountainRange{Float64, 2} with simulation time 0.0 and height: 4×5 Matrix{Float64}:
 0.0  0.0  0.0  0.0  0.0
 0.0  0.0  0.0  0.0  0.0
 0.0  0.0  0.0  0.0  0.0
 0.0  0.0  0.0  0.0  0.0

julia> step!(m)
4×5 MountainRange{Float64, 2} with simulation time 0.01 and height: 4×5 Matrix{Float64}:
 0.011  0.012  0.013  0.014  0.015
 0.021  0.022  0.023  0.024  0.025
 0.031  0.032  0.033  0.034  0.035
 0.041  0.042  0.043  0.044  0.045

julia> m[1, 1]
0.011000000000000001

julia> steepness(m)
6.32e-5
```
"""
struct MountainRange{T<:AbstractFloat, N} <: AbstractArray{T, N}
    t::Ref{T}      # simulation time; ref for immutability
    r::Array{T, N} # uplift rate
    h::Array{T, N} # height
    g::Array{T, N} # growth rate
end



# Constructors

"""
    MountainRange([t::Real=0,] r::AbstractArray{T, N}[, h=zeros(size(r))]) where {T, N}

Create a `MountainRange{T, N}` with simulation time `t`, uplift rate `r`, and height `h`.
"""
function MountainRange(t::Real, r::AbstractArray{T, N}, h=zeros(T, size(r))) where {T, N}
    return MountainRange{T, N}(Ref{T}(t), copy(r), copy(h), growthrate(h, r))
end

function MountainRange(r::AbstractArray{T, N}, h=zeros(T, size(r))) where {T, N}
    return MountainRange(zero(T), copy(r), copy(h))
end

"""
    MountainRange([T=$defaultT,] dims::Tuple)
    MountainRange([T=$defaultT,] dims::Integer...)

Create a `MountainRange{T}` of size `dims` with zero uplift rate and height.
"""
MountainRange(T::Type, dims::Tuple) = MountainRange(zeros(T, dims))

MountainRange(dims::Tuple) = MountainRange(defaultT, dims)

MountainRange(T::Type, dims::Integer...) = MountainRange(T, dims)

MountainRange(dims::Integer...) = MountainRange(dims)



# Comparison operators

function Base.:(==)(x::MountainRange, y::MountainRange)
    return simtime(x)==simtime(y) && x.r==y.r && x.h==y.h && x.g==y.g
end

function Base.:(≈)(x::MountainRange, y::MountainRange)
    return size(x)==size(y) && simtime(x)≈simtime(y) && x.r≈y.r && x.h≈y.h && x.g≈y.g
end



# AbstractArray interface

Base.size(m::MountainRange) = size(m.h)

Base.getindex( m::MountainRange, args...) = getindex( m.h, args...)

Base.setindex!(m::MountainRange, args...) = setindex!(m.h, args...)



# Accessors

"""
    simtime(m)

Return the current simulation time of a `MountainRange`.
"""
simtime(m::MountainRange) = m.t[]

"""
    upliftrate(m)

Return the array containing the uplift rate of a `MountainRange`.
"""
upliftrate(m::MountainRange) = m.r

"""
    growthrate(m)

Return the array containing the growth rate of a `MountainRange`.
"""
growthrate(m::MountainRange) = m.g