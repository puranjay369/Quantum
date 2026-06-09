#include "type_checker.h"
#include <iostream>

TypeChecker::TypeChecker() : currentReturnType("void") {
    // Pre-declare built-in global functions
    symTable.define("print", "void", true);
}

void TypeChecker::check(ProgramNode* program) {
    checkProgram(program);
}

void TypeChecker::checkProgram(ProgramNode* node) {
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

    checkBlock(static_cast<BlockNode*>(node->body.get()));

    symTable.exitScope();
}

void TypeChecker::checkBlock(BlockNode* node) {
    // Note: If a Block is a function's main body, we could avoid double-scoping,
    // but standard nested scoping (push/pop) hurts nothing and correctly sandboxes loops/ifs.
    symTable.enterScope();
    for (auto& stmt : node->statements) {
        checkStatement(stmt.get());
    }
    symTable.exitScope();
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
        case NodeType::FunctionCall: // standalone method execution
            checkExpr(node);         
            break;
        default:
            checkExpr(node);
    }
}

void TypeChecker::checkVarDecl(VarDeclNode* node) {
    // 1. Evaluate what the initializer code produces
    std::string exprType = checkExpr(node->initializer.get());

    // 2. See if it matches the left side declared type
    if (exprType != node->type) {
        throw std::runtime_error("Type Error: Cannot assign type '" + exprType + 
                                 "' to variable '" + node->name + "' of type '" + node->type + "'.");
    }

    // 3. Save it to our local scope
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

std::string TypeChecker::checkExpr(ASTNode* node) {
    switch (node->kind) {
        case NodeType::IntLiteral:   return "int";
        case NodeType::FloatLiteral: return "float";
        case NodeType::StringLiteral:return "string";
        
        case NodeType::Identifier: {
            auto* id = static_cast<IdentifierNode*>(node);
            return symTable.lookup(id->name).type; // Ensures variable actually exists!
        }
        case NodeType::BinOp:
            return checkBinOp(static_cast<BinOpNode*>(node));
            
        case NodeType::FunctionCall:
            return checkFunctionCall(static_cast<FunctionCallNode*>(node));

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