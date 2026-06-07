#pragma once
#include <string>
#include <vector>
#include <memory>

enum class NodeType {
    Program, FunctionDef, Block,
    VarDecl, ReturnStmt, IfStmt,
    BinOp, IntLiteral, FloatLiteral,
    StringLiteral, Identifier, FunctionCall
};

struct ASTNode {
    NodeType kind;
    int line, col;
    virtual ~ASTNode() = default;
};

using NodePtr = std::unique_ptr<ASTNode>;

struct IntLiteralNode : ASTNode {
    int value;
};

struct FloatLiteralNode : ASTNode {
    double value;
};

struct StringLiteralNode : ASTNode {
    std::string value;
};

struct IdentifierNode : ASTNode {
    std::string name;
};

struct BinOpNode : ASTNode {
    std::string op;     
    NodePtr left;
    NodePtr right;
};

struct VarDeclNode : ASTNode {
    std::string name;
    std::string type;   
    NodePtr initializer; 
};

struct ReturnStmtNode : ASTNode {
    NodePtr value;
};

struct IfStmtNode : ASTNode {
    NodePtr condition;
    NodePtr thenBlock;
    NodePtr elseBlock;  
};

struct BlockNode : ASTNode {
    std::vector<NodePtr> statements;
};

struct FunctionCallNode : ASTNode {
    std::string callee;
    std::vector<NodePtr> args;
};

struct FunctionDefNode : ASTNode {
    std::string name;
    std::vector<std::pair<std::string,std::string>> params;
    std::string returnType;
    bool isSecure = false;
    NodePtr body; 
};

struct ProgramNode : ASTNode {
    std::vector<NodePtr> functions;
};