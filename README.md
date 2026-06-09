# minicc — Mini C Compiler + Stack VM

A complete compiler pipeline for a C-like language, targeting a custom stack-based virtual machine. Written from scratch in C++17.

```
source.mc  →  Lexer  →  Parser  →  AST  →  CodeGen  →  Optimizer  →  VM  →  output
```

---

## Features

**Language supported:**
- Types: `int`, `char`, `void`
- Variables: local, global, arrays
- Control flow: `if/else`, `while`, `for`
- Functions: recursion, multiple parameters, return values
- Operators: arithmetic, comparison, logical, `++`/`--`, `+=`/`-=`
- Built-in: `print(expr)` for integers and strings
- Comments: `//` line and `/* */` block

**Compiler pipeline:**
- **Lexer** — hand-written, produces typed tokens with line/col info
- **Parser** — recursive-descent, builds a typed AST
- **Code Generator** — visitor-pattern walk of AST, two-pass call resolution
- **Peephole Optimizer** — 4 passes: constant folding, dead push/pop elimination, dead jump elimination, double-negation removal
- **VM** — register-based frame layout, heap for arrays, 35 opcodes

---

## Project Structure

```
minicc/
├── CMakeLists.txt
├── include/
│   ├── token.h          # Token types + Token struct
│   ├── lexer.h          # Lexer interface
│   ├── ast.h            # All AST node types + Visitor interface
│   ├── parser.h         # Parser interface
│   ├── symbol_table.h   # Scoped symbol table
│   ├── bytecode.h       # Opcodes, Instr, FuncMeta, BytecodeImage
│   ├── codegen.h        # Code generator (Visitor)
│   ├── optimizer.h      # Peephole optimizer
│   └── vm.h             # Virtual machine
├── src/
│   ├── main.cpp         # Compiler driver (minicc binary)
│   ├── vm_main.cpp      # Standalone VM info (minivm binary)
│   ├── lexer.cpp
│   ├── ast.cpp          # AST pretty-printer
│   ├── parser.cpp
│   ├── symbol_table.cpp
│   ├── codegen.cpp
│   ├── optimizer.cpp
│   └── vm.cpp
├── tests/
│   ├── test_lexer.cpp   # Unit tests for lexer
│   ├── test_parser.cpp  # Unit tests for parser
│   ├── test_codegen.cpp # Unit tests for code generator
│   ├── test_vm.cpp      # Unit tests for VM
│   └── test_e2e.cpp     # End-to-end compile+run tests (19 test cases)
└── examples/
    ├── hello.mc         # Hello World
    ├── fibonacci.mc     # Recursive Fibonacci
    ├── sorting.mc       # Bubble sort on arrays
    └── counter.mc       # Global state + multiple functions
```

---

## Build

**Requirements:** CMake 3.16+, C++17 compiler (GCC 9+ or Clang 10+)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
```

For a release (optimized) build:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
```

---

## Usage

```bash
# Compile and run a .mc source file
./build/minicc examples/fibonacci.mc

# Dump the token stream (Lexer output)
./build/minicc --dump-tokens examples/hello.mc

# Dump the AST (Parser output)
./build/minicc --dump-ast examples/hello.mc

# Dump the bytecode disassembly (CodeGen output)
./build/minicc --dump-asm examples/fibonacci.mc

# Run without the optimizer
./build/minicc --no-opt examples/hello.mc

# Trace VM execution (prints every instruction)
./build/minicc --trace examples/hello.mc
```

---

## Run Tests

```bash
cd build && ctest --output-on-failure
```

All 5 test suites (lexer, parser, codegen, vm, e2e) should pass.

---

## Example Programs

### Hello World
```c
void main() {
    print("Hello, World!");
}
```

### Recursive Fibonacci
```c
int fib(int n) {
    if (n <= 1) { return n; }
    return fib(n - 1) + fib(n - 2);
}
void main() {
    int i = 0;
    while (i <= 10) {
        print(fib(i));
        i++;
    }
}
```

### For loop with sum
```c
int main() {
    int sum = 0;
    for (int i = 1; i <= 100; i++) {
        sum += i;
    }
    return sum;   // returns 5050
}
```

---

## VM Architecture

The VM is a **stack machine** with a separate heap for arrays.

**Call convention:**
```
Stack frame layout (from fp upward):
  fp+0  return address
  fp+1  saved frame pointer
  fp+2  local count (debug)
  fp+3  local slot 0  ← parameters start here
  fp+4  local slot 1
  ...
```

**Key opcodes:**

| Opcode | Description |
|---|---|
| `PUSH n` | Push integer constant |
| `LOAD slot` | Load local variable |
| `STORE slot` | Store local variable |
| `GLOAD/GSTORE` | Global variable access |
| `ANEW n` | Allocate array of size n on heap |
| `ALOAD/ASTORE` | Array element read/write |
| `CALL addr` | Function call |
| `RET_VAL` | Return with value |
| `JMP/JZ/JNZ` | Unconditional/conditional jumps |
| `PRINT_INT/PRINT_STR` | Output |

---

## Optimizer Passes

The peephole optimizer runs in a fixpoint loop until no changes occur:

1. **Constant folding** — `PUSH 3, PUSH 4, ADD` → `PUSH 7`
2. **Dead push/pop** — `PUSH n, POP` → removed
3. **Dead jump** — `JMP next_instruction` → removed
4. **Double negation** — `NEG, NEG` → removed

After each removal pass, all jump targets are rewritten to account for the shifted instruction indices.

---

## Design Decisions

- **Visitor pattern** on AST — clean separation between parsing and code generation. The same AST supports the pretty-printer, codegen, and can support a type-checker pass later.
- **Two-pass call resolution** — function calls emit a placeholder CALL that is back-patched after all functions are compiled, enabling forward references without a separate symbol pass.
- **Heap-based arrays** — arrays are allocated on a separate heap (not the stack), which avoids the complexity of variable-size stack frames and mirrors how languages like C actually handle dynamic allocation.
- **HALT sentinel** in optimizer — removed instructions are marked with `HALT(operand=-1)` before a single compaction pass, which keeps the algorithm simple and avoids index invalidation during iteration.
