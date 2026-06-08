#pragma once
#include <string>
#include <sstream>
#include "../parser/ast.h"

class CodeGen {
public:
    std::string generate(ProgramNode* program);

private:
    std::ostringstream out;  // we write C code into this
    int indentLevel = 0;

    void emit(const std::string& s);    // write a line with indentation
    void emitRaw(const std::string& s); // write without newline (for expressions)

    void genProgram(ProgramNode* node);
    void genFunction(FunctionDefNode* node);
    void genBlock(BlockNode* node);
    void genStatement(ASTNode* node);
    void genVarDecl(VarDeclNode* node);
    void genReturn(ReturnStmtNode* node);
    void genIf(IfStmtNode* node);
    void genFunctionCall(FunctionCallNode* node);
    std::string genExpr(ASTNode* node);  // returns C expression as a string
    std::string buildPrintf(FunctionCallNode* node); 
    std::string mapType(const std::string& qtype); // int→int, string→char*
    std::string indent();
};