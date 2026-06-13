#pragma once
#include <string>
#include <vector>
#include <memory>

enum class NodeType {
    Program, FunctionDef, Block,
    VarDecl, ReturnStmt, IfStmt,
    ForStmt, WhileStmt, AssignStmt, 
    BinOp, IntLiteral, FloatLiteral,
    HeapAllocExpr,FreeStmt,IndexExpr,
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
    std::string resolvedType; //added
};

struct BinOpNode : ASTNode {
    std::string op;     
    NodePtr left;
    NodePtr right;
};

struct VarDeclNode : ASTNode {
    std::string name;
    std::string type;
    std::string resolvedType;
    std::string heapElementType; 
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

struct ForStmtNode : ASTNode {
    std::string varName;     // i
    NodePtr rangeStart;      // 0
    NodePtr rangeEnd;        // 10
    NodePtr body;            // BlockNode
    std::string resolvedType = "int"; // range vars are always int
};

struct WhileStmtNode : ASTNode {
    NodePtr condition;
    NodePtr body;
};

struct AssignStmtNode : ASTNode {
    std::string name;
    NodePtr value;
};

struct HeapAllocExprNode : ASTNode {
    std::string elementType;  // the T in heap<T>
    NodePtr size;             // heap_alloc(10) → size = 10
};

struct FreeStmtNode : ASTNode {
    std::string varName;
};

struct IndexExprNode : ASTNode {
    std::string varName;
    NodePtr index;
    std::string resolvedType;
};