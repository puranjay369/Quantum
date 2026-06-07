#include "parser.h"
#include <stdexcept>

Parser::Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

Token& Parser::peek()  { return tokens[pos]; }
Token& Parser::peek2() { return tokens[std::min(pos+1, tokens.size()-1)]; }
bool   Parser::isAtEnd() { return peek().type == TokenType::EOF_TOKEN; }

bool Parser::check(TokenType t) { return peek().type == t; }

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

Token Parser::advance() {
    Token t = tokens[pos];
    if (!isAtEnd()) pos++;
    return t;
}

Token Parser::expect(TokenType t, const std::string& msg) {
    if (!check(t)) {
        auto& tok = peek();
        throw std::runtime_error("Line " + std::to_string(tok.line) +
                                 ": Expected " + msg + ", got '" + tok.value + "'");
    }
    return advance();
}

std::unique_ptr<ProgramNode> Parser::parse() {
    return parseProgram();
}

std::unique_ptr<ProgramNode> Parser::parseProgram() {
    auto prog = std::make_unique<ProgramNode>();
    prog->kind = NodeType::Program;
    while (!isAtEnd()) {
        bool secure = false;
        if (check(TokenType::AT)) {
            advance(); // consume @
            expect(TokenType::IDENT, "annotation name"); // "Secure"
            secure = true;
        }
        if (check(TokenType::KW_FN)) {
            prog->functions.push_back(parseFunctionDef(secure));
        } else {
            throw std::runtime_error("Expected fn definition");
        }
    }
    return prog;
}

std::unique_ptr<FunctionDefNode> Parser::parseFunctionDef(bool isSecure) {
    expect(TokenType::KW_FN, "fn");
    auto fn = std::make_unique<FunctionDefNode>();
    fn->kind = NodeType::FunctionDef;
    fn->isSecure = isSecure;
    fn->name = expect(TokenType::IDENT, "function name").value;

    expect(TokenType::LPAREN, "(");
    while (!check(TokenType::RPAREN) && !isAtEnd()) {
        std::string pName = expect(TokenType::IDENT, "parameter name").value;
        expect(TokenType::COLON, ":");
        std::string pType = parseTypeName();
        fn->params.push_back({pName, pType});
        if (!check(TokenType::RPAREN)) expect(TokenType::COMMA, ",");
    }
    expect(TokenType::RPAREN, ")");

    fn->returnType = "void";
    if (match(TokenType::ARROW)) {
        fn->returnType = parseTypeName();
    }

    fn->body = parseBlock();
    return fn;
}

std::unique_ptr<BlockNode> Parser::parseBlock() {
    expect(TokenType::LBRACE, "{");
    auto block = std::make_unique<BlockNode>();
    block->kind = NodeType::Block;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        block->statements.push_back(parseStatement());
    }
    expect(TokenType::RBRACE, "}");
    return block;
}

NodePtr Parser::parseStatement() {
    if (check(TokenType::KW_LET))    return parseVarDecl();
    if (check(TokenType::KW_RETURN)) return parseReturnStmt();
    if (check(TokenType::KW_IF))     return parseIfStmt();
    // expression statement (e.g. a function call)
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, ";");
    return expr;
}

NodePtr Parser::parseVarDecl() {
    auto node = std::make_unique<VarDeclNode>();
    node->kind = NodeType::VarDecl;
    node->line = peek().line;
    expect(TokenType::KW_LET, "let");
    node->name = expect(TokenType::IDENT, "variable name").value;
    expect(TokenType::COLON, ":");
    node->type = parseTypeName();
    expect(TokenType::ASSIGN, "=");
    node->initializer = parseExpression();
    expect(TokenType::SEMICOLON, ";");
    return node;
}

NodePtr Parser::parseReturnStmt() {
    auto node = std::make_unique<ReturnStmtNode>();
    node->kind = NodeType::ReturnStmt;
    expect(TokenType::KW_RETURN, "return");
    node->value = parseExpression();
    expect(TokenType::SEMICOLON, ";");
    return node;
}

NodePtr Parser::parseIfStmt() {
    auto node = std::make_unique<IfStmtNode>();
    node->kind = NodeType::IfStmt;
    expect(TokenType::KW_IF, "if");
    expect(TokenType::LPAREN, "(");
    node->condition = parseExpression();
    expect(TokenType::RPAREN, ")");
    node->thenBlock = parseBlock();
    node->elseBlock = nullptr;
    if (match(TokenType::KW_ELSE)) {
        node->elseBlock = parseBlock();
    }
    return node;
}

// Expressions: precedence handled by calling chain
// parseExpression → parseComparison → parseAddSub → parseMulDiv → parsePrimary

NodePtr Parser::parseExpression() { return parseComparison(); }

NodePtr Parser::parseComparison() {
    auto left = parseAddSub();
    while (check(TokenType::EQ)  || check(TokenType::NEQ) ||
           check(TokenType::LT)  || check(TokenType::GT)  ||
           check(TokenType::LTE) || check(TokenType::GTE)) {
        auto node = std::make_unique<BinOpNode>();
        node->kind = NodeType::BinOp;
        node->op   = advance().value;
        node->left = std::move(left);
        node->right = parseAddSub();
        left = std::move(node);
    }
    return left;
}

NodePtr Parser::parseAddSub() {
    auto left = parseMulDiv();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        auto node = std::make_unique<BinOpNode>();
        node->kind = NodeType::BinOp;
        node->op   = advance().value;
        node->left = std::move(left);
        node->right = parseMulDiv();
        left = std::move(node);
    }
    return left;
}

NodePtr Parser::parseMulDiv() {
    auto left = parsePrimary();
    while (check(TokenType::STAR) || check(TokenType::SLASH)) {
        auto node = std::make_unique<BinOpNode>();
        node->kind = NodeType::BinOp;
        node->op   = advance().value;
        node->left = std::move(left);
        node->right = parsePrimary();
        left = std::move(node);
    }
    return left;
}

NodePtr Parser::parsePrimary() {
    // Integer literal
    if (check(TokenType::INT_LITERAL)) {
        auto node = std::make_unique<IntLiteralNode>();
        node->kind  = NodeType::IntLiteral;
        node->value = std::stoi(advance().value);
        return node;
    }
    // Float literal
    if (check(TokenType::FLOAT_LITERAL)) {
        auto node = std::make_unique<FloatLiteralNode>();
        node->kind  = NodeType::FloatLiteral;
        node->value = std::stod(advance().value);
        return node;
    }
    // String literal
    if (check(TokenType::STRING_LITERAL)) {
        auto node = std::make_unique<StringLiteralNode>();
        node->kind  = NodeType::StringLiteral;
        node->value = advance().value;
        return node;
    }
    // Identifier or function call
    if (check(TokenType::IDENT)) {
        std::string name = advance().value;
        if (match(TokenType::LPAREN)) {
            auto node = std::make_unique<FunctionCallNode>();
            node->kind   = NodeType::FunctionCall;
            node->callee = name;
            while (!check(TokenType::RPAREN) && !isAtEnd()) {
                node->args.push_back(parseExpression());
                if (!check(TokenType::RPAREN)) expect(TokenType::COMMA, ",");
            }
            expect(TokenType::RPAREN, ")");
            return node;
        }
        auto node = std::make_unique<IdentifierNode>();
        node->kind = NodeType::Identifier;
        node->name = name;
        return node;
    }
    // Grouped expression: (expr)
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        expect(TokenType::RPAREN, ")");
        return expr;
    }
    throw std::runtime_error("Line " + std::to_string(peek().line) +
                             ": Unexpected token '" + peek().value + "'");
}

std::string Parser::parseTypeName() {
    if (check(TokenType::TYPE_INT))    { advance(); return "int"; }
    if (check(TokenType::TYPE_FLOAT))  { advance(); return "float"; }
    if (check(TokenType::TYPE_STRING)) { advance(); return "string"; }
    if (check(TokenType::TYPE_BOOL))   { advance(); return "bool"; }
    if (check(TokenType::IDENT))       { return advance().value; }
    throw std::runtime_error("Expected type name, got '" + peek().value + "'");
}