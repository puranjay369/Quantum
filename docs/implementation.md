# Quantum Compiler — Implementation Guide

> For Tarunpreet: everything Puranjay built in session 1, explained so you can pick up right where we left off.

---

## What we built

The compiler pipeline has two stages working end-to-end:

```
Source (.q file)  →  [Lexer]  →  Token stream  →  [Parser]  →  AST  →  printed to terminal
```

---

## How to build and run

```bash
g++ src/main.cpp src/lexer/lexer.cpp src/parser/parser.cpp -Isrc -std=c++17 -o qcc
./qcc examples/hello.q
```

You should see a tree like:
```
Program
  FunctionDef: main
    Block
      VarDecl: x : int
        IntLiteral: 5
```

---

## Stage 1: Lexer

**What it does:** reads the raw `.q` source file character by character and groups characters into tokens.

**Files:**
- `src/lexer/token_types.h` — the `TokenType` enum. Every possible kind of symbol in the language has a name here (e.g. `KW_LET`, `IDENT`, `PLUS`, `ARROW`)
- `src/lexer/token.h` — the `Token` struct. Each token has: its type, its raw text value, and the line/col it came from
- `src/lexer/lexer.h` — class declaration
- `src/lexer/lexer.cpp` — the implementation

**How the lexer works:**

The main loop in `tokenize()` is:
1. Skip whitespace and `//` line comments
2. Look at the current character
3. Decide what kind of token starts here and read it
4. Repeat until end of file

Key methods:
- `peek()` — look at current char without consuming it
- `peek2()` — look one ahead (used to distinguish `->` from `-`, `..` from `.`, `==` from `=`)
- `advance()` — consume and return current char, update line/col counters
- `readNumber()` — reads int or float literals
- `readString()` — reads `"..."` string literals
- `readIdentOrKeyword()` — reads a word, then checks a lookup table to see if it's a keyword

The `@` character is emitted as an `AT` token so `@Secure` gets parsed as `AT` + `IDENT("Secure")`.

---

## Stage 2: Parser + AST

**What it does:** consumes the token stream and builds an Abstract Syntax Tree — a tree of objects that represents the structure of the program.

**Files:**
- `src/parser/ast.h` — all AST node types as C++ structs
- `src/parser/parser.h` — class declaration
- `src/parser/parser.cpp` — the recursive-descent parser

**AST nodes defined (in ast.h):**

| Node | What it represents |
|------|--------------------|
| `ProgramNode` | the whole file — holds a list of functions |
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

All nodes are heap-allocated via `unique_ptr<ASTNode>` (aliased as `NodePtr`). Memory is managed automatically.

**How the parser works — recursive descent:**

Each grammar rule is a function that calls other functions. The call chain mirrors the grammar:

```
parseProgram
  └─ parseFunctionDef (one per fn in the file)
       └─ parseBlock
            └─ parseStatement (one per line)
                 ├─ parseVarDecl      (if starts with let)
                 ├─ parseReturnStmt   (if starts with return)
                 ├─ parseIfStmt       (if starts with if)
                 └─ parseExpression   (anything else)
                      └─ parseComparison
                           └─ parseAddSub
                                └─ parseMulDiv
                                     └─ parsePrimary
```

The expression chain (`parseComparison` → `parseAddSub` → `parseMulDiv` → `parsePrimary`) is how operator precedence is enforced. `*` and `/` bind tighter than `+` and `-` because `parseMulDiv` is called deeper in the chain.

**Helper methods in the parser:**
- `check(type)` — is the current token of this type?
- `match(type)` — if yes, consume it and return true
- `expect(type, msg)` — consume it or throw an error with the line number
- `advance()` — consume and return current token

**Error handling:** the parser throws `std::runtime_error` with the line number whenever it hits something unexpected. This is basic but good enough for now — we will improve error messages in Phase 4.

---

## Stage 3: main.cpp

`main.cpp` wires everything together:
1. Open and read the `.q` file
2. Create a `Lexer`, call `tokenize()`
3. Create a `Parser` with the token list, call `parse()` to get an AST
4. Call `printAST()` to print the tree (for debugging)

`printAST()` is a recursive function in `main.cpp` that walks the AST and prints each node with indentation.

---

## What to build next: Phase 3 — C code emitter

The next step is to create `src/codegen/codegen.cpp`. It walks the AST and emits C code into a `.c` file, then calls `gcc` on it to produce a real binary.

This means `./qcc examples/hello.q` will compile and actually run for the first time.

Key things the emitter needs to handle first:
- `fn main()` → `int main()`
- `let x: int = 5;` → `int x = 5;`
- `a + b`, `x == y` → same in C
- `print(x)` → `printf("%d\n", x)` (or similar)
- `return expr;` → `return expr;`

Start with `src/codegen/codegen.h` and `src/codegen/codegen.cpp`, then wire it into `main.cpp`.

---

## Project file structure

```
src/
  main.cpp                  ← entry point, printAST lives here
  lexer/
    token_types.h           ← TokenType enum
    token.h                 ← Token struct
    lexer.h                 ← Lexer class declaration
    lexer.cpp               ← Lexer implementation
  parser/
    ast.h                   ← all AST node structs
    parser.h                ← Parser class declaration
    parser.cpp              ← recursive-descent parser
examples/
  hello.q                   ← test file
```