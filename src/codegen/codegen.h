#pragma once
#include <string>
#include <sstream>
#include "../parser/ast.h"
#include "../semantic/type_checker.h"

// CodeGen (C-Emitter Bridge)
// Walks the fully parsed AST and emits valid, equivalent C code.
// The output is then passed to GCC to generate native machine code.
class CodeGen {
public:
    std::string generate(ProgramNode* program);

private:
    std::ostringstream out;  // Accumulates generated C code
    int indentLevel = 0;

    TypeChecker* typeChecker = nullptr;

    bool inSecureContext = false;

    void emit(const std::string& s);    // Write a line with indentation
    void emitRaw(const std::string& s); // Write without newline (for expressions)

    // Code generators for different AST nodes
    void genProgram(ProgramNode* node);
    void genFunction(FunctionDefNode* node);
    void genBlock(BlockNode* node);
    void genStatement(ASTNode* node);
    void genVarDecl(VarDeclNode* node);
    void genReturn(ReturnStmtNode* node);
    void genIf(IfStmtNode* node);
    void genFor(ForStmtNode* node);
    void genWhile(WhileStmtNode* node);
    void genAssign(AssignStmtNode* node);
    void genFunctionCall(FunctionCallNode* node);
    void genFree(FreeStmtNode* node);
    
    std::string genExpr(ASTNode* node);              // Returns C expression as a string
    std::string buildPrintf(FunctionCallNode* node); // Converts print() -> printf()
    std::string mapType(const std::string& qtype);   // Maps int->int, string->char*, etc.
    std::string indent();
};