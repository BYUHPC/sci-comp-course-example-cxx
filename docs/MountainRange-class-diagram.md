# Mountain Range — Class Diagram

> [!IMPORTANT]
> This diagram relies on [Mermaid diagrams](https://mermaid.js.org/) which displays properly when rendered within GitHub.
>
> It may not work properly when rendered within other websites. [Click here to view the source](https://github.com/BYUHPC/sci-comp-course-example-cxx/blob/main/docs/MountainRange-class-diagram.md).

## Intro

This [class diagram](https://mermaid.js.org/syntax/classDiagram.html#class-diagrams) written with Mermaid visually represents
the relationships between classes which offer multiple computational approaches to the `MountainRange`.

It is designed to emphasize the role of **inheritance** in sharing/overriding code
from the base class as much as possible while sub-classes provide _only_ the new
functionality associated with its particular objective.

The code covered by this diagram exists in five separate example files:
* [initial.cpp](../src/initial.cpp) (driver code | initial)
* [MountainRangeBasic.hpp](../src/MountainRangeBasic.hpp) (simple class | initial)
* [mountainsolve.cpp](../src/mountainsolve.cpp) (driver code)
* [MountainRange.hpp](../src/MountainRange.hpp) (base class)
* [MountainRangeThreaded.hpp](../src/MountainRangeThreaded.hpp) (sub-class)
* [MountainRangeMPI.hpp](../src/MountainRangeMPI.hpp) (sub-class)
* [MountainRangeGPU.hpp](../src/MountainRangeGPU.hpp) (sub-class)

## Videos

- 🎥 [MountainRange — Class Diagram](https://www.loom.com/share/8280654f78754a16902ecd3c9b2c9128?sid=5c193f4c-ae6f-49bb-9c9f-914db2833183)
- 🎥 [MountainRange — Code Walkthrough](https://www.loom.com/share/2e5242942ca74d17b85ce05f97138e60?sid=3d91e827-d55a-44d4-bfc8-58adff1cbac5)

## Inheritance

Inheritance allows a base class to define common functionality that multiple specialized sub-classes can extend or override as needed. This approach reduces code duplication and makes it easier to experiment with different computational methods while maintaining a consistent interface.

See these resources to learn more about inheritance:
* [Inheritance in C++ | GeeksForGeeks](https://www.geeksforgeeks.org/inheritance-in-c/)
* [Inheritance in C++ | Simplilearn](https://www.simplilearn.com/tutorials/cpp-tutorial/types-of-inheritance-in-cpp)
* [Virtual functions | IBM](https://www.ibm.com/docs/en/zos/3.1.0?topic=only-virtual-functions-c)
* [C++ Virtual Functions and Function Overriding | Programiz](https://www.programiz.com/cpp-programming/virtual-functions)

## Legend

| Item | Meaning |
| :-----: | :------------- |
| `+` | **Public** visibility (access modifier) |
| `#` | **Protected** visibility (access modifier) |
| `-` | **Private** visibility (access modifier) |
| <ins>underline</ins> | `static` method/attribute |
| <i>italics</u> | `virtual` method that can be overwritten <br>_Note: This is a deviation from standard UML <br>which uses underline to signal `abstract`._ |
| `const` | Methods: Calling the function does not change the state of any attributes <br>Attributes: The value will never change after assignment |

## Diagram

```mermaid
---
title: Mountain Range — Class Diagram
---
classDiagram
direction LR

namespace Initial {
    class InitialMain["initial.cpp"]
    class MountainRangeBasic
}

class InitialMain {
    - size_t len
    - size_t plateau_start, plateau_end
    - vector~double~ r, h
    - MountainRangeBasic m
}


class MountainRangeBasic {
    %% Global type variables (readability + mutability)
    + size_t size_type
    + double value_type

    %% Data storage parameters
    # value_type default_dt$
    # size_t header_size$
    # size_type ndims
    # size_type cells
    # value_type t
    # vector~value_type~ r
    # vector~value_type~ h
    # vector~value_type~ g

    %% Getter methods
    +size() size_type const
    +sim_time() value_type const
    +uplift_rate() size_type const
    +height() size_type const

    %% Constructor
    + MountainRange(r, h)

    %% Solving
    # update_g_cell(i)
    # update_h_cell(i, dt)
    # ds_cell(i) const

    + dsteepness() value_type const*
    + step(dt) value_type*
    + solve(dt=default_dt) value_type
}

namespace WithIO {
    class MountainSolve["mountainsolve.cpp"]
    class MountainRange
}

class MountainSolve {
    + print(...)
    - help()
    - char* infile
    - char* outfile
    - MtnRange m
}

class MountainRange {
    %% Constructors
    # MountainRange(ndims, cells, t, r, h)
    # MountainRange(std::istream)
    + MountainRange(String filename)

    %% Error handling
    # handle_wrong_dimensions()$
    # handle_write_failure(String filename)$
    # handle_read_failure(String filename)$

    %% Output
    + write(String filename) void const*

    %% Solving
    + split_range(n, rank, size) std::range$
    - get_checkpoint_interval() value_type const
    - should_perform_checkpoint() bool const

    %% Edit implementation
    + solve(dt=default_dt) value_type
}

class MountainRangeThreaded {
    %% New internal bookkeeping variables
    - bool continue_iteration
    - size_type nthreads
    - std::barrier ds_barrier
    - std::barrier step_barrier
    - vector~std::jthread~ ds_workers
    - vector~std::jthread~ step_workers
    - atomic~value_type~ ds_aggregator

    %% Helper methods
    + looping_threadpool(thread_count, F)$
    - this_thread_cell_range(tid) std::range
    + std::string help_message$

    %% Constructor & Destructor
    + MountainRangeThreaded(...)
    + ~MountainRangeThreaded()

    %% Override methods
    + dsteepness() value_type const
    + step(dt) value_type
}

class MountainRangeGPU {
    + index_range(x) tuple~iter_begin, iter_end~$

    %% Constructor
    + MountainRangeGPU(...)

    %% Override methods
    + dsteepness() value_type const
    + step(dt) value_type
}

class MountainRangeMPI {
    %% TODO: I'm not sure what this returns
    + read_at_all(f, offset)$
    - this_process_cell_range() std::range

    %% Static variables
    %% These are unique to each worker since each is in a separate process
    - mpl::communicator comm_world$
    - int comm_rank$
    - int comm_size$

    %% Constructors
    - MountainRangeMPI(mpl::file)
    + MountainRangeMPI(String filename)

    %% Override
    + write(String filename) void const
    + dsteepness() value_type const
    + step(dt) value_type

    %% New methods
    - exchange_halos(x) void
}

%% Main classes
InitialMain : main()
MountainSolve : main()

%% Relationships

%%InitialMain ..> MountainRangeBasic
%%MountainSolve ..> MountainRange
InitialMain ..> MountainSolve : Replaced by

MountainRangeBasic --> MountainRange : Evolves into, OR<br>remains separate
MountainRange <|-- MountainRangeThreaded
MountainRange <|-- MountainRangeGPU
MountainRange <|-- MountainRangeMPI
```
