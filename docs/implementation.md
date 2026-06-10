# Quantum Compiler — Implementation Guide

> For Tarunpreet: everything built in sessions 1, 2 & 3, explained so you can pick up right where we left off.

---

## How to build and run

```bash
g++ src/main.cpp src/lexer/lexer.cpp src/parser/parser.cpp \
    src/semantic/symbol_table.cpp src/semantic/type_checker.cpp \
    src/security/secure_checker.cpp \
    src/codegen/codegen.cpp \
    -Isrc -std=c++17 -o qcc
./qcc examples/hello.q
./output
```

---

## The full pipeline (all working)

```
source.q → Lexer → tokens → Parser → AST → TypeChecker → SecureChecker → CodeGen → output.c → gcc → ./output
```

---

## Stage 1: Lexer

**What it does:** reads the raw `.q` source file character by character and groups characters into tokens.

**Files:**
- `src/lexer/token_types.h` — the `TokenType` enum
- `src/lexer/token.h` — `Token` struct: `{TokenType type, string value, int line, int col}`
- `src/lexer/lexer.h` — class declaration
- `src/lexer/lexer.cpp` — implementation

**How it works:** the main loop in `tokenize()` skips whitespace and `//` comments, looks at the current character, decides what kind of token starts here, reads it, repeats until EOF.

Key methods: `peek()`, `peeknext()`, `advance()`, `readNumber()`, `readString()`, `readIdentOrKeyword()`.

**Bug fixed this session:** `peeknext()` was returning `source[pos]` instead of `source[pos + 1]`. This broke float literals, `..` range operator, `->`, `==`, and every other multi-char token. Fix was one character change.

The `@` character is emitted as `AT` so `@Secure` becomes `AT` + `IDENT("Secure")`.

---

## Stage 2: Parser + AST

**What it does:** consumes the token stream and builds an Abstract Syntax Tree.

**Files:**
- `src/parser/ast.h` — all AST node types as C++ structs
- `src/parser/parser.h` — class declaration
- `src/parser/parser.cpp` — recursive-descent parser

**AST nodes defined:**

| Node | What it represents |
|------|--------------------|
| `ProgramNode` | whole file — list of functions |
| `FunctionDefNode` | `fn name(params) -> type { body }`, has `isSecure` flag |
| `BlockNode` | `{ statements... }` |
| `VarDeclNode` | `let x: int = 5;`, has `resolvedType` set by type checker |
| `ReturnStmtNode` | `return expr;` |
| `IfStmtNode` | `if (cond) { } else { }` |
| `ForStmtNode` | `for i in 0..10 { }` — has `varName`, `rangeStart`, `rangeEnd`, `body` |
| `WhileStmtNode` | `while (cond) { }` — has `condition`, `body` |
| `AssignStmtNode` | `x = expr;` — has `name`, `value` |
| `BinOpNode` | `a + b`, `x == y`, etc. |
| `IntLiteralNode` | `42` |
| `FloatLiteralNode` | `3.14` |
| `StringLiteralNode` | `"hello"` |
| `IdentifierNode` | variable name, has `resolvedType` set by type checker |
| `FunctionCallNode` | `add(x, y)` |

All nodes use `unique_ptr<ASTNode>` aliased as `NodePtr`.

**Expression precedence chain:**
```
parseExpression → parseComparison → parseAddSub → parseMulDiv → parsePrimary
```

**`parseAssignOrCall()`** — the default statement handler. Peeks ahead: if `IDENT =` it's an assignment, if `IDENT (` it's a function call.

**Error handling:** throws `std::runtime_error` with line number on bad syntax.

---

## Stage 3: Type Checker

**What it does:** walks the AST after parsing, validates types and scopes, and annotates AST nodes with resolved types so codegen doesn't have to guess.

**Files:**
- `src/semantic/symbol_table.h` / `symbol_table.cpp` — stack of hash maps
  - `enterScope()` / `exitScope()` — push/pop scope
  - `define(name, type, isFunction, paramTypes)` — register symbol
  - `lookup(name)` — search from innermost scope outward, throws if not found
  - `isDefinedInCurrentScope(name)` — prevents redefinition in same scope
- `src/semantic/type_checker.h` / `type_checker.cpp` — AST traversal

**Key methods:**

| Method | What it does |
|--------|-------------|
| `checkFunction()` | enters scope, registers params, calls `checkBlock(false)` |
| `checkBlock(createScope)` | iterates statements; `false` skips double-scoping |
| `checkVarDecl()` | validates initializer type matches declared type, sets `node->resolvedType` |
| `checkAssign()` | looks up variable type, validates RHS matches |
| `checkFor()` | enters scope, defines loop var as `int`, checks range + body |
| `checkWhile()` | validates condition is `bool` or `int` |
| `checkIf()` | validates condition, checks both branches |
| `checkExpr()` | returns resolved type; sets `id->resolvedType` on `IdentifierNode` |
| `checkBinOp()` | enforces type match; comparison ops return `"bool"` |
| `checkFunctionCall()` | validates callee exists, arg count, arg types |
| `getType(name)` | public lookup — only valid while scopes are active |

**Critical bug fixed:** `checkBlock()` was always calling `enterScope()`/`exitScope()`, but `checkFunction()` also did — causing double scoping. Variables defined inside a function were going into an inner scope that got popped before `print(result)` ran. Fix: `checkBlock(bool createScope = true)`, called with `false` from `checkFunction()`.

**How resolved types flow to codegen:** instead of querying the symbol table at codegen time (scopes are gone by then), the type checker writes `resolvedType` directly onto `IdentifierNode` and `VarDeclNode` during traversal. Codegen reads it from there.

---

## Stage 4: Security Checker

**What it does:** runs after the type checker, before codegen. Walks only `@Secure` functions and rejects unsafe operations.

**Files:**
- `src/security/secure_checker.h` / `secure_checker.cpp`

**Rules enforced inside `@Secure` functions:**
- No `free()` calls
- No `heap` allocations

**Error format:** `@Secure violation at line N: <reason>`

Only functions with `FunctionDefNode.isSecure = true` are checked. Regular functions are skipped entirely.

---

## Stage 5: CodeGen

**What it does:** walks the AST and emits valid C into `output.c`, then shells out to `gcc` to produce `./output`.

**Files:**
- `src/codegen/codegen.h` / `codegen.cpp`

**Gen methods:**

| Method | What it emits |
|--------|--------------|
| `genFunction()` | C function signature + body |
| `genBlock()` | `{` ... `}` with indentation |
| `genVarDecl()` | `int x = 5;` |
| `genReturn()` | `return expr;` |
| `genIf()` | `if (cond) { } else { }` |
| `genFor()` | `for (int i = start; i < end; i++)` |
| `genWhile()` | `while (cond)` |
| `genAssign()` | `x = expr;` |
| `genExpr()` | recursively builds C expression string |
| `buildPrintf()` | maps `print(x)` → correct `printf` format string |
| `mapType()` | `int→int`, `float→double`, `string→char*` |

**`buildPrintf()` fix:** no longer uses heuristics. Reads `resolvedType` from `IdentifierNode` directly — set by the type checker during traversal. Picks `%d`, `%f`, or `%s` correctly for any variable type.

**Special:** Quantum's `^` maps to `pow()` — that's why `-lm` is in the gcc call.

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
    for i in 0..5 {
        print(i);
    }
}
```

**While loop + assignment:**
```
fn main() {
    let x: int = 0;
    while (x < 5) {
        print(x);
        x = x + 1;
    }
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

---

## Project file structure

```
src/
  main.cpp                    ← lex → parse → typecheck → securecheck → codegen → gcc
  lexer/
    token_types.h
    token.h
    lexer.h
    lexer.cpp
  parser/
    ast.h                     ← all AST node structs (includes resolvedType fields)
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
  codegen/
    codegen.h
    codegen.cpp
examples/
  hello.q
```

---

## What to build next: Phase 6 — Goroutines + LLVM

- Parse `go funcName(args)` — spawns a goroutine
- Parse `chan<T>` type, `.send()`, `.recv()` methods
- Add `wait_all()` built-in
- Implement goroutines as pthreads first, then swap to LLVM IR later
- Swap C emitter for real LLVM IR generation