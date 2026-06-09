#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>           // for system()
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "codegen/codegen.h"
#include "semantic/type_checker.h"

std::string tokenTypeName(TokenType t) {
    switch(t) {
        case TokenType::KW_LET:        return "KW_LET";
        case TokenType::KW_FN:         return "KW_FN";
        case TokenType::IDENT:         return "IDENT";
        case TokenType::INT_LITERAL:   return "INT_LITERAL";
        case TokenType::COLON:         return "COLON";
        case TokenType::TYPE_INT:      return "TYPE_INT";
        case TokenType::ASSIGN:        return "ASSIGN";
        case TokenType::SEMICOLON:     return "SEMICOLON";
        case TokenType::ARROW:         return "ARROW";
        case TokenType::EOF_TOKEN:     return "EOF";
        default:                       return "OTHER";
    }
}

void printAST(const ASTNode* node, int indent = 0) {
    std::string pad(indent * 2, ' ');
    if (!node) return;
    switch (node->kind) {
        case NodeType::Program: {
            auto* n = static_cast<const ProgramNode*>(node);
            std::cout << pad << "Program\n";
            for (auto& fn : n->functions) printAST(fn.get(), indent+1);
            break;
        }
        case NodeType::FunctionDef: {
            auto* n = static_cast<const FunctionDefNode*>(node);
            std::cout << pad << "FunctionDef: " << n->name
                      << (n->isSecure ? " [@Secure]" : "") << "\n";
            printAST(n->body.get(), indent+1);
            break;
        }
        case NodeType::Block: {
            auto* n = static_cast<const BlockNode*>(node);
            std::cout << pad << "Block\n";
            for (auto& s : n->statements) printAST(s.get(), indent+1);
            break;
        }
        case NodeType::VarDecl: {
            auto* n = static_cast<const VarDeclNode*>(node);
            std::cout << pad << "VarDecl: " << n->name << " : " << n->type << "\n";
            printAST(n->initializer.get(), indent+1);
            break;
        }
        case NodeType::ReturnStmt: {
            auto* n = static_cast<const ReturnStmtNode*>(node);
            std::cout << pad << "Return\n";
            printAST(n->value.get(), indent+1);
            break;
        }
        case NodeType::IfStmt: {
            auto* n = static_cast<const IfStmtNode*>(node);
            std::cout << pad << "IfStmt\n";
            std::cout << pad << "  condition:\n";
            printAST(n->condition.get(), indent+2);
            std::cout << pad << "  then:\n";
            printAST(n->thenBlock.get(), indent+2);
            if (n->elseBlock) {
                std::cout << pad << "  else:\n";
                printAST(n->elseBlock.get(), indent+2);
            }
            break;
        }
        case NodeType::BinOp: {
            auto* n = static_cast<const BinOpNode*>(node);
            std::cout << pad << "BinOp: " << n->op << "\n";
            printAST(n->left.get(),  indent+1);
            printAST(n->right.get(), indent+1);
            break;
        }
        case NodeType::IntLiteral: {
            auto* n = static_cast<const IntLiteralNode*>(node);
            std::cout << pad << "IntLiteral: " << n->value << "\n";
            break;
        }
        case NodeType::FloatLiteral: {
            auto* n = static_cast<const FloatLiteralNode*>(node);
            std::cout << pad << "FloatLiteral: " << n->value << "\n";
            break;
        }
        case NodeType::StringLiteral: {
            auto* n = static_cast<const StringLiteralNode*>(node);
            std::cout << pad << "StringLiteral: \"" << n->value << "\"\n";
            break;
        }
        case NodeType::Identifier: {
            auto* n = static_cast<const IdentifierNode*>(node);
            std::cout << pad << "Identifier: " << n->name << "\n";
            break;
        }
        case NodeType::FunctionCall: {
            auto* n = static_cast<const FunctionCallNode*>(node);
            std::cout << pad << "Call: " << n->callee << "\n";
            for (auto& a : n->args) printAST(a.get(), indent+1);
            break;
        }
        default:
            std::cout << pad << "(unknown node)\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "Usage: qcc <file.q>\n"; return 1; }

    std::ifstream file(argv[1]);
    std::stringstream buf;
    buf << file.rdbuf();

    Lexer lexer(buf.str());
    auto tokens = lexer.tokenize();

    Parser parser(tokens);
    auto ast = parser.parse();
    //printAST(ast.get());

    // Run Semantic Analysis (Type Checking)
    TypeChecker typeChecker;
    typeChecker.check(ast.get());

    // for (auto& tok : tokens) {
    //     std::cout << "[" << tokenTypeName(tok.type) << "] "
    //               << "\"" << tok.value << "\""
    //               << " (line " << tok.line << ", col " << tok.col << ")\n";
    // }

    // Generate C code
    CodeGen codegen;
    std::string cCode = codegen.generate(ast.get());

    // Write .c file
    std::ofstream cFile("output.c");
    cFile << cCode;
    cFile.close();

    // Compile with gcc
    int result = system("gcc output.c -o output -lm");
    if (result != 0) {
        std::cerr << "gcc compilation failed\n";
        return 1;
    }

    std::cout << "Compiled successfully → ./output\n";

    return 0;
}

