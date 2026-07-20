# Quantum Compiler — Implementation Guide

> For Tarunpreet: everything built in sessions 1–4, explained so you can pick up right where we left off.

---

## How to build and run

```bash
g++ src/main.cpp src/lexer/lexer.cpp src/parser/parser.cpp \
    src/semantic/symbol_table.cpp src/semantic/type_checker.cpp \
    src/security/secure_checker.cpp \
    src/memory/memory_manager.cpp \
    src/codegen/codegen.cpp \
    -Isrc -std=c++17 -o qcc
./qcc examples/hello.q
./output
```

---

## The full pipeline

```
source.q → Lexer → tokens → Parser → AST → TypeChecker → SecureChecker → CodeGen → output.c → gcc → ./output
```

---

## Stage 1: Lexer

**Files:** `src/lexer/token_types.h`, `token.h`, `lexer.h`, `lexer.cpp`

The main loop in `tokenize()` skips whitespace and `//` comments, reads the current character, decides what token starts here, reads it, repeats until EOF.

Key methods: `peek()`, `peeknext()`, `advance()`, `readNumber()`, `readString()`, `readIdentOrKeyword()`.

**Bug fixed:** `peeknext()` was returning `source[pos]` instead of `source[pos + 1]` — broke float literals and every multi-char token (`->`, `..`, `==` etc).

`@Secure` becomes `AT` + `IDENT("Secure")` — `@` emitted as `AT` token.

`heap_alloc` is registered as `KW_HEAP_ALLOC` in the keywords map.

---

## Stage 2: Parser + AST

**Files:** `src/parser/ast.h`, `parser.h`, `parser.cpp`

**All AST nodes:**

| Node | What it represents |
|------|--------------------|
| `ProgramNode` | whole file |
| `FunctionDefNode` | `fn name(params) -> type { }`, has `isSecure` bool |
| `BlockNode` | `{ statements... }` |
| `VarDeclNode` | `let x: int = 5;`, has `resolvedType` and `heapElementType` |
| `ReturnStmtNode` | `return expr;` |
| `IfStmtNode` | `if (cond) { } else { }` |
| `ForStmtNode` | `for i in 0..10 { }` |
| `WhileStmtNode` | `while (cond) { }` |
| `AssignStmtNode` | `x = expr;` |
| `HeapAllocExprNode` | `heap_alloc(n)`, has `elementType` and `size` |
| `FreeStmtNode` | `free(varName)` |
| `IndexExprNode` | `arr[i]`, has `resolvedType` |
| `BinOpNode` | `a + b`, `x == y` etc |
| `IntLiteralNode` | `42` |
| `FloatLiteralNode` | `3.14` |
| `StringLiteralNode` | `"hello"` |
| `IdentifierNode` | variable name, has `resolvedType` |
| `FunctionCallNode` | `add(x, y)` |

**`parseTypeName()`** — handles `heap<T>`, returns the string `"heap<int>"` etc.

**`parseAssignOrCall()`** — default statement handler:
- `IDENT [` → index assignment (NOT YET IMPLEMENTED — deferred)
- `IDENT =` → plain assignment
- anything else → expression statement

**Expression precedence chain:**
```
parseExpression → parseComparison → parseAddSub → parseMulDiv → parsePrimary
```

---

## Stage 3: Type Checker

**Files:** `src/semantic/symbol_table.h`, `symbol_table.cpp`, `type_checker.h`, `type_checker.cpp`

**Symbol table:** stack of hash maps. `lookup()` searches from innermost scope outward, throws if not found. `SymbolInfo` has `{type, isFunction, paramTypes}`.

**Critical scoping fix:** `checkBlock(bool createScope = true)` — called with `false` from `checkFunction()` to avoid double-scoping the function body.

**How resolved types flow to codegen:** type checker writes `resolvedType` directly onto `IdentifierNode` and `VarDeclNode` during traversal. Codegen reads from the node — never queries the symbol table (scopes are gone by then).

**Key methods:**

| Method | What it does |
|--------|-------------|
| `checkVarDecl()` | validates type match; detects `heap<T>`, sets `heapElementType`, tracks heap var |
| `checkAssign()` | looks up var type, validates RHS matches |
| `checkFor()` | enters scope, defines loop var as `int` |
| `checkWhile()` | validates condition is `bool` or `int` |
| `checkFree()` | validates var is heap type, checks not already freed, marks freed |
| `checkIndex()` | checks use-after-free, extracts element type from `heap<T>` |
| `checkExpr()` | returns resolved type, sets `resolvedType` on identifier nodes, checks use-after-free |
| `checkBinOp()` | enforces type match; comparison ops return `"bool"` |
| `checkFunctionCall()` | validates callee exists, arg count, arg types |

---

## Stage 4: Memory Tracker

**Files:** `src/memory/memory_manager.h`, `memory_manager.cpp`

Tracks which variables are heap-allocated and which have been freed. Used by the type checker during AST traversal.

| Method | What it does |
|--------|-------------|
| `trackHeapVar(name)` | registers a variable as heap-allocated |
| `markFreed(name)` | marks a variable as freed |
| `isHeap(name)` | returns true if heap-allocated |
| `checkNotFreed(name, line)` | throws `Memory Error` if variable was already freed |

The type checker holds a `MemoryTracker memTracker` instance and calls these during `checkVarDecl`, `checkFree`, `checkIndex`, and `checkExpr` (identifier case).

---

## Stage 5: Security Checker

**Files:** `src/security/secure_checker.h`, `secure_checker.cpp`

Runs after type checker, before codegen. Only enforces rules inside `@Secure` functions.

Rules:
- Rejects `free()` calls — checks `FunctionCallNode.callee == "free"`
- Rejects `heap<T>` allocations — checks `node->type.substr(0,5) == "heap<"`

Error format: `@Secure violation at line N: <reason>`

---

## Stage 6: CodeGen

**Files:** `src/codegen/codegen.h`, `codegen.cpp`

**Gen methods:**

| Method | What it emits |
|--------|--------------|
| `genFunction()` | C function signature + body, sets `inSecureContext` |
| `genBlock()` | `{` ... `}` with indentation |
| `genVarDecl()` | stack: `int x = 5;` / heap: `int* arr = (int*)malloc(n * sizeof(int));` |
| `genReturn()` | `return expr;` |
| `genIf()` | `if (cond) { } else { }` |
| `genFor()` | `for (int i = start; i < end; i++)` |
| `genWhile()` | `while (cond)` |
| `genAssign()` | `x = expr;` |
| `genFree()` | `free(varName);` |
| `genExpr()` | recursive C expression; `^` → `pow()`; `arr[i]` → `varName[index]` |
| `buildPrintf()` | reads `resolvedType` from `IdentifierNode` for correct `%d`/`%f`/`%s` |
| `mapType()` | `int→int`, `float→double`, `string→char*`, `heap<T>→T*` |

`inSecureContext` bool — set in `genFunction()` based on `isSecure`. Heap allocs inside `@Secure` get a `// secure_region` comment in output.

Headers emitted: `stdio.h`, `stdlib.h` (for malloc/free), `math.h` (for pow).

---

## Known limitations / deferred to post-Phase 6

- `arr[i] = value` index assignment not implemented (`IndexAssignNode` missing)
- Full array support deferred
- Stack allocator (`stack_alloc`) not implemented
- No runtime encryption of `secure_region` memory
- Full ownership/borrow-checker style tracking deferred
- Structs and `impl` blocks not implemented

---

## Working test programs

**Functions + arithmetic:**
```
fn multiply(a: int, b: int) -> int {
    return a * b;
}
fn main() {
    let x: int = 6;
    let y: int = 7;
    let result: int = multiply(x, y);
    print(result);
}
```

**For loop:**
```
fn main() {
    for i in 0..5 { print(i); }
}
```

**While loop:**
```
fn main() {
    let x: int = 0;
    while (x < 5) { print(x); x = x + 1; }
}
```

**Mixed types:**
```
fn main() {
    let x: float = 3.14;
    let name: string = "Puranjay";
    print(x);
    print(name);
}
```

**Heap alloc:**
```
fn main() {
    let arr: heap<int> = heap_alloc(5);
    free(arr);
}
```

---

## File structure

```
src/
  main.cpp
  lexer/
    token_types.h
    token.h
    lexer.h
    lexer.cpp
  parser/
    ast.h
    parser.h
    parser.cpp
  semantic/
    symbol_table.h
    symbol_table.cpp
    type_checker.h
    type_checker.cpp
  security/
    secure_checker.h
    secure_checker.cpp
  memory/
    memory_manager.h
    memory_manager.cpp
  codegen/
    codegen.h
    codegen.cpp
examples/
  hello.q
```

---

## What to build next: Phase 6 — Goroutines + Channels + LLVM

**Step 1 — Goroutines via pthreads:**
- Add `GoStmtNode` to AST: `go funcName(args)`
- Parser: `parseGoStmt()` on `KW_GO` token
- Type checker: validate function exists and arg types match
- CodeGen: emit `pthread_create(...)`, track thread ids, `wait_all()` → `pthread_join` on all

**Step 2 — Channels:**
- `chan<int> numbers;` — typed channel declaration
- `numbers.send(x)` and `numbers.recv()`
- Implement as thread-safe queue in C (mutex + condition variable)

**Step 3 — LLVM backend:**
- Replace `system("gcc output.c ...")` with LLVM IR generation via LLVM C++ API
- Emit IR from AST nodes directly
- Hook up LLVM optimiser passes (O1–O3)
- Add `-lLLVM` to build command