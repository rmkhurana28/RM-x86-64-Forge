# RM-x86_64-Forge

A from-scratch x86-64 compiler backend that takes Three-Address Code as input and emits real, runnable Intel assembly. Implements Chaitin's Graph Coloring Register Allocator with iterative spill rewriting, escape analysis, and full System V AMD64 ABI compliance.

**Developer:** Ridham Khurana  
**Language:** C++17  
**Target ISA:** x86-64 (Intel syntax)  

---

## 🏗️ Pipeline Architecture

```
┌──────────────┐     ┌─────────────────────┐     ┌───────────────┐
│  TAC Parser  │ ──▶ │ Instruction Select.  │ ──▶ │ CFG Builder   │
│  (3-Address)  │     │  (TAC → 2AC)        │     │ (Basic Blocks) │
└──────────────┘     └─────────────────────┘     └──────┬────────┘
                                                        │
                     ┌─────────────────────┐            ▼
                     │  Dead Code Elim.     │ ◀── Liveness Analysis
                     │  (Iterative DCE)     │     (USE/DEF/IN/OUT)
                     └──────────┬──────────┘
                                │
                                ▼
                     ┌─────────────────────┐     ┌───────────────────┐
                     │ Interference Graph   │ ──▶ │ Graph Coloring    │
                     │ (Adjacency Lists)    │     │ (Chaitin + Spill) │
                     └─────────────────────┘     └──────┬────────────┘
                                                        │
                     ┌─────────────────────┐            ▼
                     │  x86-64 Assembly     │ ◀── Register Mapping
                     │  (.intel_syntax)     │     & Stack Layout
                     └─────────────────────┘
```

Each stage writes its output to a numbered file under `output/`, so you can trace the entire compilation from IR to machine code.

---

## ⚙️ Pipeline Stages

1. TAC Parsing → Instruction Selection (3AC → 2AC)
2. CFG Construction → Iterative Liveness Analysis (fixed-point)
3. Dead Code Elimination (cascading)
4. Interference Graph Construction
5. Chaitin's Graph Coloring (Kempe simplification, k=14)
6. Spill Rewrite → Re-run from Stage 2 until convergence
7. x86-64 Assembly Emission (`.intel_syntax noprefix`, SysV AMD64 ABI)

---

## 🔧 Key Technical Details

### Register Allocation
- Chaitin's Graph Coloring with 14 physical x86-64 registers
- Kempe's theorem for simplification (degree < K removal)
- Iterative spill rewrite loop with convergence guarantees
- `_helper_` variable generation for spilled operands
- Spill candidates are selected by highest degree, with `_helper_` variables excluded to prevent infinite loops

### Escape Analysis
- Pre-pass scans the TAC for `&` (address-of) usage
- Any variable whose address is taken is permanently banned from register allocation and forced to a stack slot
- `LEA` instructions are directly rewritten to use the stack address — no helper indirection

### Memory Operand Handling
- Spill rewriting correctly identifies variables inside bracket expressions (e.g., `[arr + idx*8]`)
- Uses `std::regex` with word boundaries (`\b`) to prevent substring corruption (e.g., matching `a` inside `rax`)
- Handles edge cases where both operands of an instruction reference the same spilled variable

### Calling Convention (System V AMD64 ABI)
- First 6 arguments passed in `rdi, rsi, rdx, rcx, r8, r9`
- Overflow arguments pushed to the stack (right-to-left)
- Caller cleans up stack after call returns
- Callee-saved registers (`rbx, r12-r15`) are preserved across function boundaries

---

## 📂 Project Structure

```
RM-x86_64-Forge/
├── src/
│   ├── main.cc                              # Compiler driver
│   ├── tac_parser/
│   │   ├── instruction.h / .cc              # TAC instruction representation
│   │   └── three_address_code_parser.h / .cc # TAC file parser
│   ├── tac_to_2ac/
│   │   ├── two_address_instruction.h / .cc  # 2AC instruction + memory helpers
│   │   └── converter.h / .cc               # TAC → 2AC lowering engine
│   ├── liveness_analysis/
│   │   ├── basic_block.h / .cc              # Basic block with USE/DEF/IN/OUT sets
│   │   ├── cfg.h / .cc                      # CFG, liveness, interference graph,
│   │   │                                    # graph coloring, spill rewrite, emission
│   └── test/
│       └── test.txt                         # Input TAC test case
├── output/                                  # Generated pipeline outputs (see below)
├── CMakeLists.txt
├── Makefile
└── README.md
```

---

## 📊 Output Files

Each compilation run produces 9 files under `output/`, one per pipeline stage:

| File | Contents |
|------|----------|
| `00_all_in_one.txt` | Concatenation of all files below |
| `01_3-addr_code.txt` | Parsed TAC with instruction types |
| `02_2-addr_code.txt` | Lowered 2-address instructions |
| `03_cfg.txt` | CFG blocks, edges, USE/DEF/IN/OUT sets |
| `04_optimized_2-addr_code.txt` | Post-DCE instruction stream |
| `05_interference_graph.txt` | Full adjacency list for every variable |
| `06_stack.txt` | Kempe simplification stack (pop order) |
| `07_mapping_and_spills.txt` | Final variable → register/stack mapping |
| `08_final_assembly.txt` | Emitted x86-64 Intel assembly |

---

## 📋 Supported TAC Instructions

| Type | Example |
|------|---------|
| Binary Operation | `a = b + c` &nbsp; `a = b * c` &nbsp; `a = b & c` &nbsp; `a = b << 2` |
| Unary Operation | `a = -b` &nbsp; `a = !b` &nbsp; `a = ~b` |
| Assignment | `a = b` |
| Conditional Goto | `if a < b goto L1` &nbsp; `ifFalse a goto L2` |
| Unconditional Goto | `goto L1` |
| Function Definition | `func foo:` |
| Function Call | `call foo, 2` &nbsp; `a = call foo, 4` |
| Parameter Passing | `param x` |
| Return | `return a` &nbsp; `return` |
| Array Load | `a = arr[i]` |
| Array Store | `arr[i] = a` |
| Address-Of | `ptr = &a` |
| Pointer Load | `a = *ptr` |
| Pointer Store | `*ptr = a` |
| Labels | `L1:` |

**Supported operators:** `+` `-` `*` `/` `%` `==` `!=` `<` `<=` `>` `>=` `&&` `||` `&` `|` `^` `<<` `>>` `!` `-` (unary) `~`

---

## 🛠️ Build & Run

```bash
# Build the project
make

# Build and run
make run

# Clean build artifacts
make clean
```

The input TAC is read from `src/test/test.txt`. Output is written to `output/`.  
Final assembly lands in `output/08_final_assembly.txt`.

---

## 💡 Example

**Input TAC:**
```text
func main:
a = 10
b = 20
ptr = &a
*ptr = b
res = *ptr
return res
```

**Output (x86-64 Intel Assembly):**
```asm
.intel_syntax noprefix
.global main

main:
    PUSH rbp
    MOV rbp, rsp
    SUB rsp, 16
    MOV rax, 10
    MOV [rbp - 8], rax
    MOV rax, 20
    LEA rcx, [rbp - 8]
    MOV [rcx], rax
    MOV rax, [rcx]    
    MOV rsp, rbp
    POP rbp
    ret
```

---

## 📝 License

**Educational Portfolio License**

Copyright © 2026 Ridham Khurana (RM)

This software is provided as a portfolio demonstration and educational resource.

**Permitted Uses:**
- View, clone, and run this code for evaluation and review purposes
- Study the implementation to learn compiler design concepts and techniques
- Test functionality and modify test cases locally for assessment
- Use for recruitment, hiring, interview evaluation, and skills assessment
- Reference specific implementations in technical discussions with proper attribution

**Prohibited Uses:**
- Incorporating this code into your own projects, products, or portfolio
- Submitting this code (modified or unmodified) as part of academic coursework
- Redistributing or publishing this code or derivative works
- Using this code for commercial purposes without explicit permission
- Presenting this work as your own or without proper attribution to the author

**Note:** This code is shared to demonstrate the author's technical capabilities
and to serve as an educational resource for those learning compiler design.
For any use case beyond evaluation and learning, please contact the author.

**Contact:** khurana.ridham222@gmail.com

---
