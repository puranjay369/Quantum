# 🌌 **Project Quantum — Language Specification & Vision Document**

**Authors: Puranjay & Tarunpreet**

---

## 1. **Introduction**

**Quantum** is a **modern systems programming language** designed to merge **speed, safety, and concurrency** — while giving developers the power to **choose between performance and security** on demand.

Unlike most languages that enforce a single paradigm (either full safety like Rust, or full control like C++), Quantum introduces a **dual-mode execution philosophy**:

- **QF (Quantum Fast)** → For performance-critical, low-level code.
- **QS (Quantum Safe)** → For secure, memory-checked, and isolation-required sections.

Quantum aims to make systems programming **as fast as C**, **as safe as Rust**, and **as scalable as Go**, within a unified framework.

---

## 2.  **Core Philosophy**

Quantum is built around 4 principles:

| Principle | Meaning |
| --- | --- |
| **Control** | The developer decides how the program runs — fast or safe. |
| **Simplicity** | Syntax and structure should remain clean and predictable. |
| **Performance** | The compiler targets native code through LLVM, no VM or GC. |
| **Isolation** | Security is opt-in, meaning only sensitive code gets the overhead. |

> Tagline: "Speed where you can, safety where you must."

---

## 3.  **Execution Modes**

Quantum introduces **dual compilation modes** for flexible trade-offs:

### 🔹 Quantum Fast (QF)

- **No safety checks** — full manual control.
- Direct access to raw memory.
- Ideal for microservices, graphics, and high-performance computing.

### 🔸 Quantum Safe (QS)

- **Compile-time safety enforcement.**
- Memory ownership and bounds checking.
- Isolated runtime environment (no direct pointer mutation).
- Suitable for backend, cryptography, and system-critical code.

Developers can **mix both modes in a single program**:

```quantum
@Secure
fn processUserData(data: string) -> string {
    // Runs in Quantum Safe mode
    return sanitize(data);
}

fn main() {
    let result = processUserData("input");
    compute_fast(result); // Runs in Quantum Fast mode
}
```

---

## 4.  **Language Syntax Overview**

Quantum's syntax resembles Rust + Go hybrid, designed to be simple yet expressive.

### Variables and Functions

```quantum
let x: int = 5;

fn add(a: int, b: int) -> int {
    return a + b;
}
```

### Conditionals

```quantum
if (x > 5) {
    print("Greater");
} else {
    print("Lesser");
}
```

### Loops

```quantum
// for loop 
for i in 0..10 {
    print(i);
}
// do while 
// while 
```

### Structs and Methods

```quantum
struct Point {
    x: float,
    y: float
}

impl Point {
    fn distance(self, other: Point) -> float {
        return sqrt((self.x - other.x)^2 + (self.y - other.y)^2);
    }
}
```

### Concurrency (Go-style)

```quantum
fn task(id: int) {
    print("Task", id);
}

fn main() {
    go task(1);
    go task(2);
    wait_all();  // waits for all goroutines
}
```

---

## 5.  **Compiler Architecture**

The Quantum compiler (`qcc`) is built using **C++ + LLVM**.

It follows a **multi-phase pipeline**:

```
Source (.q)
   ↓
Lexer
   ↓
Parser → AST
   ↓
Semantic Analyzer (Type + Scope)
   ↓
Security Checker (@Secure mode)
   ↓
LLVM IR Generator
   ↓
Optimizer
   ↓
Binary Output (.out / .exe)
```

###  Compiler Modules

| Module | Role |
| --- | --- |
| **Lexer** | Converts source code into tokens. |
| **Parser** | Builds an Abstract Syntax Tree (AST). |
| **Semantic Analyzer** | Validates types, variable scopes, and function calls. |
| **Security Checker** | Enforces rules for `@Secure` mode. |
| **CodeGen (LLVM)** | Translates AST → LLVM IR → native code. |
| **Optimizer** | Applies LLVM passes (O1–O3). |
| **CLI Interface** | Manages build, run, and debug commands. |

---

## 6.  **Memory Model**

Quantum uses a **stack-first, minimal heap** memory model.

### Key Features:

- **Automatic stack management** for local variables.
- **Manual heap allocation** for dynamic structures.
- **Ownership tracking** in QS mode (like Rust's borrow checker, but optional).
- **No garbage collector** — Quantum aims for deterministic performance.

### Example:

```quantum
fn demo() {
    let a = 5;             // stack
    let arr = heap; // heap allocation
    arr[2] = 10;
    free(arr);              // manual release (only in QF mode)
}
```

### Security Hooks:

In QS mode, memory allocations are tagged as `secure_region`.

Future versions can encrypt or sandbox these regions at runtime.

---

## 7.  **Selective Security System (@Secure)**

Quantum's **@Secure** annotation introduces **compile-time verification** and **optional runtime isolation**.

### Compile-Time Rules

Inside `@Secure` functions:

- No pointer arithmetic.
- No unsafe casts.
- All arrays have bounds checks.
- Ownership enforcement (no dangling references).

### Future Runtime Rules

- Encrypted memory segments.
- Isolated thread pools.
- Dynamic sandboxing for critical code.

---

## 8.  **Concurrency Model**

Quantum uses **goroutine-like lightweight threads**:

- `go` keyword spawns a concurrent function.
- Communication via **typed channels**.
- Internal **cooperative scheduler** for task management.

### Example:

```quantum
chan<int> numbers;

fn producer() {
    for i in 1..5 {
        numbers.send(i);
    }
}

fn consumer() {
    while let val = numbers.recv() {
        print(val);
    }
}

fn main() {
    go producer();
    go consumer();
    wait_all();
}
```

---

## 9.  **Optimization System**

Quantum's optimizer is built on top of **LLVM's pass manager**.

### Optimization Levels

- **O0:** No optimization (debug mode)
- **O1:** Basic constant folding, DCE
- **O2:** Function inlining, CFG simplification
- **O3:** Aggressive loop unrolling, vectorization

Selective optimizations are applied:

- `@Secure` code disables unsafe optimizations.
- QF code uses full LLVM optimizations.

---

## 10.  **Runtime Library**

Quantum includes a minimal runtime for:

- Memory allocation (`stack_alloc`, `heap_alloc`)
- Concurrency primitives (goroutines, channels)
- IO and math support
- Secure memory API (future)

Organized as:

```
runtime/
├── memory/
├── concurrency/
├── security/
└── runtime.h
```

---

## 11.  **Standard Library**

The standard library (`stdlib/`) will be written in Quantum itself.

Modules planned:

- `io.q` — Input/output
- `math.q` — Math utilities
- `string.q` — String manipulation
- `system.q` — Environment access
- `concurrency.q` — Goroutines and channels

---

## 12.  **Toolchain**

Quantum ecosystem tools:

| Tool | Description |
| --- | --- |
| **qcc** | Quantum compiler (main binary). |
| **qpm** | Quantum Package Manager. |
| **qfmt** | Code formatter. |
| **qtest** | Test runner. |
| **LSP Server** | Editor integration (syntax highlight, autocomplete). |

---

## 13.  **Future Roadmap**

| Phase | Goal |
| --- | --- |
| **Phase 1** | Minimal compiler + basic syntax + LLVM IR output. |
| **Phase 2** | Memory model + ownership + runtime integration. |
| **Phase 3** | Implement @Secure + selective safety enforcement. |
| **Phase 4** | Add concurrency + channel-based communication. |
| **Phase 5** | Optimize performance, add standard library. |
| **Phase 6** | Cross-platform support + tooling ecosystem. |

---

## 14.  **Philosophical Summary**

Quantum isn't just a compiler — it's a statement:

> "Developers deserve control. Security should be a choice, not a prison."

It bridges the gap between **safety** and **freedom**, giving power back to the programmer — with a compiler intelligent enough to support both worlds.

---

## 15.  **Final Vision**

> Build a language that:
> 
> - Compiles to pure native machine code
> - Runs as fast as C++
> - Enforces safety when you ask for it
> - Concurrency built into its DNA
> - And a compiler that's simple, modular, and educational to build

Quantum is not about replacing C++ or Rust —

It's about **evolving** the way we think about performance and safety, not as opposites, but as **two sides of the same quantum state.**

---

##  **Project Structure**

```
Quantum/
│
├── README.md
├── LICENSE
├── Makefile                # For building the compiler & runtime
├── CMakeLists.txt          # Optional: for cross-platform builds
│
├── docs/                   # Documentation & design papers
│   ├── roadmap.md
│   ├── syntax_spec.md
│   ├── memory_model.md
│   ├── security_model.md
│   ├── concurrency_design.md
│   └── compiler_architecture.md
│
├── examples/               # Example .q programs
│   ├── hello.q
│   ├── fibonacci.q
│   ├── concurrency_test.q
│   ├── secure_memory.q
│   └── benchmark.q
│
├── src/                    # Source code for the compiler itself
│   │
│   ├── main.cpp            # Entry point for the compiler
│   │
│   ├── lexer/              # Lexical analysis
│   │   ├── lexer.cpp
│   │   ├── lexer.h
│   │   ├── token.h
│   │   └── token_types.h
│   │
│   ├── parser/             # Parser + AST
│   │   ├── parser.cpp
│   │   ├── parser.h
│   │   ├── ast.cpp
│   │   └── ast.h
│   │
│   ├── semantic/           # Type checker & semantic analysis
│   │   ├── type_checker.cpp
│   │   ├── type_checker.h
│   │   ├── symbol_table.cpp
│   │   └── symbol_table.h
│   │
│   ├── codegen/            # Code generation (LLVM backend)
│   │   ├── codegen.cpp
│   │   ├── codegen.h
│   │   ├── llvm_bridge.cpp
│   │   └── llvm_bridge.h
│   │
│   ├── memory/             # Memory model (stack/heap, ownership tracking)
│   │   ├── memory_manager.cpp
│   │   ├── memory_manager.h
│   │   ├── ownership.cpp
│   │   └── ownership.h
│   │
│   ├── security/           # @Secure annotation & safe mode logic
│   │   ├── secure_checker.cpp
│   │   ├── secure_checker.h
│   │   ├── policy_engine.cpp
│   │   └── policy_engine.h
│   │
│   ├── concurrency/        # Go-style lightweight threading
│   │   ├── goroutine.cpp
│   │   ├── goroutine.h
│   │   ├── scheduler.cpp
│   │   └── scheduler.h
│   │
│   ├── optimizer/          # LLVM pass setup, speed/memory optimization
│   │   ├── optimizer.cpp
│   │   └── optimizer.h
│   │
│   ├── utils/              # Shared tools/utilities
│   │   ├── logger.cpp
│   │   ├── logger.h
│   │   ├── error.cpp
│   │   ├── error.h
│   │   └── config.h
│   │
│   └── cli/                # Compiler CLI interface
│       ├── cli.cpp
│       └── cli.h
│
├── runtime/                # Low-level runtime library (used by generated binaries)
│   ├── memory/
│   │   ├── heap_allocator.cpp
│   │   └── stack_allocator.cpp
│   ├── concurrency/
│   │   ├── thread_api.cpp
│   │   └── channel_api.cpp
│   ├── security/
│   │   ├── sandbox.cpp
│   │   └── memory_guard.cpp
│   └── runtime.h
│
├── stdlib/                 # Standard library for Quantum programs
│   ├── io.q
│   ├── math.q
│   ├── string.q
│   ├── system.q
│   └── concurrency.q
│
├── tests/                  # Unit tests for compiler + runtime
│   ├── lexer_tests.cpp
│   ├── parser_tests.cpp
│   ├── memory_tests.cpp
│   ├── secure_tests.cpp
│   ├── concurrency_tests.cpp
│   └── codegen_tests.cpp
│
└── tools/                  # Developer tools (future)
    ├── lsp/                # Language Server Protocol integration
    │   ├── lsp_server.cpp
    │   └── lsp_protocol.h
    ├── package_manager/
    │   ├── qp_init.cpp
    │   ├── qp_build.cpp
    │   └── qp_publish.cpp
    └── formatter/
        ├── formatter.cpp
        └── formatter.h
```

---

##  **Getting Started**

1. **Clone the repository:**
   ```bash
   git clone https://github.com/puranjay369/Quantum.git
   cd Quantum
   ```

2. **Build the compiler:**
   ```bash
   make all
   ```

3. **Run examples:**
   ```bash
   ./qcc examples/hello.q
   ./hello
   ```

4. **Run tests:**
   ```bash
   make test
   ```

---

##  **Contributing**

We welcome contributions! Please see our roadmap in `docs/roadmap.md` for current priorities.

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

---

##  **License**

This project is licensed under the terms specified in the `LICENSE` file.

---

##  **Contact**

- [**Puranjay**](https://github.com/puranjay369)
- [**Tarunpreet**](https://github.com/GodXSpell)

