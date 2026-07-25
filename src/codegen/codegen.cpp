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

std::string CodeGen::generate(ProgramNode* program) {
    out << "#include <stdio.h>\n";
    out << "#include <stdlib.h>\n";
    out << "#include <math.h>\n";
    out << "#include <pthread.h>\n";
    out << "#include \"runtime/concurrency/channel_api.h\"\n\n";
    genProgram(program);
    return out.str();
}

std::string CodeGen::mapType(const std::string& qtype) {
    if (qtype == "int")    return "int";
    if (qtype == "float")  return "float";
    if (qtype == "double") return "double";
    if (qtype == "string") return "char*";
    if (qtype == "bool")   return "int";
    if (qtype == "void")   return "void";
    return qtype; // user-defined struct names pass through
}

void CodeGen::genProgram(ProgramNode* node) {
    hasGlobals = !node->globals.empty();

    for (auto& decl : node->globals) {
        auto* ch = static_cast<ChanDeclNode*>(decl.get());
        out << "Channel " << ch->name << ";\n";    
    }

    if (hasGlobals) {
       out << "\nstatic void __q_init_globals(void) {\n";
        indentLevel++;
        for (auto& decl : node->globals) {
            auto* ch = static_cast<ChanDeclNode*>(decl.get());
            emit("channel_init(&" + ch->name + ");");
        }
        indentLevel--;
        out << "}\n\n";
    }

    for (auto& fn : node->functions) {
        genFunction(static_cast<FunctionDefNode*>(fn.get()));
        out << "\n";
    }
}

void CodeGen::genFunction(FunctionDefNode* node) {
    inSecureContext = node->isSecure;
    // Return type and name
    out << mapType(node->returnType) << " " << node->name << "(";

    // Parameters
    for (size_t i = 0; i < node->params.size(); i++) {
        out << mapType(node->params[i].second) << " " << node->params[i].first;
        if (i + 1 < node->params.size()) out << ", ";
    }
    out << ") ";

    if (node->name == "main") {
        emit("{");
        indentLevel++;
        if (!node->body) {
            throw std::runtime_error("CodeGen: main function has no body");
        }
        if (node->body->kind != NodeType::Block) {
            throw std::runtime_error("CodeGen: main body must be a block");
        }
        if (hasGlobals) {
            emit("__q_init_globals();");
        }
        auto* block = static_cast<BlockNode*>(node->body.get());
        for (auto& stmt : block->statements) {
            genStatement(stmt.get());
        }
        indentLevel--;
        emit("}");
    } else {
        genBlock(static_cast<BlockNode*>(node->body.get()));
    }

    out << "\n";

    // pthread-compatible wrapper, only useful if called via `go`
    if (!node->params.empty()) {
        out << "void* " << node->name << "_thread(void* arg) {\n";
        out << "    int " << node->params[0].first << " = *(int*)arg;\n";
        out << "    free(arg);\n";
        out << "    " << node->name << "(" << node->params[0].first << ");\n";
        out << "    return NULL;\n";
        out << "}\n";
    } else {
        out << "void* " << node->name << "_thread(void* arg) {\n";
        out << "    " << node->name << "();\n";
        out << "    return NULL;\n";
        out << "}\n";
    }
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
        case NodeType::ForStmt:
            genFor(static_cast<ForStmtNode*>(node));
            break;
        case NodeType::WhileStmt:
            genWhile(static_cast<WhileStmtNode*>(node));
            break;
        case NodeType::AssignStmt:
            genAssign(static_cast<AssignStmtNode*>(node));
            break;
        case NodeType::FunctionCall:
            emit(genExpr(node) + ";");
            break;
        case NodeType::FreeStmt:
            genFree(static_cast<FreeStmtNode*>(node));
            break;
        case NodeType::GoStmt:
            genGo(static_cast<GoStmtNode*>(node));
            break;
        case NodeType::ChanDecl:
            genChanDecl(static_cast<ChanDeclNode*>(node));
            break;
        case NodeType::ChanSend:
            genChanSend(static_cast<ChanSendNode*>(node));
            break;
        default:
            // expression statement (e.g. a bare expression)
            emit(genExpr(node) + ";");
    }
}

void CodeGen::genVarDecl(VarDeclNode* node) {
    if (node->type.substr(0, 5) == "heap<") {
        std::string elemType = mapType(node->heapElementType);
        std::string size = genExpr(node->initializer.get());
        // check if inside @Secure — tag as secure_region
        if (inSecureContext) {
            emit("// secure_region");
        }
        emit(elemType + "* " + node->name + " = (" + elemType + "*)malloc(" + size + " * sizeof(" + elemType + "));");
    } else {
        std::string ctype = mapType(node->type);
        std::string val = genExpr(node->initializer.get());
        emit(ctype + " " + node->name + " = " + val + ";");
    }
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

void CodeGen::genFor(ForStmtNode* node) {
    std::string var   = node->varName;
    std::string start = genExpr(node->rangeStart.get());
    std::string end   = genExpr(node->rangeEnd.get());
    emit("for (int " + var + " = " + start + "; " + var + " < " + end + "; " + var + "++) ");
    indentLevel--;
    genBlock(static_cast<BlockNode*>(node->body.get()));
    indentLevel++;
}

void CodeGen::genWhile(WhileStmtNode* node) {
    emit("while (" + genExpr(node->condition.get()) + ") ");
    indentLevel--;
    genBlock(static_cast<BlockNode*>(node->body.get()));
    indentLevel++;
}

void CodeGen::genAssign(AssignStmtNode* node) {
    emit(node->name + " = " + genExpr(node->value.get()) + ";");
}

void CodeGen::genFree(FreeStmtNode* node) {
    emit("free(" + node->varName + ");");
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

            if (n->callee == "wait_all") {
                std::string joins;
                for (auto& t : spawnedThreads) {
                    joins += "pthread_join(" + t + ", NULL); ";
                }
                spawnedThreads.clear();
                return joins.empty() ? "" : joins.substr(0, joins.size() - 1);
            }

            std::string call = n->callee + "(";
            for (size_t i = 0; i < n->args.size(); i++) {
                call += genExpr(n->args[i].get());
                if (i + 1 < n->args.size()) call += ", ";
            }
            return call + ")";
        }

        case NodeType::IndexExpr: {
            auto* n = static_cast<IndexExprNode*>(node);
            return n->varName + "[" + genExpr(n->index.get()) + "]";
        } 
        
        case NodeType::ChanRecv: {
            auto* n = static_cast<ChanRecvNode*>(node);
            return "channel_recv(&" + n->chanName + ")";
        }

        case NodeType::BoolLiteral:
            return static_cast<BoolLiteralNode*>(node)->value ? "1" : "0";

        case NodeType::UnaryOp: {
            auto* n = static_cast<UnaryOpNode*>(node);
            std::string operand = genExpr(n->operand.get());
            if (n->op == "-") return "(-" + operand + ")";
            if (n->op == "!") return "(!" + operand + ")";
            throw std::runtime_error("CodeGen: unknown unary operator");
        }

        default:
            throw std::runtime_error("CodeGen: unknown expression node");
    }
}

void CodeGen::genGo(GoStmtNode* node) {
    std::string threadVar = "t" + std::to_string(threadCounter++);
    emit("pthread_t " + threadVar + ";");

    if (node->args.empty()) {
        emit("pthread_create(&" + threadVar + ", NULL, (void*(*)(void*))" + node->funcName + "_thread, NULL);");
    } else {
        // pack single int arg via pointer
        std::string argVal = genExpr(node->args[0].get());
        std::string argVar = "arg" + std::to_string(threadCounter);
        emit("int* " + argVar + " = malloc(sizeof(int)); *" + argVar + " = " + argVal + ";");
        emit("pthread_create(&" + threadVar + ", NULL, " + node->funcName + "_thread, " + argVar + ");");
    }
    spawnedThreads.push_back(threadVar);
}

void CodeGen::genChanDecl(ChanDeclNode* node) {
    emit("Channel " + node->name + ";");
    emit("channel_init(&" + node->name + ");");
}

void CodeGen::genChanSend(ChanSendNode* node) {
    emit("channel_send(&" + node->chanName + ", " + genExpr(node->value.get()) + ");");
}

// Map Quantum's print(x) to the right printf format string
std::string CodeGen::buildPrintf(FunctionCallNode* node) {
    if (node->args.empty()) return "printf(\"\\n\")";

    auto* arg = node->args[0].get();
    std::string val = genExpr(arg);
    std::string fmt;

    if (arg->kind == NodeType::IntLiteral) {
        fmt = "%d";
    } else if (arg->kind == NodeType::FloatLiteral) {
        fmt = "%f";
    } else if (arg->kind == NodeType::BoolLiteral) {
        fmt = "%d";
    } else if (arg->kind == NodeType::StringLiteral) {
        fmt = "%s";
    } else if (arg->kind == NodeType::Identifier) {
        std::string type = static_cast<IdentifierNode*>(arg)->resolvedType;
        if (type == "float" || type == "double") fmt = "%f";
        else if (type == "string")               fmt = "%s";
        else                                      fmt = "%d";
    }

    return "printf(\"" + fmt + "\\n\", " + val + ")";
}
