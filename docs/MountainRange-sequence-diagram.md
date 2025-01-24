# Mountain Range — Sequence Diagram

> [!IMPORTANT]
> This diagram relies on [Mermaid diagrams](https://mermaid.js.org/) which display properly when rendered within GitHub.
>
> It may not work properly when rendered within other websites. [Click here to view the source](https://github.com/BYUHPC/sci-comp-course-example-cxx/blob/main/docs/MountainRange-sequence-diagram.md).

## Intro

This [sequence diagram](https://mermaid.js.org/syntax/sequenceDiagram.html#sequence-diagrams) written with Mermaid visually represents the calls and work being performed in the `MountainRange` example.

It is designed to help visualize the new behaviors required in Phase 2 relating to file IO and checkpointing.
While this diagram merely illustrates the difference, the code may evolve to hold all the code in a single file, or separate the concerns into two separate files.
Compare against the [MountainRange (simplified) sequence diagram](./MountainRange-simplified-sequence-diagram.md).

The code covered by this diagram exists in two separate example files:
* [MountainRange.hpp](../src/MountainRange.hpp) (base class)
* [mountainsolve.cpp](../src/mountainsolve.cpp) (driver code)

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
    %% Read from external file
    MR->>BIO: try_read_bytes() [ndims]
    MR->>BIO: try_read_bytes() [cells]
    MR->>BIO: try_read_bytes() [t]
    MR->>BIO: try_read_bytes() [r]
    MR->>BIO: try_read_bytes() [h]

    MR->>MR: step(0)
MR-->>-main: MountainRange
%% End construct MountainRange

%% Call Solve
note over main,MR: Begin Solving
main->>+MR: solve()
    %% Begin solve loop
    MR->>MR: get_checkpoint_interval()
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
            MR->>+MR: write(checkpoint_file_name)
                MR->>BIO: try_write_bytes() [ndims, cells, t]
                MR->>BIO: try_write_bytes() [r]
                MR->>BIO: try_write_bytes() [h]
            MR-->>-MR: void
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
MR-->>-main: void
main->>cout: "Successfully wrote " << outfile
%% End write

note left of main: Program exits
deactivate main
```
