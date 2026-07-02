# RM-x86_64-Forge

A custom x86_64 Compiler Backend and Register Allocator.

## Pipeline
1. Instruction Selection (3-Address to 2-Address)
2. Liveness Analysis (CFG & Data Flow)
3. Interference Graph Construction
4. Graph Coloring & Register Allocation (Chaitin-Briggs)
5. Code Emission
