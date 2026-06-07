#pragma once
#include <vector>
#include "../lexer/token.h"
#include "ast.h"

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<ProgramNode> parse();

private:
    std::vector<Token> tokens;
    size_t pos = 0;

    Token&  peek();
    Token&  peek2();
    Token   advance();
    bool    check(TokenType t);
    bool    match(TokenType t);
    Token   expect(TokenType t, const std::string& msg);
    bool    isAtEnd();

    std::unique_ptr<ProgramNode>     parseProgram();
    std::unique_ptr<FunctionDefNode> parseFunctionDef(bool isSecure);
    std::unique_ptr<BlockNode>       parseBlock();
    NodePtr                          parseStatement();
    NodePtr                          parseVarDecl();
    NodePtr                          parseReturnStmt();
    NodePtr                          parseIfStmt();
    NodePtr                          parseExpression();
    NodePtr                          parseComparison();
    NodePtr                          parseAddSub();
    NodePtr                          parseMulDiv();
    NodePtr                          parsePrimary();
    std::string                      parseTypeName();
};