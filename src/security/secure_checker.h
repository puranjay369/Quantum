#pragma once
#include "../parser/ast.h"
#include <stdexcept>

class SecureChecker {
public:
    void check(ProgramNode* program);

private:
    bool inSecureFunction = false;

    void checkFunction(FunctionDefNode* node);
    void checkBlock(BlockNode* node);
    void checkStatement(ASTNode* node);
    void checkExpr(ASTNode* node);
    void checkVarDecl(VarDeclNode* node);

    void reject(const std::string& msg, int line);
};