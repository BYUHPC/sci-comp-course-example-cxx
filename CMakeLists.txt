# BOILERPLATE ##########################################################################################################

cmake_minimum_required(VERSION 3.19)
project(MountainRange CXX)

# Packages
find_package(OpenMP)
find_package(Threads)
find_package(MPI)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS FALSE)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Include everything in src, and binary_io.hpp
include_directories(src)
include_directories(simple-cxx-binary-io)

# Default to RelWithDebInfo build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose Release, Debug, or RelWithDebInfo" FORCE)
endif()





# BINARIES #############################################################################################################

# mountaindiff
add_executable(mountaindiff src/mountaindiff.cpp)

# initial
add_executable(initial src/initial.cpp)

# mountainsolve_serial
add_executable(mountainsolve_serial src/mountainsolve_openmp.cpp)

# mountainsolve_openmp
if(OpenMP_CXX_FOUND)
    add_executable(mountainsolve_openmp src/mountainsolve_openmp.cpp)
    target_link_libraries(mountainsolve_openmp OpenMP::OpenMP_CXX)
endif()

# mountainsolve_thread
if(Threads_FOUND)
    add_executable(mountainsolve_thread src/mountainsolve_thread.cpp)
    target_include_directories(mountainsolve_thread PRIVATE CoordinatedLoopingThreadpoolCXX)
    target_link_libraries(mountainsolve_thread Threads::Threads)
endif()

# mountainsolve_mpi
if(MPI_CXX_FOUND)
    add_executable(mountainsolve_mpi src/mountainsolve_mpi.cpp)
    target_link_libraries(mountainsolve_mpi MPI::MPI_CXX)
endif()

# mountainsolve_gpu
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL NVHPC)
    message("-- Found nvc++, will build mountainsolve_gpu")
    set(NVCXX_COMPUTE_CAPABILITY "60" CACHE STRING "nvc++ GPU compute capability")
    set(NVCXX_CPU_ARCHITECTURE "haswell" CACHE STRING "CPU architecture that nvc++ will compile for")
    add_executable(mountainsolve_gpu src/mountainsolve_gpu.cpp)
    target_compile_options(mountainsolve_gpu PRIVATE
                           "-tp=${NVCXX_CPU_ARCHITECTURE}" "-stdpar=gpu" "-gpu=sm_${NVCXX_COMPUTE_CAPABILITY}")
    target_link_options(   mountainsolve_gpu PRIVATE
                           "-tp=${NVCXX_CPU_ARCHITECTURE}" "-stdpar=gpu" "-gpu=sm_${NVCXX_COMPUTE_CAPABILITY}")
else()
    message("-- Did not find nvc++, won't build mountainsolve_gpu")
endif()





# TESTING ##############################################################################################################

include(CTest)
if(BUILD_TESTING)
    # Helpers
    set(TEST_SOLVER "${CMAKE_SOURCE_DIR}/test/test_solver.sh")
    set(MTN_DIFF "${CMAKE_CURRENT_BINARY_DIR}/mountaindiff")
    set(TESTING_INFILE "${CMAKE_SOURCE_DIR}/samples/tiny-1D-in.mr" CACHE STRING "input mountain range file for tests")
    set(TESTING_OUTFILE "${CMAKE_SOURCE_DIR}/samples/tiny-1D-out.mr" CACHE STRING "expected output file for tests")
    function(test_solver SOLVER_NAME TEST_NAME)
        add_test(NAME "${TEST_NAME}"
                 COMMAND bash "${TEST_SOLVER}" "${MTN_DIFF}" "${CMAKE_CURRENT_BINARY_DIR}/${SOLVER_NAME}"
                              "${TESTING_INFILE}" "${TESTING_OUTFILE}")
    endfunction()

    # mountaindiff
    add_test(NAME "mountaindiff accepts identical plates"
             COMMAND "${MTN_DIFF}" "${TESTING_INFILE}" "${TESTING_INFILE}")
    add_test(NAME "mountaindiff rejects plates with different times"
             COMMAND "${MTN_DIFF}" "${TESTING_INFILE}" "${TESTING_OUTFILE}")
    set_property(TEST "mountaindiff rejects plates with different times" PROPERTY WILL_FAIL TRUE)
    add_test(NAME "mountaindiff rejects plates with different sizes"
             COMMAND "${MTN_DIFF}" "${TESTING_INFILE}" "${CMAKE_SOURCE_DIR}/samples/small-1D-in.mr")
    set_property(TEST "mountaindiff rejects plates with different sizes" PROPERTY WILL_FAIL TRUE)

    # initial
    add_test(NAME "initial works" COMMAND initial)
    set_tests_properties("initial works" PROPERTIES PASS_REGULAR_EXPRESSION "1.69")

    # mountainsolve_serial
    test_solver(mountainsolve_serial "mountainsolve_serial works")

    # parallel program tests
    foreach(N 1 2 3 11) # 11 is to make sure that processes with no responsibility don't cause problems
        # mountainsolve_openmp
        if(OpenMP_CXX_FOUND)
            set(OPENMP_TEST_NAME "mountainsolve_openmp works with ${N} threads")
            test_solver(mountainsolve_openmp "${OPENMP_TEST_NAME}")
            set_property(TEST "${OPENMP_TEST_NAME}" PROPERTY ENVIRONMENT OMP_NUM_THREADS=${N})
        endif()

        # mountainsolve_thread
        if(Threads_FOUND)
            set(THREAD_TEST_NAME "mountainsolve_thread works with ${N} threads")
            test_solver(mountainsolve_thread "${THREAD_TEST_NAME}")
            set_property(TEST "${THREAD_TEST_NAME}" PROPERTY ENVIRONMENT SOLVER_NUM_THREADS=${N})
        endif()

        # mountainsolve_mpi
        if(MPI_CXX_FOUND)
            add_test(NAME "mountainsolve_mpi works with ${N} processes"
                     COMMAND bash "${TEST_SOLVER}" "${MTN_DIFF}" mpirun -n 3 "${CMAKE_CURRENT_BINARY_DIR}/mountainsolve_mpi"
                                  "${TESTING_INFILE}" "${TESTING_OUTFILE}")
        endif()
    endforeach()

    # mountainsolve_gpu
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL NVHPC)
        test_solver(mountainsolve_gpu "mountainsolve_gpu works")
    endif()
endif()
