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
    loop Until steepness < epsilon()
        
        %% Evaluate steepness
        MR->>+MR: dsteepness()
            MR->>+MR: index_range(h)
            note right of MR: Create a Range View<br> compatible with GPU which.
            MR-->>-MR: tuple<range_view_begin, range_view_end>

            note right of MR: Standard syntax `std::transform_reduce`<br> compiles to GPU specific instructions
            create participant GPU
            MR->>GPU: transform_reduce()

                note right of GPU: Compiled code copies `g` and `h`<br> into GPU specific memory.

                note right of GPU: total = 0

                create participant kernel as GPU Kernel (x1000 ish)
                GPU->kernel: init
                par GPU to kernel1
                GPU->>kernel: transform(1)
                note right of kernel: Each kernel processes<br>a single cell. <br>Effectively: `ds_cell(1)`
                kernel->>GPU: reduce(ans, total)
                and GPU to kernel2
                GPU->>kernel: transform(2)
                note right of kernel: Effectively: `ds_cell(2)`
                kernel->>GPU: reduce(ans, total)
                and GPU to kernelN
                GPU->>kernel: transform(N)
                note right of kernel: Effectively: `ds_cell(N)`
                kernel->>GPU: reduce(ans, total)
                end

                destroy kernel
                GPU-xkernel: deinit  

                note right of GPU: Compiled code releases <br>`g` and `h` in GPU memory.

            destroy GPU
            GPU-->>MR: value_type <br>[total accumulated from kernels]

        MR-->>-MR: total energy
        
        %% Perform step
        MR->>+MR: step()
        
        MR-->>-MR: void
    
    end
MR-->>-main: t
%% End solve

%% Call Write
note over main,MR: Write Result
main->>+MR: write()
MR-->>-main: void
%% end write

note left of main: Program exits
deactivate main
```
