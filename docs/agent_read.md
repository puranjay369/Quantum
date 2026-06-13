# Quantum Compiler — AI Agent Context File

> Feed this file to any AI assistant at the start of a session. It summarises everything built so far so the AI does not need to read the entire codebase.

---

## Project identity

- **Language:** Quantum (.q files)
- **Compiler binary:** `qcc`
- **Built with:** C++17 + LLVM (LLVM not integrated yet)
- **Authors:** Puranjay & Tarunpreet
- **Repo:** https://github.com/puranjay369/Quantum

---

## What has been built (as of session 4)

### Phase 1 — Lexer ✅ COMPLETE

Files: `src/lexer/token_types.h`, `token.h`, `lexer.h`, `lexer.cpp`

Key behaviours:
- Handles `->` vs `-`, `..` vs `.`, `==` vs `=`, `!=`, `<=`, `>=`
- Line comments via `//`
- Tracks line + col for every token
- `@` emitted as `AT` token (for `@Secure`)
- Bug fixed: `peeknext()` was returning `source[pos]` instead of `source[pos + 1]` — broke floats and all multi-char tokens
- `heap_alloc` added as `KW_HEAP_ALLOC` keyword token

---

### Phase 2 — Parser + AST ✅ COMPLETE

Files: `src/parser/ast.h`, `parser.h`, `parser.cpp`

AST nodes:
- `ProgramNode`, `FunctionDefNode` (has `isSecure`), `BlockNode`
- `VarDeclNode` (has `resolvedType`, `heapElementType`), `ReturnStmtNode`, `IfStmtNode`
- `ForStmtNode` — `for i in start..end { }`
- `WhileStmtNode` — `while (cond) { }`
- `AssignStmtNode` — `x = expr;`
- `HeapAllocExprNode` — `heap_alloc(n)` with `elementType`, `size`
- `FreeStmtNode` — `free(varName)`
- `IndexExprNode` — `arr[i]` with `resolvedType`
- `BinOpNode`, `IntLiteralNode`, `FloatLiteralNode`, `StringLiteralNode`
- `IdentifierNode` (has `resolvedType`), `FunctionCallNode`

Parser handles:
- `parseTypeName()` — handles `heap<T>` returning `"heap<int>"` etc
- `parseAssignOrCall()` — peeks ahead to distinguish assignment vs function call
- `parseHeapAlloc()`, `parseFreeStmt()`
- `IndexAssignNode` (`arr[i] = expr`) — NOT YET IMPLEMENTED, deferred to post-Phase 6

---

### Phase 3 — C Code Emitter ✅ COMPLETE

Files: `src/codegen/codegen.h`, `codegen.cpp`

Gen methods: `genFunction()`, `genBlock()`, `genVarDecl()`, `genReturn()`, `genIf()`, `genFor()`, `genWhile()`, `genAssign()`, `genFree()`, `genExpr()`, `buildPrintf()`, `mapType()`

Key details:
- `genVarDecl()` detects `heap<T>` type and emits `malloc()` instead of stack declaration
- `inSecureContext` flag set in `genFunction()` — heap allocs inside `@Secure` get a `// secure_region` comment
- `buildPrintf()` reads `resolvedType` from `IdentifierNode` — no heuristics
- `genExpr()` handles `IndexExprNode` — emits `arr[i]`
- `#include <stdlib.h>` added for `malloc`/`free`
- `^` maps to `pow()`, `-lm` in gcc call

---

### Phase 4 — Type Checker & Semantic Analysis ✅ COMPLETE

Files: `src/semantic/symbol_table.h`, `symbol_table.cpp`, `type_checker.h`, `type_checker.cpp`

Key details:
- `checkBlock(bool createScope = true)` — called with `false` from `checkFunction()` to avoid double scoping
- `checkVarDecl()` — detects `heap<T>`, sets `heapElementType`, calls `memTracker.trackHeapVar()`
- `checkFree()` — validates variable is heap type, checks not already freed, marks freed
- `checkIndex()` — checks use-after-free, extracts element type from `heap<T>`
- `checkAssign()`, `checkFor()`, `checkWhile()`, `checkIf()`
- `resolvedType` written directly onto AST nodes during traversal — codegen reads from nodes, not symbol table
- `getType(name)` public method exists but scopes are gone after checking — use `resolvedType` on nodes instead

---

### Phase 5 — @Secure Enforcement + Memory Model ✅ COMPLETE (partial)

Files: `src/security/secure_checker.h`, `secure_checker.cpp`, `src/memory/memory_manager.h`, `memory_manager.cpp`

What works:
- `@Secure` functions reject `free()` calls
- `@Secure` functions reject `heap<T>` allocations — checked via `node->type.substr(0,5) == "heap<"`
- `MemoryTracker` (in `memory_manager.h/.cpp`) tracks heap variables and freed variables
- Use-after-free detection — throws `Memory Error at line N: use of freed variable 'x'`
- `free()` on non-heap variable throws error

Known limitations / deferred to post-Phase 6:
- `arr[i] = value` index assignment not yet parsed (`IndexAssignNode` not implemented)
- Full array support (heap arrays work for alloc/free but element assignment deferred)
- Stack allocator (`stack_alloc`) not implemented yet
- No runtime encryption of `secure_region` memory yet
- Full ownership / borrow-checker style tracking deferred

---

## Current build command

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

## Working example programs

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

**If/else:**
```
fn main() {
    let age: int = 20;
    if (age > 18) { print(1); } else { print(0); }
}
```

**For loop:**
```
fn main() {
    for i in 0..5 { print(i); }
}
```

**While loop + assignment:**
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
    let age: int = 20;
    print(x);
    print(name);
    print(age);
}
```

**Heap alloc + free:**
```
fn main() {
    let arr: heap<int> = heap_alloc(5);
    free(arr);
}
```

**Use-after-free rejection:**
```
fn main() {
    let arr: heap<int> = heap_alloc(5);
    free(arr);
    print(arr);  // rejected: Memory Error at line 4
}
```

**@Secure rejection:**
```
@Secure
fn safeFunc() {
    let arr: heap<int> = heap_alloc(5);  // rejected
}
```

---

## What is NOT built yet

| Feature | Status |
|---------|--------|
| Phase 6: Goroutines + channels | not started |
| Phase 6: LLVM backend | not started |
| `arr[i] = value` index assignment | deferred post-Phase 6 |
| Stack allocator | deferred post-Phase 6 |
| Full array support | deferred post-Phase 6 |
| Structs + impl blocks | deferred |

---

## Key design decisions

- LLVM backend deferred — emits C and calls `gcc`, swapping LLVM in at Phase 6
- `resolvedType` written onto AST nodes by type checker — codegen reads from nodes not symbol table
- Goroutine syntax tokenised but not parsed yet
- `output.c` and `./output` are build artifacts, not committed to repo
- `IndexAssignNode` (`arr[i] = expr`) deferred to after Phase 6

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

## Next task for AI — Phase 6: Goroutines + Channels + LLVM

Step 1 — Parse and codegen goroutines using pthreads:
- `go funcName(args)` → spawns a pthread
- `wait_all()` → `pthread_join` on all spawned threads
- Add `GoStmtNode` to AST

Step 2 — Channels:
- `chan<int> numbers;` — typed channel declaration
- `numbers.send(x)` — send value
- `numbers.recv()` — receive value
- Implement as a thread-safe queue using mutex + condition variable in C

Step 3 — LLVM backend (swap C emitter):
- Replace `system("gcc output.c ...")` with real LLVM IR generation
- Use LLVM C++ API to emit IR from AST
- Hook up LLVM optimiser passes (O1–O3)