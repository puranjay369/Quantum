# Quantum Language — Project Proposal

**Authors:** Puranjay & Tarunpreet

---

## What is Quantum?

Quantum is a modern systems programming language that lets developers choose between performance and safety on a per-function basis — without switching languages or tools.

Most languages force a single tradeoff: Rust enforces safety everywhere, C++ gives you control everywhere. Quantum introduces a dual-mode execution model so you get both in one codebase.

---

## The core idea: dual execution modes

| Mode | Keyword / Annotation | What it means |
|------|----------------------|---------------|
| **Quantum Fast (QF)** | default | No safety checks, raw memory access, full LLVM optimisation |
| **Quantum Safe (QS)** | `@Secure` on a function | Compile-time bounds checks, ownership tracking, no pointer arithmetic |

Both modes compile to native machine code through LLVM. There is no VM, no garbage collector.

```quantum
@Secure
fn processUserData(data: string) -> string {
    return sanitize(data);   // safe mode: bounds checked, ownership enforced
}

fn main() {
    let result = processUserData("input");
    compute_fast(result);    // fast mode: no overhead
}
```

---

## Design principles

| Principle | What it means in practice |
|-----------|--------------------------|
| Control | The developer decides how each function runs |
| Simplicity | Syntax is Rust + Go hybrid — clean and predictable |
| Performance | Targets native code via LLVM, no VM or GC |
| Isolation | Safety is opt-in — only annotated code pays the overhead |

> Tagline: *"Speed where you can, safety where you must."*

---

## Language features

**Variables and functions**
```quantum
let x: int = 5;

fn add(a: int, b: int) -> int {
    return a + b;
}
```

**Structs and methods**
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

**Concurrency (Go-style goroutines)**
```quantum
chan<int> numbers;

fn producer() {
    for i in 1..5 { numbers.send(i); }
}

fn consumer() {
    while let val = numbers.recv() { print(val); }
}

fn main() {
    go producer();
    go consumer();
    wait_all();
}
```

---

## Compiler architecture

```
Source (.q)  →  Lexer  →  Parser / AST  →  Semantic Analyser
     →  Security Checker (@Secure)  →  LLVM IR  →  Optimiser  →  Binary
```

Built in C++ using LLVM as the backend. The compiler (`qcc`) is modular — each pipeline stage is an independent module.

---

## Memory model

- Stack-first: local variables live on the stack by default
- Manual heap: `heap` keyword allocates, `free()` releases (QF mode only)
- Ownership tracking in QS mode — similar to Rust's borrow checker but optional
- No garbage collector — deterministic performance

---

## Toolchain (planned)

| Tool | Purpose |
|------|---------|
| `qcc` | Compiler |
| `qpm` | Package manager |
| `qfmt` | Code formatter |
| `qtest` | Test runner |
| LSP server | Editor integration |

---

## Roadmap

| Phase | Goal | Status |
|-------|------|--------|
| 1 | Lexer + tokeniser | ✅ Complete |
| 2 | Parser + AST | ✅ Complete |
| 3 | C code emitter (working end-to-end compiler) | ✅ Complete |
| 4 | Type checker + semantic analysis | 🔲 Next |
| 5 | `@Secure` enforcement + memory model | 🔲 Planned |
| 6 | Concurrency + LLVM backend + standard library | 🔲 Planned |

---

## Current status

The compiler is end-to-end working. A `.q` file containing functions, variables, arithmetic, if/else, and `print()` calls compiles to a native binary and runs correctly. The pipeline is: source → lex → parse → emit C → gcc → binary.

---

## Why Quantum?

Quantum is not trying to replace C++ or Rust. It is exploring whether performance and safety can coexist as **two modes of the same language** rather than two separate tools — making systems programming more accessible without sacrificing power.