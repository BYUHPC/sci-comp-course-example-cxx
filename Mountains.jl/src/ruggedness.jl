export steepness, dsteepness, jaggedness, djaggedness, ruggedness, druggedness



"""
    gradientdotsum(x::AbstractArray, y::AbstractArray)

Return `∑ᵢ(∇xᵢ⋅∇yᵢ)`, where `i` are each interior index of `x` and `y`.
"""
function gradientdotsum(x::AbstractArray{T, N}, y::AbstractArray{T, N}) where {T, N}
    s = zero(T)
    @inbounds @fastmath @simd for i in interiorindices(x)
        s += gradient(x, i) * gradient(y, i)
    end
    return s
end



"""
    laplaciandotsum(x::AbstractArray, y::AbstractArray)

return `∑ᵢ(∇²xᵢ⋅∇²yᵢ)`, where `i` are each interior index of `x` and `y`
"""
function laplaciandotsum(x::AbstractArray{T, N}, y::AbstractArray{T, N}) where {T, N}
    s = zero(T)
    @inbounds @fastmath @simd for i in interiorindices(x)
        s += laplacian(x, i) * laplacian(y, i)
    end
    return s
end



"""
    steepness(m::MountainRange)

Return the steepness of `m`.

Steepness is defined as `∫ₘ‖∇h‖²dx/X`, where `X` is `m`'s length and `h` is its height.
"""
steepness(h::AbstractArray) = gradientdotsum(h, h)/(length(h)-2)

steepness(m::MountainRange) = steepness(m.h)



"""
    dsteepness(m::MountainRange)

Return the derivative of the [`steepness`](@ref) of `m`.

Steepness's derivative is `2∫ₘ(∇ḣ⋅∇h)dx/X`, where `X` is `m`'s length and `h` its height.
"""
dsteepness(h::AbstractArray, g::AbstractArray) = 2*gradientdotsum(h, g)/(length(h)-2)

dsteepness(m::MountainRange) = dsteepness(m.h, m.g)




"""
    jaggedness(m::MountainRange)

Return the jaggedness of `m`.

Jaggedness is defined as `∫ₘ‖∇²h‖²dx/X`, where `X` is `m`'s length and `h` is its height.
"""
jaggedness(m::AbstractArray) = laplaciandotsum(m, m)/(length(m)-2)



"""
    djaggedness(m::MountainRange)

Return the derivative of the [`jaggedness`](@ref) of `m`.

Jaggedness's derivative is `2∫ₘ(∇²ḣ⋅∇²h)dx/X`, where `X` is `m`'s length and `h` its height.
"""
djaggedness(h::AbstractArray, g::AbstractArray) = 2*laplaciandotsum(h, g)/(length(h)-2)

djaggedness(m::MountainRange) = djaggedness(m.h, m.g)



"""
    ruggedness(m::MountainRange)

Return `m`'s ruggedness (the sum of [`steepness`](@ref) and [`jaggedness`](@ref)).
"""
ruggedness(m::AbstractArray) = steepness(m) + jaggedness(m)



"""
    druggedness(m::MountainRange)

Return the derivative of `m`'s [`ruggedness`](@ref) (the sum of [`dsteepness`](@ref) and
[`djaggedness`](@ref)).
"""
druggedness(h::AbstractArray, g::AbstractArray) = dsteepness(h, g) + djaggedness(h, g)

druggedness(m::MountainRange) = druggedness(m.h, m.g)