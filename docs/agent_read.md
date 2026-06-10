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

## What has been built (as of session 2)

### Phase 1 — Lexer  { COMPLETE }

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

Milestone reached: `./qcc examples/hello.q` prints all tokens with line/col. 

---

### Phase 2 — Parser + AST  { COMPLETE }

Files written:
- `src/parser/ast.h` — all AST node structs using `unique_ptr` ownership:
  - `ProgramNode`, `FunctionDefNode`, `BlockNode`
  - `VarDeclNode`, `ReturnStmtNode`, `IfStmtNode`
  - `BinOpNode`, `IntLiteralNode`, `FloatLiteralNode`, `StringLiteralNode`
  - `IdentifierNode`, `FunctionCallNode`
  - `NodePtr = unique_ptr<ASTNode>`
- `src/parser/parser.h` — `Parser` class declaration
- `src/parser/parser.cpp` — recursive-descent parser:
  - `parseProgram()` → `parseFunctionDef()` → `parseBlock()` → `parseStatement()`
  - Expression chain: `parseExpression` → `parseComparison` → `parseAddSub` → `parseMulDiv` → `parsePrimary`
  - Handles `@Secure` annotation on functions
  - Throws `std::runtime_error` with line number on syntax errors
- `src/main.cpp` — entry point: reads `.q` file → lexes → parses → calls `printAST()` to print the tree

Milestone reached: `./qcc examples/hello.q` prints a correct AST tree. 

---

### Phase 3 — C Code Emitter  { COMPLETE }

Files written:
- `src/codegen/codegen.h` — `CodeGen` class declaration
- `src/codegen/codegen.cpp` — walks the AST and emits valid C code:
  - `generate()` — entry point, emits `#include` headers then calls `genProgram()`
  - `genFunction()` — emits C function signature + body
  - `genBlock()` — emits `{ ... }` with indentation
  - `genVarDecl()` — emits `int x = 5;` style declarations
  - `genReturn()` — emits `return expr;`
  - `genIf()` — emits `if (cond) { } else { }`
  - `genExpr()` — recursively emits expressions; maps `^` to `pow()`
  - `buildPrintf()` — maps Quantum's `print()` to `printf()` with format string heuristic
  - `mapType()` — maps Quantum types to C types: `int→int`, `float→double`, `string→char*`
- `src/main.cpp` — updated: after parsing, calls `CodeGen::generate()`, writes `output.c`, shells out to `gcc -lm`, produces `./output` binary

Known limitation: `buildPrintf` currently defaults to `%d`/heuristics until we deeply bind the types resolved from Phase 4 directly to the AST nodes before running CodeGen.

Milestone reached: `./qcc examples/hello.q && ./output` compiles and runs Quantum code. 

---

### Phase 4 — Type Checker & Semantic Analysis { COMPLETE }

Files written:
- `src/semantic/symbol_table.h` & `symbol_table.cpp` — Stack of hash maps that handles local scopes logic (`enterScope`, `exitScope`).
- `src/semantic/type_checker.h` & `type_checker.cpp` — Traverses the AST validating variables exist, expressions match logically (e.g. `int == int`), checks function signatures, and ensures `return` types match the declared function.

Milestone reached: Semantic validation blocks compilation and throws descriptive errors before CodeGen if types are faulty. 

---

## Current build command

```bash
g++ src/main.cpp src/lexer/lexer.cpp src/parser/parser.cpp src/semantic/symbol_table.cpp src/semantic/type_checker.cpp src/codegen/codegen.cpp -Isrc -std=c++17 -o qcc
./qcc examples/hello.q
./output
```

---

## Working example programs

**Function call + arithmetic:**
```
fn multiply(a: int, b: int) -> int {
    return a + b;
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

---

## What is NOT built yet

| Phase | What | Status |
|-------|------|--------|
| 5 | `@Secure` enforcement + memory model | not started |
| 6 | Goroutines + channels + LLVM backend | not started |

---

## Key design decisions made

- LLVM backend is deferred — Phase 3 emits C and calls `gcc`, swapping LLVM in at Phase 6
- `@Secure` is already tokenised and parsed (`FunctionDefNode.isSecure = true`) but not enforced yet
- No garbage collector — stack-first, manual heap in QF mode
- Goroutine syntax (`go`, channels) is tokenised but not parsed yet
- `output.c` and `./output` are intermediate build artifacts, not committed to repo

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
  codegen/
    codegen.h
    codegen.cpp
examples/
  hello.q
```

---

## Next task for AI

**Phase 5 — @Secure & Memory Model**
Enforce memory rules on the AST. Validate the `@Secure` annotations, block raw pointer arithmetic manually, and implement strict ownership tracking inside of Secure functions.