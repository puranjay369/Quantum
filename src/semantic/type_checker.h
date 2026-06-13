#pragma once
#include "../parser/ast.h"
#include "symbol_table.h"
#include <string>
#include <stdexcept>
#include "../memory/memory_manager.h"

// Traverses the AST after parsing to validate types and scoping.
class TypeChecker {
public:
    TypeChecker();

    // Main entry point
    void check(ProgramNode* program);
    std::string getType(const std::string& name);

private:
    SymbolTable symTable;
    MemoryTracker memTracker;
    std::string currentReturnType; // Tracks the return type of the enclosing function

    // Node visitors
    void checkProgram(ProgramNode* node);
    void checkFunction(FunctionDefNode* node);
    void checkBlock(BlockNode* node, bool createScope = true);
    void checkStatement(ASTNode* node);
    void checkVarDecl(VarDeclNode* node);
    void checkReturn(ReturnStmtNode* node);
    void checkIf(IfStmtNode* node);
    void checkFor(ForStmtNode* node);
    void checkWhile(WhileStmtNode* node);
    void checkAssign(AssignStmtNode* node);
    void checkFree(FreeStmtNode* node);

    // Expression evaluators (return the resolved type like "int" or "string")
    std::string checkIndex(IndexExprNode* node);
    std::string checkExpr(ASTNode* node);
    std::string checkBinOp(BinOpNode* node);
    std::string checkFunctionCall(FunctionCallNode* node);
};