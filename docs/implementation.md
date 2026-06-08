# Quantum Compiler — Implementation Guide

> For Tarunpreet: everything built in sessions 1 & 2, explained so you can pick up right where we left off.

---

## How to build and run

```bash
g++ src/main.cpp src/lexer/lexer.cpp src/parser/parser.cpp src/codegen/codegen.cpp -Isrc -std=c++17 -o qcc
./qcc examples/hello.q
./output
```

---

## The full pipeline (all working)

```
source.q  →  Lexer  →  tokens  →  Parser  →  AST  →  CodeGen  →  output.c  →  gcc  →  ./output
```

---

## Stage 1: Lexer

**What it does:** reads the raw `.q` source file character by character and groups characters into tokens.

**Files:**
- `src/lexer/token_types.h` — the `TokenType` enum. Every possible kind of symbol in the language has a name here (e.g. `KW_LET`, `IDENT`, `PLUS`, `ARROW`)
- `src/lexer/token.h` — the `Token` struct. Each token has: its type, its raw text value, and the line/col it came from
- `src/lexer/lexer.h` — class declaration
- `src/lexer/lexer.cpp` — the implementation

**How it works:** the main loop in `tokenize()` skips whitespace and `//` comments, looks at the current character, decides what kind of token starts here, reads it, and repeats until EOF.

Key methods: `peek()`, `peek2()` (for multi-char operators like `->`, `..`, `==`), `advance()`, `readNumber()`, `readString()`, `readIdentOrKeyword()`.

The `@` character is emitted as `AT` so `@Secure` becomes `AT` + `IDENT("Secure")`.

---

## Stage 2: Parser + AST

**What it does:** consumes the token stream and builds an Abstract Syntax Tree — a tree of objects representing the structure of the program.

**Files:**
- `src/parser/ast.h` — all AST node types as C++ structs
- `src/parser/parser.h` — class declaration
- `src/parser/parser.cpp` — the recursive-descent parser

**AST nodes defined:**

| Node | What it represents |
|------|--------------------|
| `ProgramNode` | whole file — list of functions |
| `FunctionDefNode` | `fn name(params) -> type { body }` |
| `BlockNode` | `{ statements... }` |
| `VarDeclNode` | `let x: int = 5;` |
| `ReturnStmtNode` | `return expr;` |
| `IfStmtNode` | `if (cond) { } else { }` |
| `BinOpNode` | `a + b`, `x == y`, etc. |
| `IntLiteralNode` | `42` |
| `FloatLiteralNode` | `3.14` |
| `StringLiteralNode` | `"hello"` |
| `IdentifierNode` | a variable name |
| `FunctionCallNode` | `add(x, y)` |

All nodes use `unique_ptr<ASTNode>` (aliased as `NodePtr`) — memory is automatic.

**Expression precedence** is enforced by the call chain:
```
parseExpression → parseComparison → parseAddSub → parseMulDiv → parsePrimary
```
Deeper in the chain = tighter binding. `*` and `/` bind before `+` and `-`.

**Error handling:** throws `std::runtime_error` with the line number on bad syntax.

---

## Stage 3: C Code Emitter (CodeGen)

**What it does:** walks the AST and emits equivalent C code into `output.c`, then calls `gcc` to compile it into a runnable binary (`./output`).

**Files:**
- `src/codegen/codegen.h` — `CodeGen` class declaration
- `src/codegen/codegen.cpp` — the implementation

**How it works:** every AST node type has a corresponding `gen*` method:

| Method | What it emits |
|--------|--------------|
| `genProgram()` | iterates over all functions |
| `genFunction()` | C function signature + body |
| `genBlock()` | `{` ... `}` with indentation tracking |
| `genVarDecl()` | `int x = 5;` |
| `genReturn()` | `return expr;` |
| `genIf()` | `if (cond) { } else { }` |
| `genExpr()` | recursively builds a C expression string |
| `buildPrintf()` | maps `print(x)` → `printf("%d\n", x)` |
| `mapType()` | `int→int`, `float→double`, `string→char*` |

**Special cases:**
- Quantum's `^` (power) maps to C's `pow()` — that's why `-lm` is in the gcc call
- `print()` is not a real C function — `buildPrintf` converts it to `printf` with a format string
- Format string heuristic: uses `%d` for int literals and identifiers, `%f` for floats, `%s` for strings. This is approximate until Phase 4 adds proper type tracking.

**Output flow in `main.cpp`:**
1. Lex → Parse → CodeGen → write `output.c`
2. `system("gcc output.c -o output -lm")` compiles it
3. Run `./output` to see results

---

## Working test programs

**Test 1 — function call + arithmetic:**
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
Expected output: `13`

**Test 2 — if/else:**
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
Expected output: `1`

---

## Project file structure

```
src/
  main.cpp                  ← entry point; lex → parse → codegen → gcc
  lexer/
    token_types.h           ← TokenType enum
    token.h                 ← Token struct
    lexer.h                 ← Lexer class declaration
    lexer.cpp               ← Lexer implementation
  parser/
    ast.h                   ← all AST node structs
    parser.h                ← Parser class declaration
    parser.cpp              ← recursive-descent parser
  codegen/
    codegen.h               ← CodeGen class declaration
    codegen.cpp             ← AST → C emitter
examples/
  hello.q                   ← test file
```

---

## What to build next: Phase 4 — Type checker

Create `src/semantic/symbol_table.h/.cpp` and `src/semantic/type_checker.h/.cpp`.

The type checker runs between parsing and codegen. It walks the AST, builds a symbol table (variable name → type + scope), and catches errors like undeclared variables, type mismatches, and wrong argument counts.

It also fixes the `buildPrintf` heuristic — once the type checker knows every variable's type, codegen can look it up and pick the right format string instead of guessing.