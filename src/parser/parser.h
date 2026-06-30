#pragma once
#include <vector>
#include "../lexer/token.h"
#include "ast.h"

// Parser builds an Abstract Syntax Tree (AST) from a stream of Tokens.
// It uses a recursive-descent strategy mapping directly to Quantum's grammar.
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<ProgramNode> parse();

private:
    std::vector<Token> tokens;
    size_t pos = 0;

    // Token traversal helpers
    Token&  peek();
    Token&  peeknext();
    Token   advance();
    bool    check(TokenType t);
    bool    match(TokenType t);                          // Consumes if type matches
    Token   expect(TokenType t, const std::string& msg); // Throws if no match
    bool    isAtEnd();

    // AST node parsers
    std::unique_ptr<ProgramNode>     parseProgram();
    std::unique_ptr<FunctionDefNode> parseFunctionDef(bool isSecure);
    std::unique_ptr<BlockNode>       parseBlock();
    NodePtr                          parseStatement();
    NodePtr                          parseVarDecl();
    NodePtr                          parseReturnStmt();
    NodePtr                          parseIfStmt();
    NodePtr                          parseForStmt();
    NodePtr                          parseWhileStmt();
    NodePtr                          parseAssignOrCall();
    NodePtr                          parseHeapType();      // parses heap<int>
    NodePtr                          parseHeapAlloc();     // parses heap_alloc(10)
    NodePtr                          parseFreeStmt();      // parses free(arr);
    NodePtr                          parseIndexExpr(const std::string& name); // parses arr[i]
    NodePtr                          parseGoStmt();

    // Expression precedence parsers (lowest to highest precedence)
    NodePtr                          parseExpression();
    NodePtr                          parseComparison();
    NodePtr                          parseAddSub();
    NodePtr                          parseMulDiv();
    NodePtr                          parsePrimary();
    
    std::string                      parseTypeName();
};