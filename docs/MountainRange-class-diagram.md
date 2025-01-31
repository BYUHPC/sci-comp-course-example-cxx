# Mountain Range — Class Diagram

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

class MountainRangeSimplified["MountainRange (Simplified)"]
class MountainRangeSimplified {
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
    - nt comm_size$

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

%% Relationships
MountainRangeSimplified --> MountainRange
MountainRange <|-- MountainRangeThreaded
MountainRange <|-- MountainRangeGPU
MountainRange <|-- MountainRangeMPI
```
