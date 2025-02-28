# Mountain Range Basic — Sequence Diagram

> [!IMPORTANT]
> This diagram relies on [Mermaid diagrams](https://mermaid.js.org/) which display properly when rendered within GitHub.
>
> It may not work properly when rendered within other websites. [Click here to view the source](https://github.com/BYUHPC/sci-comp-course-example-cxx/blob/main/docs/MountainRangeBasic-sequence-diagram.md).

## Intro

This [sequence diagram](https://mermaid.js.org/syntax/sequenceDiagram.html#sequence-diagrams) written with Mermaid visually represents
the calls and work being performed in the `MountainRange` example.

It is designed to help visualize the relationships between
the various entities involved in running the program.
The close reader will observe stacked activation functions representing calls
to methods on the base or subclass of the `MountainRange` object.

The code covered by this diagram exists in two separate example files:
* [MountainRangeBasic.hpp](../src/MountainRangeBasic.hpp) (base class)
* [initial.cpp](../src/initial.cpp) (driver code)

<!-- I'm waiting to record these videos until we have finalized the form of this file -->
<!--
## Videos

- 🎥 [MountainRangeBasic — Sequence Diagram]()
- 🎥 [MountainRangeBasic — Code Walkthrough]()
-->

## Diagram

```mermaid
---
title: Mountain Range Basic — Sequence Diagram
---

sequenceDiagram

participant main
participant MR as MountainRangeBasic
participant cout as std::cout

note left of main: Program starts
activate main

%% Initialize MountainRange
note over main,MR: Construct MountainRange
note right of main: Init constants and<br>fill vectors with zeros.
main->>+MR: constructor()
    MR->>+MR: step(0)
    note right of MR: Initialize g
    MR-->>-MR: void
MR-->>-main: MountainRange
%% End construct MountainRange

%% Call Solve
note over main,MR: Begin Solving
main->>+MR: solve()

    %% Begin solve loop
    loop While dsteepness() > epsilon()

        %% Evaluate steepness
        MR->>+MR: dsteepness()
            loop for cell in interior cells
                MR->>MR: ds_cell(cell)
            end
        MR-->>-MR: total energy
        %% End steepness calculation

        %% Perform step
        MR->>+MR: step()

            %% Update h cells
            loop for cell in cells
                MR->>MR: update_h_cell(cell)
            end
            %% End update h cells

            %% Update g cells
            loop for cell in interior cells
                MR->>MR: update_g_cell(cell)
            end
            %% End update g cells

            %% Update other state variables
            note right of MR: Update other state variables

        MR-->>-MR: void
        %% End step

    end
    %% End solve loop

MR-->>-main: t
%% End solve

%% Print result
note over main,MR: Print Result
main->>cout: t << std::endl
%% End print

note left of main: Program exits
deactivate main
```
