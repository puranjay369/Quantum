#include "codegen.h"
#include <stdexcept>

std::string CodeGen::indent() {
    return std::string(indentLevel * 4, ' ');
}

void CodeGen::emit(const std::string& s) {
    out << indent() << s << "\n";
}

void CodeGen::emitRaw(const std::string& s) {
    out << s;
}

std::string CodeGen::mapType(const std::string& qtype) {
    if (qtype == "int")    return "int";
    if (qtype == "float")  return "double";
    if (qtype == "string") return "char*";
    if (qtype == "bool")   return "int";
    if (qtype == "void")   return "void";
    return qtype; // user-defined struct names pass through
}

std::string CodeGen::generate(ProgramNode* program) {
    // Standard C headers every Quantum program needs
    out << "#include <stdio.h>\n";
    out << "#include <math.h>\n\n";
    genProgram(program);
    return out.str();
}

void CodeGen::genProgram(ProgramNode* node) {
    for (auto& fn : node->functions) {
        genFunction(static_cast<FunctionDefNode*>(fn.get()));
        out << "\n";
    }
}

void CodeGen::genFunction(FunctionDefNode* node) {
    // Return type and name
    out << mapType(node->returnType) << " " << node->name << "(";

    // Parameters
    for (size_t i = 0; i < node->params.size(); i++) {
        out << mapType(node->params[i].second) << " " << node->params[i].first;
        if (i + 1 < node->params.size()) out << ", ";
    }
    out << ") ";

    genBlock(static_cast<BlockNode*>(node->body.get()));
}

void CodeGen::genBlock(BlockNode* node) {
    emit("{");
    indentLevel++;
    for (auto& stmt : node->statements) {
        genStatement(stmt.get());
    }
    indentLevel--;
    emit("}");
}

void CodeGen::genStatement(ASTNode* node) {
    switch (node->kind) {
        case NodeType::VarDecl:
            genVarDecl(static_cast<VarDeclNode*>(node));
            break;
        case NodeType::ReturnStmt:
            genReturn(static_cast<ReturnStmtNode*>(node));
            break;
        case NodeType::IfStmt:
            genIf(static_cast<IfStmtNode*>(node));
            break;
        case NodeType::FunctionCall:
            emit(genExpr(node) + ";");
            break;
        default:
            // expression statement (e.g. a bare expression)
            emit(genExpr(node) + ";");
    }
}

void CodeGen::genVarDecl(VarDeclNode* node) {
    std::string ctype = mapType(node->type);
    std::string val   = genExpr(node->initializer.get());
    emit(ctype + " " + node->name + " = " + val + ";");
}

void CodeGen::genReturn(ReturnStmtNode* node) {
    emit("return " + genExpr(node->value.get()) + ";");
}

void CodeGen::genIf(IfStmtNode* node) {
    emit("if (" + genExpr(node->condition.get()) + ") ");
    indentLevel--; // genBlock adds its own indent
    genBlock(static_cast<BlockNode*>(node->thenBlock.get()));
    indentLevel++;
    if (node->elseBlock) {
        // back up one line to put else on same line as closing brace
        // simplest approach: just emit else on next line
        emit("else ");
        indentLevel--;
        genBlock(static_cast<BlockNode*>(node->elseBlock.get()));
        indentLevel++;
    }
}

std::string CodeGen::genExpr(ASTNode* node) {
    switch (node->kind) {

        case NodeType::IntLiteral:
            return std::to_string(static_cast<IntLiteralNode*>(node)->value);

        case NodeType::FloatLiteral:
            return std::to_string(static_cast<FloatLiteralNode*>(node)->value);

        case NodeType::StringLiteral:
            return "\"" + static_cast<StringLiteralNode*>(node)->value + "\"";

        case NodeType::Identifier:
            return static_cast<IdentifierNode*>(node)->name;

        case NodeType::BinOp: {
            auto* n = static_cast<BinOpNode*>(node);
            std::string l = genExpr(n->left.get());
            std::string r = genExpr(n->right.get());
            // Quantum uses ^ for power — map to pow()
            if (n->op == "^")
                return "pow(" + l + ", " + r + ")";
            return "(" + l + " " + n->op + " " + r + ")";
        }

        case NodeType::FunctionCall: {
            auto* n = static_cast<FunctionCallNode*>(node);

            // Map Quantum's print() to printf
            if (n->callee == "print") {
                return buildPrintf(n);
            }

            std::string call = n->callee + "(";
            for (size_t i = 0; i < n->args.size(); i++) {
                call += genExpr(n->args[i].get());
                if (i + 1 < n->args.size()) call += ", ";
            }
            return call + ")";
        }

        default:
            throw std::runtime_error("CodeGen: unknown expression node");
    }
}

// Map Quantum's print(x) to the right printf format string
std::string CodeGen::buildPrintf(FunctionCallNode* node) {
    if (node->args.empty()) return "printf(\"\\n\")";

    // Guess format specifier from the argument type
    // (full type inference comes in Phase 4 — for now we use heuristics)
    auto* arg = node->args[0].get();
    std::string fmt;
    std::string val = genExpr(arg);

    if (arg->kind == NodeType::IntLiteral)
        fmt = "%d";
    else if (arg->kind == NodeType::FloatLiteral)
        fmt = "%f";
    else if (arg->kind == NodeType::StringLiteral)
        fmt = "%s";
    else
        fmt = "%d"; // default to int for identifiers until type checker is in

    return "printf(\"" + fmt + "\\n\", " + val + ")";
}