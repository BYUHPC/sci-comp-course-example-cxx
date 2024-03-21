"""
`Mountains` contains tools to simulate the crude orogenesis equation ``ḣ=r-h³+∇²h``.

For an overview of the associated assignments, justification of the equation, and
discretization methods, see https://github.com/BYUHPC/sci-comp-course-example-cxx.

The [`MountainRange`](@ref) type is used to store a mountain range's simulation time, uplift
rate, height, and growth rate. The simulation can be moved forward with [`step!`](@ref) and
[`solve!`](@ref). See [`MountainRange`](@ref) for a full list of associated functions.
"""
module Mountains

using Preferences



"""
    set_implementation(assignment)

Choose "basic" or "diffeq" version of `Mountains`; "basic" is the default.

# Examples

Set assignment to "diffeq" from the shell:

```bash
julia -e 'using Mountains; Mountains.set_implementation("diffeq")'
```
"""
function set_implementation(assignment::String)
    assignment in ("basic", "diffeq") || throw(ArgumentError(
            "Invalid assignment $assignment; options are basic and diffeq."))
    @set_preferences!("assignment" => assignment)
    @info("Assignment set to $assignment; restart Julia for the change to take effect.")
end

const assignment = @load_preference("assignment", "basic")



# Global constants

const defaultdt = 0.01 # default time step

const defaultT = Float64 # default MountainRange element type for I/O

const defaultI = UInt64 # default MountainRange integer type for I/O



# Include source files

include("MountainRange.jl")

include("utils.jl")

include("io.jl")

include("ruggedness.jl")

include("step.jl")

@static if assignment == "diffeq"
    include("solve_diffeq.jl")
else
    include("solve.jl")
end



end # module