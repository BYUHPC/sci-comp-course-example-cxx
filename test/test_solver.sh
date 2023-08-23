#!/usr/bin/env bash

# Runs the supplied solver on the supplied infile and ensures its output matches with the supplied expected outfile

# Since this is only used for testing with CMake, there is no error handling, no help message, etc.

# Arguments:
# - first:                        path to the mountain_diff binary
# - second through third to last: the command (and optionally its arguments) to run the solver
# - second to last:               the input file
# - last:                         the expected output file

# Examples:
# test/test_solver.sh bld/mountain_diff bld/solver_thread samples/small-1D-in.dat samples/small-1D-out.dat
# test/test_solver.sh bld/mountain_diff mpirun -n 3 bld/solver_mpi samples/tiny-1D-in.dat samples/tiny-1D-out.dat

# Parse
mtn_diff="$1"
infile="${@:$#-1:1}"
expected="${@:$#:1}"

# Outfile
outfile="$(mktemp)"
trap 'rm "$outfile"' EXIT

# Run solver
"${@:2:$#-3}" "$infile" "$outfile"

# Make sure outfile and expected are similar enough
"$mtn_diff" "$expected" "$outfile"
