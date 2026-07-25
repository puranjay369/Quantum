#include "type_checker.h"
#include <iostream>

TypeChecker::TypeChecker() : currentReturnType("void") {
    // Pre-declare built-in global functions
    symTable.define("print", "void", true);
    symTable.define("wait_all", "void", true);
}

void TypeChecker::check(ProgramNode* program) {
    checkProgram(program);
}

void TypeChecker::checkProgram(ProgramNode* node) {
    for (auto& decl : node->globals) {
        checkChanDecl(static_cast<ChanDeclNode*>(decl.get()));
    }

    // First pass: register all function signatures globally so they can call each other out-of-order.
    for (auto& fn : node->functions) {
        auto* fnode = static_cast<FunctionDefNode*>(fn.get());
        std::vector<std::string> pTypes;
        for (const auto& param : fnode->params) {
            pTypes.push_back(param.second);
        }
        symTable.define(fnode->name, fnode->returnType, true, pTypes);
    }

    // Second pass: step into the function bodies and check logic.
    for (auto& fn : node->functions) {
        checkFunction(static_cast<FunctionDefNode*>(fn.get()));
    }
}

void TypeChecker::checkFunction(FunctionDefNode* node) {
    currentReturnType = node->returnType; // Keep track of what "return" should match

    symTable.enterScope(); // Function body gets its own scope

    // Register parameters as local variables
    for (const auto& param : node->params) {
        symTable.define(param.first, param.second);
    }

    checkBlock(static_cast<BlockNode*>(node->body.get()),false);

    symTable.exitScope();
}

void TypeChecker::checkBlock(BlockNode* node, bool createScope) {
    if (createScope) symTable.enterScope();
    for (auto& stmt : node->statements) {
        checkStatement(stmt.get());
    }
    if (createScope) symTable.exitScope();
}

void TypeChecker::checkStatement(ASTNode* node) {
    switch (node->kind) {
        case NodeType::VarDecl:
            checkVarDecl(static_cast<VarDeclNode*>(node));
            break;
        case NodeType::ReturnStmt:
            checkReturn(static_cast<ReturnStmtNode*>(node));
            break;
        case NodeType::IfStmt:
            checkIf(static_cast<IfStmtNode*>(node));
            break;
        case NodeType::ForStmt:
            checkFor(static_cast<ForStmtNode*>(node));
            break;
        case NodeType::WhileStmt:
            checkWhile(static_cast<WhileStmtNode*>(node));
            break;
        case NodeType::AssignStmt:
            checkAssign(static_cast<AssignStmtNode*>(node));
            break;
        case NodeType::FunctionCall: // standalone method execution
            checkExpr(node);         
            break;
        case NodeType::FreeStmt:
            checkFree(static_cast<FreeStmtNode*>(node));
            break;
        case NodeType::GoStmt:
            checkGo(static_cast<GoStmtNode*>(node));
            break;
        case NodeType::ChanDecl:
            checkChanDecl(static_cast<ChanDeclNode*>(node));
            break;
        case NodeType::ChanSend:
            checkChanSend(static_cast<ChanSendNode*>(node));
            break;
        case NodeType::ChanRecv:
            checkChanRecv(static_cast<ChanRecvNode*>(node));
            break;
        default:
            checkExpr(node);
    }
}

void TypeChecker::checkVarDecl(VarDeclNode* node) {
    std::string exprType = checkExpr(node->initializer.get());

    if (node->type.substr(0, 5) != "heap<") {
        bool widensToDouble = (node->type == "double" && exprType == "float");
        if (exprType != node->type && !widensToDouble) {
            throw std::runtime_error("Type Error: Cannot assign type '" + exprType +
                                     "' to variable '" + node->name + "' of type '" + node->type + "'.");
        }
    } else {
        node->heapElementType = node->type.substr(5, node->type.size() - 6);
        memTracker.trackHeapVar(node->name);
    }

    node->resolvedType = node->type;
    symTable.define(node->name, node->type);
}

void TypeChecker::checkReturn(ReturnStmtNode* node) {
    std::string retType = checkExpr(node->value.get());
    
    if (retType != currentReturnType) {
        throw std::runtime_error("Type Error: Function expected to return '" + currentReturnType + 
                                 "' but got '" + retType + "'.");
    }
}

void TypeChecker::checkIf(IfStmtNode* node) {
    std::string condType = checkExpr(node->condition.get());
    
    // Quantum rule: IF conditionals must strictly evaluate to bool or int (evaluating > 0)
    // Here we will enforce "bool" conceptually, though our parser currently emits == as bool
    if (condType != "bool" && condType != "int") {
         throw std::runtime_error("Type Error: 'if' condition must be boolean or int, got '" + condType + "'.");
    }

    checkBlock(static_cast<BlockNode*>(node->thenBlock.get()));
    if (node->elseBlock) {
        checkBlock(static_cast<BlockNode*>(node->elseBlock.get()));
    }
}

void TypeChecker::checkFor(ForStmtNode* node) {
    symTable.enterScope();
    symTable.define(node->varName, "int"); // loop var is always int
    checkExpr(node->rangeStart.get());
    checkExpr(node->rangeEnd.get());
    checkBlock(static_cast<BlockNode*>(node->body.get()), false);
    symTable.exitScope();
}

void TypeChecker::checkWhile(WhileStmtNode* node) {
    std::string condType = checkExpr(node->condition.get());
    if (condType != "bool" && condType != "int") {
        throw std::runtime_error("Type Error: while condition must be bool or int, got '" + condType + "'.");
    }
    checkBlock(static_cast<BlockNode*>(node->body.get()));
}

void TypeChecker::checkAssign(AssignStmtNode* node) {
    SymbolInfo info = symTable.lookup(node->name);
    std::string exprType = checkExpr(node->value.get());
    if (exprType != info.type) {
        throw std::runtime_error("Type Error: Cannot assign '" + exprType +
                                 "' to variable '" + node->name + "' of type '" + info.type + "'.");
    }
}

std::string TypeChecker::getType(const std::string& name) {
    return symTable.lookup(name).type;
}

std::string TypeChecker::checkExpr(ASTNode* node) {
    switch (node->kind) {
        case NodeType::IntLiteral:   return "int";
        case NodeType::FloatLiteral: return "float";
        case NodeType::StringLiteral:return "string";
        
        case NodeType::Identifier: {
            auto* id = static_cast<IdentifierNode*>(node);
            memTracker.checkNotFreed(id->name, id->line);
            id->resolvedType = symTable.lookup(id->name).type; 
            return id->resolvedType;
            //return symTable.lookup(id->name).type; // Ensures variable actually exists!
        }
        case NodeType::BinOp:
            return checkBinOp(static_cast<BinOpNode*>(node));
            
        case NodeType::FunctionCall:
            return checkFunctionCall(static_cast<FunctionCallNode*>(node));
        
        case NodeType::HeapAllocExpr:
            return "heap<int>";
        
        case NodeType::IndexExpr:
            return checkIndex(static_cast<IndexExprNode*>(node));
        
        case NodeType::ChanRecv:
            return checkChanRecv(static_cast<ChanRecvNode*>(node));
        
        case NodeType::BoolLiteral: 
            return "bool";

        case NodeType::UnaryOp:
            return checkUnaryOp(static_cast<UnaryOpNode*>(node)); 

        default:
            throw std::runtime_error("Type Error: Unknown expression type.");
    }
}

std::string TypeChecker::checkBinOp(BinOpNode* node) {
    std::string leftType = checkExpr(node->left.get());
    std::string rightType = checkExpr(node->right.get());

    // Basic rule: operands must match (e.g., int + int)
    if (leftType != rightType) {
        throw std::runtime_error("Type Error: Operand mismatch. Left is '" + leftType + 
                                 "', Right is '" + rightType + "'.");
    }

    // Comparison operators always return bool. Math operators return the type of their operands.
    if (node->op == "==" || node->op == "!=" || node->op == ">" || 
        node->op == "<" || node->op == ">=" || node->op == "<=") {
        return "bool";
    }

    return leftType;
}

std::string TypeChecker::checkFunctionCall(FunctionCallNode* node) {
    // 1. Make sure function exists
    SymbolInfo info = symTable.lookup(node->callee);
    
    if (!info.isFunction) {
        throw std::runtime_error("Type Error: '" + node->callee + "' is a variable, not a function.");
    }

    // 2. Resolve types of all arguments being passed in
    if (node->callee != "print") {
        if (node->args.size() != info.paramTypes.size()) {
            throw std::runtime_error("Type Error: Function '" + node->callee + "' expects " + 
                                     std::to_string(info.paramTypes.size()) + " arguments, but got " + 
                                     std::to_string(node->args.size()) + ".");
        }
        for (size_t i = 0; i < node->args.size(); ++i) {
            std::string argType = checkExpr(node->args[i].get());
            if (argType != info.paramTypes[i]) {
                throw std::runtime_error("Type Error: Argument " + std::to_string(i + 1) + " of '" + 
                                         node->callee + "' expects '" + info.paramTypes[i] + "', but got '" + argType + "'.");
            }
        }
    } else {
        // 'print' is a built-in that takes any number of arguments/types for now
        for (auto& arg : node->args) {
            checkExpr(arg.get());
        }
    }

    return info.type; // Returns the function's return type
}

void TypeChecker::checkFree(FreeStmtNode* node) {
    SymbolInfo info = symTable.lookup(node->varName);
    if (info.type.substr(0, 5) != "heap<") {
        throw std::runtime_error(
            "Memory Error: Cannot free non-heap variable '" + node->varName + "'."
        );
    }
    memTracker.checkNotFreed(node->varName, node->line);
    memTracker.markFreed(node->varName);
}

std::string TypeChecker::checkIndex(IndexExprNode* node) {
    memTracker.checkNotFreed(node->varName, node->line);
    SymbolInfo info = symTable.lookup(node->varName);
    // extract T from heap<T>
    if (info.type.substr(0, 5) == "heap<") {
        std::string elemType = info.type.substr(5, info.type.size() - 6);
        node->resolvedType = elemType;
        return elemType;
    }
    throw std::runtime_error("Type Error: Cannot index non-heap variable '" + node->varName + "'.");
}

void TypeChecker::checkGo(GoStmtNode* node) {
    SymbolInfo info = symTable.lookup(node->funcName);
    if (!info.isFunction) {
        throw std::runtime_error(
            "Type Error: 'go " + node->funcName + "' — '" + node->funcName + "' is not a function."
        );
    }
    if (node->args.size() != info.paramTypes.size()) {
        throw std::runtime_error(
            "Type Error: go " + node->funcName + " expects " +
            std::to_string(info.paramTypes.size()) + " arguments, got " +
            std::to_string(node->args.size()) + "."
        );
    }
    for (size_t i = 0; i < node->args.size(); ++i) {
        std::string argType = checkExpr(node->args[i].get());
        if (argType != info.paramTypes[i]) {
            throw std::runtime_error(
                "Type Error: go " + node->funcName + " argument " + std::to_string(i + 1) +
                " expects '" + info.paramTypes[i] + "', got '" + argType + "'."
            );
        }
    }
}

void TypeChecker::checkChanDecl(ChanDeclNode* node) {
    symTable.define(node->name, "chan<" + node->elementType + ">", false);
}

void TypeChecker::checkChanSend(ChanSendNode* node) {
    SymbolInfo info = symTable.lookup(node->chanName);
    if (info.type.substr(0, 5) != "chan<") {
        throw std::runtime_error(
            "Type Error: '" + node->chanName + "' is not a channel."
        );
    }
    std::string elemType = info.type.substr(5, info.type.size() - 6);
    std::string valType  = checkExpr(node->value.get());
    if (valType != elemType) {
        throw std::runtime_error(
            "Type Error: Cannot send '" + valType + "' on channel<" + elemType + ">."
        );
    }
}

std::string TypeChecker::checkChanRecv(ChanRecvNode* node) {
    SymbolInfo info = symTable.lookup(node->chanName);
    if (info.type.substr(0, 5) != "chan<") {
        throw std::runtime_error(
            "Type Error: '" + node->chanName + "' is not a channel."
        );
    }
    std::string elemType = info.type.substr(5, info.type.size() - 6);
    node->resolvedType = elemType;
    return elemType;
}

std::string TypeChecker::checkUnaryOp(UnaryOpNode* node) {
    std::string type = checkExpr(node->operand.get());
    if (node->op == "-") {
        if (type != "int" && type != "float" && type != "double") {
            throw std::runtime_error("Type Error: unary '-' requires numeric type, got '" + type + "'.");
        }
        node->resolvedType = type;
        return type;
    }
    if (node->op == "!") {
        if (type != "bool") {
            throw std::runtime_error("Type Error: unary '!' requires bool, got '" + type + "'.");
        }
        node->resolvedType = "bool";
        return "bool";
    }
    throw std::runtime_error("Type Error: unknown unary operator '" + node->op + "'.");
}