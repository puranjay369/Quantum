#pragma once
#include "../parser/ast.h"
#include "symbol_table.h"
#include <string>
#include <stdexcept>

// Traverses the AST after parsing to validate types and scoping.
class TypeChecker {
public:
    TypeChecker();

    // Main entry point
    void check(ProgramNode* program);

private:
    SymbolTable symTable;
    std::string currentReturnType; // Tracks the return type of the enclosing function

    // Node visitors
    void checkProgram(ProgramNode* node);
    void checkFunction(FunctionDefNode* node);
    void checkBlock(BlockNode* node);
    void checkStatement(ASTNode* node);
    void checkVarDecl(VarDeclNode* node);
    void checkReturn(ReturnStmtNode* node);
    void checkIf(IfStmtNode* node);

    // Expression evaluators (return the resolved type like "int" or "string")
    std::string checkExpr(ASTNode* node);
    std::string checkBinOp(BinOpNode* node);
    std::string checkFunctionCall(FunctionCallNode* node);
};