# Mountain Range Basic — Sequence Diagram

> [!IMPORTANT]
> This diagram relies on [Mermaid diagrams](https://mermaid.js.org/) which display properly when rendered within GitHub.
>
> It may not work properly when rendered within other websites. [Click here to view the source](https://github.com/BYUHPC/sci-comp-course-example-cxx/blob/main/docs/MountainRangeBasic-sequence-diagram.md).

## Intro

This [sequence diagram](https://mermaid.js.org/syntax/sequenceDiagram.html#sequence-diagrams) written with Mermaid visually represents the calls and work being performed in the `MountainRange` example.

It is designed to help visualize the relationships between the various entities involved in running the program.

The code covered by this diagram exists in two separate example files:
* [MountainRangeBasic.hpp](../src/MountainRangeBasic.hpp) (base class)
* [initial.cpp](../src/initial.cpp) (driver code)

## Diagram

```mermaid
---
title: Mountain Range — Sequence Diagram
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
    loop While dsteepness() > epsilon()
        
        %% Evaluate steepness
        MR->>+MR: dsteepness()
            loop for cell in interior cells
                MR->>MR: ds_cell(cell)
            end
        MR-->>-MR: total energy
        
        %% Perform step
        MR->>+MR: step()
            %% Modify h cells
            loop for cell in cells
                MR->>MR: update_h_cell(cell)
            end

            %% Modify g cells
            loop for cell in interior cells
                MR->>MR: update_g_cell(cell)
            end

            %% Update other state variables
            note right of MR: Update other state variables
        MR-->>-MR: void
    
    end
MR-->>-main: t
%% End solve

%% Print result
note over main,MR: Print Result
main->>cout: t << std::endl
%% end print

note left of main: Program exits
deactivate main
```
