# Mountain Range — Sequence Diagram

> [!IMPORTANT]
> This diagram relies on [Mermaid diagrams](https://mermaid.js.org/) which display properly when rendered within GitHub.
>
> It may not work properly when rendered within other websites. [Click here to view the source](https://github.com/BYUHPC/sci-comp-course-example-cxx/blob/main/docs/MountainRange-sequence-diagram.md).

## Intro

This [sequence diagram](https://mermaid.js.org/syntax/sequenceDiagram.html#sequence-diagrams) written with Mermaid visually represents the calls and work being performed in the `MountainRange` example.

It is designed to help visualize the new behaviors required in Phase 2 relating to file IO and checkpointing.
While this diagram merely illustrates the difference, the code may evolve to hold all the code in a single file, or separate the concerns into two separate files.
Compare against the [MountainRangeBasic sequence diagram](./MountainRangeBasic-sequence-diagram.md).

The code covered by this diagram exists in two separate example files:
* [MountainRange.hpp](../src/MountainRange.hpp) (base class)
* [mountainsolve.cpp](../src/mountainsolve.cpp) (driver code)

## Videos

- 🎥 [MountainRange — Sequence Diagram](https://www.loom.com/share/32bde27b238b4ae6a75966cc435b6c0d?sid=38cb3a9f-3478-4e73-9791-5b1e2a31339d)
- 🎥 [MountainRange — Code Walkthrough](https://www.loom.com/share/6542261b640e40efb2cf7f6be695f4b2?sid=71dbc02f-4c0b-46b0-be59-8685ce13cce0)

## Diagram

```mermaid
---
title: Mountain Range — Sequence Diagram
---

sequenceDiagram

participant main
participant MR as MountainRange
participant BIO as BinaryIO
participant cout as std::cout

note left of main: Program starts
activate main

%% Initialize MountainRange
note over main,MR: Construct MountainRange
main->>+MR: constructor(infile)

    %% Call protected instructor
    MR->>+MR: constructor(istream)
        %% Read from external file
        note right of MR: Execution order is determined by<br> variable declaration order, not <br> order presented in initialization.
        MR->>BIO: try_read_bytes() [ndims]
        MR->>BIO: try_read_bytes() [cells]
        MR->>BIO: try_read_bytes() [t]
        MR->>BIO: try_read_bytes() [r]
        MR->>BIO: try_read_bytes() [h]

        MR->>MR: step(0)
    MR-->>-MR: MountainRange
    %% End protected constructor

    %% Gracefully report errors
    break IO/filesystem error
        MR-->>MR: handle_read_failure(filename)
        MR--xcout: throw logic_error("Failed to read from " + filename)
    end
    %% End error handling

MR-->>-main: MountainRange
%% End construct MountainRange

%% Call Solve
note over main,MR: Begin Solving
main->>+MR: solve()

    %% Prepare for checkpointing
    MR->>MR: get_checkpoint_interval()

    %% Begin solve loop
    loop While dsteepness() > epsilon()

        %% Evaluate steepness
        MR->>MR: dsteepness()
        note right of MR: Calculate steepness,<br>same as before.

        %% Perform step
        MR->>MR: step()
        note right of MR: Advance simulation,<br>same as before.

        %% Optionally checkpoint
        opt should_perform_checkpoint()
            %% Write the current MountainRange state to a file
            MR->>MR: write(checkpoint_file_name)
            note right of MR: Save to file, <br> same as below.
        end
        %% End checkpoint

    end
    %% End solve loop

MR-->>-main: t
%% End solve

%% Print result
main->>cout: "solved. simulation time: " << t << std::endl
%% End print

%% Call Write
note over main,MR: Write Result


%% Write the final MountainRange state to a file
main->>+MR: write(outfile)
    MR->>BIO: try_write_bytes() [ndims, cells, t]
    MR->>BIO: try_write_bytes() [r]
    MR->>BIO: try_write_bytes() [h]

    %% Gracefully report errors
    break IO/filesystem error
        MR-->>MR: handle_write_failure(filename)
        MR--xcout: throw logic_error("Failed to write to " + filename)
    end
    %% End error handling
MR-->>-main: void
main->>cout: "Successfully wrote " << outfile
%% End write

note left of main: Program exits
deactivate main
```
