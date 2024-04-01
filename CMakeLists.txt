# BOILERPLATE ##########################################################################################################

cmake_minimum_required(VERSION 3.20)
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
set(COMMON_INCLUDES src/MountainRange.hpp simple-cxx-binary-io/binary_io.hpp)

# Default to RelWithDebInfo build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose Release, Debug, or RelWithDebInfo" FORCE)
endif()





# BINARIES #############################################################################################################

# mountaindiff
add_executable(mountaindiff src/mountaindiff.cpp ${COMMON_INCLUDES})

# initial
add_executable(initial src/initial.cpp ${COMMON_INCLUDES})

# mountainsolve_serial
add_executable(mountainsolve_serial src/mountainsolve.cpp ${COMMON_INCLUDES})
target_compile_definitions(mountainsolve_serial PUBLIC USE_OPENMP)

# mountainsolve_openmp
if(OpenMP_CXX_FOUND)
    add_executable(mountainsolve_openmp src/mountainsolve.cpp ${COMMON_INCLUDES})
    target_link_libraries(mountainsolve_openmp OpenMP::OpenMP_CXX)
    target_compile_definitions(mountainsolve_openmp PUBLIC USE_OPENMP)
endif()

# mountainsolve_thread
if(Threads_FOUND)
    add_executable(mountainsolve_thread src/mountainsolve.cpp src/MountainRangeThreaded.hpp ${COMMON_INCLUDES})
    target_include_directories(mountainsolve_thread PRIVATE CoordinatedLoopingThreadpoolCXX)
    target_link_libraries(mountainsolve_thread Threads::Threads)
    target_compile_definitions(mountainsolve_thread PUBLIC USE_THREAD)
endif()

# mountainsolve_mpi
if(MPI_CXX_FOUND)
    find_package(mpl REQUIRED)
    add_executable(mountainsolve_mpi src/mountainsolve.cpp src/MountainRangeMPI.hpp ${COMMON_INCLUDES})
    target_link_libraries(mountainsolve_mpi PRIVATE MPI::MPI_CXX)
    target_link_libraries(mountainsolve_mpi PRIVATE mpl::mpl)
    target_compile_definitions(mountainsolve_mpi PUBLIC USE_MPI)
endif()

# mountainsolve_gpu
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL NVHPC)
    message("-- Found nvc++, will build mountainsolve_gpu")
    set(NVCXX_COMPUTE_CAPABILITY "60" CACHE STRING "nvc++ GPU compute capability")
    add_executable(mountainsolve_gpu src/mountainsolve.cpp src/MountainRangeGPU.hpp ${COMMON_INCLUDES})
    set_target_properties(mountainsolve_gpu PROPERTIES COMPILE_FLAGS "-stdpar=gpu -gpu=sm_${NVCXX_COMPUTE_CAPABILITY}"
                                                       LINK_FLAGS    "-stdpar=gpu -gpu=sm_${NVCXX_COMPUTE_CAPABILITY}")
    target_compile_definitions(mountainsolve_gpu PUBLIC USE_GPU)
else()
    message("-- Did not find nvc++, won't build mountainsolve_gpu")
endif()





# TESTING ##############################################################################################################

include(CTest)
if(BUILD_TESTING)
    # Helpers
    set(TEST_SOLVER "${CMAKE_SOURCE_DIR}/test/test_solver.sh")
    set(MTN_DIFF "${CMAKE_CURRENT_BINARY_DIR}/mountaindiff")
    set(TESTING_INFILE "${CMAKE_SOURCE_DIR}/samples/1d-tiny-in.mr" CACHE STRING "input mountain range file for tests")
    set(TESTING_OUTFILE "${CMAKE_SOURCE_DIR}/samples/1d-tiny-out.mr" CACHE STRING "expected output file for tests")
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
             COMMAND "${MTN_DIFF}" "${TESTING_INFILE}" "${CMAKE_SOURCE_DIR}/samples/1d-small-in.mr")
    set_property(TEST "mountaindiff rejects plates with different sizes" PROPERTY WILL_FAIL TRUE)

    # initial
    add_test(NAME "initial works" COMMAND initial)
    set_tests_properties("initial works" PROPERTIES PASS_REGULAR_EXPRESSION "1.9")

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
                     COMMAND bash "${TEST_SOLVER}" "${MTN_DIFF}" mpirun -n "${N}" "${CMAKE_CURRENT_BINARY_DIR}/mountainsolve_mpi"
                                  "${TESTING_INFILE}" "${TESTING_OUTFILE}")
        endif()
    endforeach()

    # mountainsolve_gpu
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL NVHPC)
        test_solver(mountainsolve_gpu "mountainsolve_gpu works")
    endif()
endif()
