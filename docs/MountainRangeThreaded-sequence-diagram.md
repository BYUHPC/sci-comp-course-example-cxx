# Mountain Range Threaded — Sequence Diagram

> [!IMPORTANT]
> This diagram relies on [Mermaid diagrams](https://mermaid.js.org/) which display properly when rendered within GitHub.
>
> It may not work properly when rendered within other websites. [Click here to view the source](https://github.com/BYUHPC/sci-comp-course-example-cxx/blob/main/docs/MountainRangeThreaded-sequence-diagram.md).

## Intro

This [sequence diagram](https://mermaid.js.org/syntax/sequenceDiagram.html#sequence-diagrams) written with Mermaid visually represents
the calls and work being performed in the `MountainRangeThreaded` example.

It is designed to help visualize the relationships between
the various entities involved in running the program. The close reader will observe stacked activation functions representing calls
to methods on the base or subclass of the `MountainRange` object.

The code covered by this diagram exists in three separate example files:
* [MountainRangeThreaded.hpp](../src/MountainRangeThreaded.hpp) (sub-class)
* [MountainRange.hpp](../src/MountainRange.hpp) (base class)
* [mountainsolve.cpp](../src/mountainsolve.cpp) (driver code)

## Diagram

```mermaid
---
title: Mountain Range Threaded — Sequence Diagram
config:
  mirrorActors: false
---

sequenceDiagram

participant main
participant MR as MountainRange

note left of main: Program starts
activate main

%% Construct MountainRange
note over main,MR: Construct MountainRange
main->>+MR: constructor()

    %% Create dw workers
    create participant dw as ds_workers (x8)
    MR->>+dw: spawn(8)
    dw->>dw: arrive_and_wait()

    %% Create sw workers
    create participant sw as step_workers (x8)
    MR->>+sw: spawn(8)
    sw->>sw: arrive_and_wait()

MR-->>-main: MountainRange
%% End construct MountainRange

%% Call Solve
note over main,MR: Begin Solving
main->>+MR: solve()

    %% Begin solve loop
    loop Until steepness < epsilon()

        %% Evaluate steepness
        MR->>+MR: dsteepness()
            MR--)dw: arrive_and_wait()
            note over dw: Worker threads proceed<br>to do work and <br>store result in <br>`ds_aggregator`
            MR<<-->>dw: arrive_and_wait()
        MR-->>-MR: total energy
        %% End steepness calculation

        %% Perform step
        MR->>+MR: step()
            MR--)sw: arrive_and_wait()
            note over sw: Worker threads proceed<br>to modify `h` cells
            MR<<-->>sw: arrive_and_wait()
            note over sw: Worker threads proceed<br>to modify `g` cells
            MR<<-->>sw: arrive_and_wait()
        MR-->>-MR: void
        %% End step

    end
    %% End solve loop

MR-->>-main: t
%% End solve

%% Call Write
note over main,MR: Write Result
main->>+MR: write()
MR-->>-main: void
%% End write

%% Destruct MountainRange
note over main,MR: Destruct MountainRange
main--x+MR: ~MountainRange()

    %% Destruct dw workers
    MR--xdw: arrive_and_wait()
    deactivate dw
    %%destroy dw
    note over dw: steepness threads<br>stop and rejoin

    %% Destruct sw workers
    MR--xsw: arrive_and_wait()
    deactivate sw
    %%destroy sw
    note over sw: stepping threads<br>stop and rejoin

MR-->>-main: void
destroy MR
%% End destruct MountainRange

note left of main: Program exits
deactivate main

```
