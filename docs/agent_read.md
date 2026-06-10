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

## What has been built (as of session 3)

### Phase 1 — Lexer ✅ COMPLETE

Files written:
- `src/lexer/token_types.h` — enum class `TokenType` covering all keywords, operators, literals, delimiters
- `src/lexer/token.h` — `Token` struct: `{TokenType type, string value, int line, int col}`
- `src/lexer/lexer.h` — `Lexer` class declaration
- `src/lexer/lexer.cpp` — full implementation: `peek()`, `peeknext()`, `advance()`, `skipWhitespaceAndComments()`, `readNumber()`, `readString()`, `readIdentOrKeyword()`, `tokenize()`

Key behaviours:
- Handles `->` vs `-`, `..` vs `.`, `==` vs `=`, `!=`, `<=`, `>=`
- Line comments via `//`
- Tracks line + col for every token
- `@` is emitted as `AT` token (for `@Secure`)

Bug fixed: `peeknext()` was returning `source[pos]` instead of `source[pos + 1]` — caused float literals, `..`, `->`, and all multi-char tokens to malfunction.

---

### Phase 2 — Parser + AST ✅ COMPLETE

Files written:
- `src/parser/ast.h` — all AST node structs using `unique_ptr` ownership:
  - `ProgramNode`, `FunctionDefNode`, `BlockNode`
  - `VarDeclNode`, `ReturnStmtNode`, `IfStmtNode`
  - `ForStmtNode` — `for i in start..end { }` with `varName`, `rangeStart`, `rangeEnd`, `body`
  - `WhileStmtNode` — `while (cond) { }` with `condition`, `body`
  - `AssignStmtNode` — `x = expr;` with `name`, `value`
  - `BinOpNode`, `IntLiteralNode`, `FloatLiteralNode`, `StringLiteralNode`
  - `IdentifierNode` — has `resolvedType` field set by type checker
  - `FunctionCallNode`
  - `NodePtr = unique_ptr<ASTNode>`
- `src/parser/parser.h` / `parser.cpp` — recursive-descent parser:
  - `parseStatement()` handles: `let`, `return`, `if`, `for`, `while`, assignment, expression statements
  - `parseAssignOrCall()` — peeks ahead: `IDENT =` → assignment, `IDENT (` → function call
  - `parseForStmt()` — parses `for i in 0..10 { }`
  - `parseWhileStmt()` — parses `while (cond) { }`
  - Expression chain: `parseExpression` → `parseComparison` → `parseAddSub` → `parseMulDiv` → `parsePrimary`

---

### Phase 3 — C Code Emitter ✅ COMPLETE

Files written:
- `src/codegen/codegen.h` / `codegen.cpp` — walks AST and emits valid C:
  - `genFor()` — emits `for (int i = start; i < end; i++)`
  - `genWhile()` — emits `while (cond)`
  - `genAssign()` — emits `x = expr;`
  - `buildPrintf()` — reads `resolvedType` from `IdentifierNode` to pick correct format string (`%d`, `%f`, `%s`) — no longer uses heuristics
  - `mapType()` — `int→int`, `float→double`, `string→char*`
  - `genIf()`, `genVarDecl()`, `genReturn()`, `genFunction()`, `genBlock()`, `genExpr()`

---

### Phase 4 — Type Checker & Semantic Analysis ✅ COMPLETE

Files written:
- `src/semantic/symbol_table.h` / `symbol_table.cpp` — stack of hash maps, `enterScope`/`exitScope`, `define`, `lookup`, `isDefinedInCurrentScope`
- `src/semantic/type_checker.h` / `type_checker.cpp`:
  - `checkFor()` — enters scope, defines loop var as `int`, checks range and body
  - `checkWhile()` — validates condition is `bool` or `int`
  - `checkAssign()` — looks up variable type, validates RHS matches
  - `checkVarDecl()` — sets `node->resolvedType` on the AST node directly
  - `checkExpr()` — sets `id->resolvedType` on `IdentifierNode` during traversal
  - `getType()` — public method for external lookups (used carefully — scopes are gone after checking)
  - Bug fixed: double-scoping bug — `checkBlock()` takes `bool createScope = true`; called with `false` from `checkFunction()` to avoid double scope push

---

### Phase 5 — @Secure Enforcement ✅ COMPLETE

Files written:
- `src/security/secure_checker.h` / `secure_checker.cpp`:
  - Runs after type checker, before codegen
  - Only enforces rules inside `@Secure` functions (`FunctionDefNode.isSecure = true`)
  - Rejects `free()` calls inside `@Secure`
  - Rejects `heap` allocations inside `@Secure`
  - `reject()` throws with `@Secure violation at line N: ...` message

---

## Current build command

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

## Working example programs

**Functions + arithmetic + print:**
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
    if (age > 18) {
        print(1);
    } else {
        print(0);
    }
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

**Mixed types + string formatting:**
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

**@Secure rejection:**
```
@Secure
fn safeFunc() {
    free(somePtr);   // rejected at compile time
}
```

---

## What is NOT built yet

| Phase | What | Status |
|-------|------|--------|
| 6 | Goroutines + channels + LLVM backend | not started |

## Known limitations / deferred

- Loops (`for`, `while`) work but `for` only supports int ranges (`0..N`)
- No structs or impl blocks yet (parsed as tokens but no AST nodes)
- No `go` keyword or channels parsed yet
- LLVM backend deferred to Phase 6 — currently emits C and calls `gcc`

---

## File structure (relevant files only)

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
  codegen/
    codegen.h
    codegen.cpp
examples/
  hello.q
```

---

## Next task for AI

**Phase 6 — Goroutines + Channels + LLVM backend.**
- Add `go` keyword parsing — `go funcName(args)` spawns a goroutine
- Add `chan<T>` type parsing and `send`/`recv` methods
- Add `wait_all()` built-in
- Implement goroutines as pthreads first, swap to LLVM later
- Then swap C emitter for real LLVM IR generation