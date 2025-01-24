# Mountain Range GPU — Sequence Diagram

> [!IMPORTANT]
> This diagram relies on [Mermaid diagrams](https://mermaid.js.org/) which display properly when rendered within GitHub.
>
> It may not work properly when rendered within other websites. [Click here to view the source](https://github.com/BYUHPC/sci-comp-course-example-cxx/blob/main/docs/MountainRangeGPU-sequence-diagram.md).

## Intro

This [sequence diagram](https://mermaid.js.org/syntax/sequenceDiagram.html#sequence-diagrams) written with Mermaid visually represents
the calls and work being performed in the `MountainRangeGPU` example.

It is designed to help visualize the relationships between
the various entities involved in running the program. The close reader will observe stacked activation functions representing calls
to methods on the base or subclass of the `MountainRange` object.

The code covered by this diagram exists in three separate example files:
* [MountainRangeGPU.hpp](../src/MountainRangeGPU.hpp) (sub-class)
* [MountainRange.hpp](../src/MountainRange.hpp) (base class)
* [mountainsolve.cpp](../src/mountainsolve.cpp) (driver code)

## Diagram

```mermaid
---
title: Mountain Range GPU — Sequence Diagram
---

sequenceDiagram

participant main
participant MR as MountainRange
participant GPU
participant kernel as GPU Kernel (x1000 ish)

note over GPU, kernel: GPU Unit has its own specialized memory <br>which enables the fast execution from many<br> kernels concurrently.<br><br>Data must be copied back and forth<br>from main memory, and other limitations<br> apply because of the limited memory model.

note left of main: Program starts
activate main

%% Construct MountainRange
note over main,MR: Construct MountainRange
main->>+MR: constructor()
    note right of MR: Inherited construtor.<br>Read in data from file.
MR-->>-main: MountainRange
%% End construct MountainRange

%% Call Solve
note over main,MR: Begin Solving
main->>+MR: solve()

    %% Begin solve loop
    loop Until steepness < epsilon()

        %% Evaluate steepness
        MR->>+MR: dsteepness()
            MR->>+MR: index_range(h)
            note right of MR: Create a Range View<br> compatible with GPU.
            MR-->>-MR: tuple<itr_begin, itr_end>

            %% Sum steepness from each cell
            note right of MR: Standard syntax `std::transform_reduce`<br> compiles to GPU specific instructions.
            MR->>+GPU: std::transform_reduce()

                note right of GPU: Compiled code copies `g` and `h`<br> into GPU specific memory.
                note right of GPU: total = 0
                GPU->>+kernel: init

                %% Execute instructions in parallel
                par to kernel1
                GPU->>kernel: Effectively: `ds_cell(1)`
                kernel->>GPU: reduce(ans, total)
                and to kernel2
                GPU->>kernel: Effectively: `ds_cell(2)`
                kernel->>GPU: reduce(ans, total)
                and to kernelN
                GPU->>kernel: Effectively: `ds_cell(N)`
                kernel->>GPU: reduce(ans, total)
                end
                %% End parallel execution

                kernel-->>-GPU: deinit
                note right of GPU: Compiled code releases <br>`g` and `h` in GPU memory.

            GPU-->>-MR: value_type <br>[total accumulated from kernels]
            %% End steepness summation

        MR-->>-MR: total energy
        %% End steepness evaluation

        %% Perform step
        MR->>+MR: step()

            MR->>+MR: index_range(h)
            MR-->>-MR: tuple<itr_begin, itr_end>

            note right of MR: Standard syntax `std::for_each`<br> compiles to GPU specific instructions.

            %% Update h cells
            MR->>+GPU: std::for_each()

                note right of GPU: Copy `g` and `h` into GPU memory.
                GPU->>+kernel: init

                %% Execute instructions in parallel
                par to kernel1
                GPU->>kernel: Effectively: `update_h_cell(1)`
                and to kernel2
                GPU->>kernel: Effectively: `update_h_cell(2)`
                and to kernelN
                GPU->>kernel: Effectively: `update_h_cell(N)`
                end
                %% End parallel execution

                kernel-->>-GPU: deinit
                note right of GPU: Compiled code copies modified <br>`g` and `h` from GPU memory <br>back to main memory.

            GPU-->>-MR: void
            %% End update h cells

            %% Update g cells
            MR->>+GPU: std::for_each()

                note right of GPU: Copy `g` and `h` into GPU memory.
                GPU->>+kernel: init

                %% Execute instructions in parallel
                par to kernel1
                GPU->>kernel: Effectively: `update_g_cell(1)`
                and to kernel2
                GPU->>kernel: Effectively: `update_g_cell(2)`
                and to kernelN
                GPU->>kernel: Effectively: `update_g_cell(N)`
                end
                %% End parallel execution

                kernel-->>-GPU: deinit
                note right of GPU: Copy modified `g` and `h` to main.

            GPU-->>-MR: void
            %% End update g cells

        MR-->>-MR: void
        %% End step

    end
    %% End solve loop

MR-->>-main: t
%% End solve

%% Call Write
note over main,MR: Write Result
main->>+MR: write()
note right of MR: Inherited method.<br>Write result to file.
MR-->>-main: void
%% End write

note left of main: Program exits
deactivate main
```
