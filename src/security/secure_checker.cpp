#include "secure_checker.h"

void SecureChecker::reject(const std::string& msg, int line) {
    throw std::runtime_error(
        "@Secure violation at line " + std::to_string(line) + ": " + msg
    );
}

void SecureChecker::check(ProgramNode* program) {
    for (auto& fn : program->functions) {
        checkFunction(static_cast<FunctionDefNode*>(fn.get()));
    }
}

void SecureChecker::checkFunction(FunctionDefNode* node) {
    inSecureFunction = node->isSecure;
    checkBlock(static_cast<BlockNode*>(node->body.get()));
    inSecureFunction = false;
}

void SecureChecker::checkBlock(BlockNode* node) {
    for (auto& stmt : node->statements) {
        checkStatement(stmt.get());
    }
}

void SecureChecker::checkStatement(ASTNode* node) {
    if (!inSecureFunction) return; // only enforce inside @Secure

    switch (node->kind) {
        case NodeType::VarDecl:
            checkVarDecl(static_cast<VarDeclNode*>(node));
            break;
        case NodeType::ReturnStmt:
            checkExpr(static_cast<ReturnStmtNode*>(node)->value.get());
            break;
        case NodeType::IfStmt: {
            auto* n = static_cast<IfStmtNode*>(node);
            checkExpr(n->condition.get());
            checkBlock(static_cast<BlockNode*>(n->thenBlock.get()));
            if (n->elseBlock)
                checkBlock(static_cast<BlockNode*>(n->elseBlock.get()));
            break;
        }
        case NodeType::FunctionCall:
            checkExpr(node);
            break;
        default:
            break;
    }
}

void SecureChecker::checkVarDecl(VarDeclNode* node) {
    if (node->type.substr(0, 5) == "heap<") {
        reject("heap allocation is not allowed in @Secure functions", node->line);
    }
    if (node->initializer) checkExpr(node->initializer.get());
}

void SecureChecker::checkExpr(ASTNode* node) {
    if (!node) return;

    switch (node->kind) {
        case NodeType::BinOp: {
            auto* n = static_cast<BinOpNode*>(node);
            // No pointer arithmetic — reject & and * used as unary pointer ops
            // For now reject explicit unsafe ops if we add them later
            checkExpr(n->left.get());
            checkExpr(n->right.get());
            break;
        }
        case NodeType::FunctionCall: {
            auto* n = static_cast<FunctionCallNode*>(node);
            // No free() inside @Secure
            if (n->callee == "free") {
                reject("free() is not allowed in @Secure functions", n->line);
            }
            for (auto& arg : n->args) checkExpr(arg.get());
            break;
        }
        default:
            break;
    }
}


