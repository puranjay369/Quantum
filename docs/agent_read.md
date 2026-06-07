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

## What has been built (as of session 1)

### Phase 1 — Lexer ✅ COMPLETE

Files written:
- `src/lexer/token_types.h` — enum class `TokenType` covering all keywords, operators, literals, delimiters
- `src/lexer/token.h` — `Token` struct: `{TokenType type, string value, int line, int col}`
- `src/lexer/lexer.h` — `Lexer` class declaration
- `src/lexer/lexer.cpp` — full implementation: `peek()`, `peek2()`, `advance()`, `skipWhitespaceAndComments()`, `readNumber()`, `readString()`, `readIdentOrKeyword()`, `tokenize()`

Key behaviours:
- Handles `->` vs `-`, `..` vs `.`, `==` vs `=`, `!=`, `<=`, `>=`
- Line comments via `//`
- Tracks line + col for every token
- `@` is emitted as `AT` token (for `@Secure`)

Milestone reached: `./qcc examples/hello.q` prints all tokens with line/col. ✅

---

### Phase 2 — Parser + AST ✅ COMPLETE

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

Milestone reached: `./qcc examples/hello.q` prints a correct AST tree. ✅

---

## Current build command

```bash
g++ src/main.cpp src/lexer/lexer.cpp src/parser/parser.cpp -Isrc -std=c++17 -o qcc
./qcc examples/hello.q
```

---

## What is NOT built yet

| Phase | What | Status |
|-------|------|--------|
| 3 | C code emitter (AST → .c → gcc) | not started |
| 4 | Type checker + symbol table | not started |
| 5 | `@Secure` enforcement + memory model | not started |
| 6 | Goroutines + channels + LLVM backend | not started |

---

## Key design decisions made

- LLVM backend is deferred — Phase 3 will emit C and call `gcc`, swapping LLVM in at Phase 6
- `@Secure` is already tokenised and parsed (`FunctionDefNode.isSecure = true`) but not enforced yet
- No garbage collector — stack-first, manual heap in QF mode
- Goroutine syntax (`go`, channels) is tokenised but not parsed yet

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
examples/
  hello.q
```

---

## Next task for AI

<!-- **Phase 3 — C code emitter.**
Create `src/codegen/codegen.h` and `src/codegen/codegen.cpp`.
Walk the AST and emit valid C into a `.c` file, then shell out to `gcc` to produce a binary.
Start with: variable declarations, arithmetic, function definitions, `print()` mapped to `printf`. -->