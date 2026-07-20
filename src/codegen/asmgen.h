#pragma once
#include <string>
#include <sstream>
#include <unordered_map>
#include "../parser/ast.h"

class AsmGen {
public:
    std::string generate(ProgramNode* program);

private:
    std::ostringstream out;  // Accumulates generated assembly code
    std::ostringstream data; // Accumulates data section code
    std::unordered_map<std::string, int> slots; // variable name -> stack offset ( provide slots for local variables in stack frame )
    
    int labelCounter = 0;    // For generating unique labels
    int nextOffset = 0;      // For allocating stack space for local variables
    
    void genProgram(ProgramNode* node);      // Generates assembly for the entire program
    void genFunction(FunctionDefNode* node); // Generates assembly for a function definition
    void genStatement(ASTNode* node);        // Generates assembly for a statement (variable declaration, function call, etc.)
    void genVarDecl(VarDeclNode* node);      // Generates assembly for a variable declaration
    void genExpr(ASTNode* node);        // puts a literal or variable's value into rax
    void genPrintRuntime();                  // prints whatever is currently in rax
};