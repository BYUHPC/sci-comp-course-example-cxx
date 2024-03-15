"""
    neighborindices(i::CartesianIndex{N}[, x::AbstractArray]) where N

Return an `N`-tuple of 2-tuples of `CartesianIndex`es, each pair being the indices adjacent
to `i` along the corresponding axis.

If `N`-dimensional array `x` is provided, bounds checking will occur: if a neighbor index
would fall outside the bounds of the array, that neighbor will be replaced with `i`. This
can significantly hamper performance.

# Examples

```jldoctest
julia> neighborindices(CartesianIndex((3, 6)))
((CartesianIndex(2, 6), CartesianIndex(4, 6)), (CartesianIndex(3, 5), CartesianIndex(3, 7)))
julia> neighborindices(CartesianIndex((2, 5)), zeros(4, 5))
((CartesianIndex(1, 5), CartesianIndex(3, 5)), (CartesianIndex(2, 4), CartesianIndex(2, 5)))
```
"""
function neighborindices(i::CartesianIndex{N}) where N
    return ntuple(n->(i+CartesianIndex(ntuple(m->m==n ? offset : 0, N))
                      for offset in (-1, 1)), N)
end

function neighborindices(i::CartesianIndex{N}, x::AbstractArray{T, N}) where {T, N}
    return ntuple(n->(
        CartesianIndex(ntuple(m->m==n ? max(i[m]-1, firstindex(x, m)) : i[m], N)),
        CartesianIndex(ntuple(m->m==n ? min(i[m]+1, lastindex( x, m)) : i[m], N))
    ), N)
end



"""
    interiorindices(x)

Return a `CartesianIndices` of the non-edge cells of `x`.

# Examples

```jldoctest
julia> interiorindices(zeros(4,5,6))
CartesianIndices((2:3, 2:4, 2:5))
```
"""
function interiorindices(x::AbstractArray{<:Any, N}) where N
    return CartesianIndices(ntuple(n->firstindex(x, n)+1:lastindex(x, n)-1, N))
end



function laplacian(x::AbstractArray{<:Real, N}, i::CartesianIndex{N}) where N
    L = -N*x[i]
    for (above, below) in neighborindices(i)
        L += (x[above]+x[below])/2
    end
    return L
end



function gradient(x::AbstractArray{T, N}, i::CartesianIndex{N}) where {T, N}
    s = zero(T)
    for (above, below) in neighborindices(i)
        s += x[below]-x[above]
    end
    return s/2
end