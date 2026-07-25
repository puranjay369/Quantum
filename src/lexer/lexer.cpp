#include "lexer.h"
#include <stdexcept>
#include <unordered_map>

Lexer::Lexer(const std::string& source) : source(source) {}

char Lexer::peek() {
    if (isAtEnd()) return '\0';
    return source[pos];
}
char Lexer::peeknext() {
    if(pos + 1 >= source.size()) return '\0';
    return source[pos + 1];
}


char Lexer::advance() {
    char c = source[pos++];
    if (c == '\n') { line++; col = 1; }
    else           { col++; }
    return c;
}

bool Lexer::isAtEnd() {
    return pos >= source.size();
}

void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && peeknext() == '/') {       
            while (!isAtEnd() && peek() != '\n') advance();
        } else {
            break;
        }
    }
}

Token Lexer::readNumber() {
    int startLine = line, startCol = col;
    std::string num;
    bool isFloat = false;
    while (!isAtEnd() && (isdigit(peek()) || peek() == '.')) {
        if (peek() == '.') {
            if (peeknext() == '.') break;
            isFloat = true;
        }
        num += advance();  
    }
    return Token(isFloat ? TokenType::FLOAT_LITERAL : TokenType::INT_LITERAL,num, startLine, startCol);
}

Token Lexer::readString() {
    int startLine = line, startCol = col;
    advance();
    std::string str;
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\\') { advance(); }
        str += advance();
    }
    if (!isAtEnd()) advance();
    return Token(TokenType::STRING_LITERAL, str, startLine, startCol);
}

TokenType Lexer::keywordOrIdent(const std::string& word){
    static const std::unordered_map<std::string, TokenType> keywords = {
        {"let",     TokenType::KW_LET},
        {"fn",      TokenType::KW_FN},
        {"return",  TokenType::KW_RETURN},
        {"if",      TokenType::KW_IF},
        {"else",    TokenType::KW_ELSE},
        {"for",     TokenType::KW_FOR},
        {"while",   TokenType::KW_WHILE},
        {"in",      TokenType::KW_IN},
        {"go",      TokenType::KW_GO},
        {"struct",  TokenType::KW_STRUCT},
        {"impl",    TokenType::KW_IMPL},
        {"heap",    TokenType::KW_HEAP},
        {"free",    TokenType::KW_FREE},
        {"int",     TokenType::TYPE_INT},
        {"float",   TokenType::TYPE_FLOAT},
        {"string",  TokenType::TYPE_STRING},
        {"bool",    TokenType::TYPE_BOOL},
        {"heap_alloc", TokenType::KW_HEAP_ALLOC},
        {"chan",    TokenType::KW_CHAN},
        {"true",    TokenType::KW_TRUE},
        {"false",   TokenType::KW_FALSE},
        {"double",  TokenType::TYPE_DOUBLE}
    };
    auto it = keywords.find(word);
    return (it != keywords.end()) ? it->second : TokenType::IDENT;
}

Token Lexer::readIdentOrKeyword() {
    int startLine = line, startCol = col;
    std::string word;
    while (!isAtEnd() && (isalnum(peek()) || peek() == '_')) {
        word += advance();
    }
    return Token(keywordOrIdent(word), word, startLine, startCol);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while(true){
        skipWhitespaceAndComments();
        if (isAtEnd()) {
            tokens.emplace_back(TokenType::EOF_TOKEN, "", line, col);
            break;
        }

        int startLine = line, startCol = col;
        char c = peek();

        if (isdigit(c)) { tokens.push_back(readNumber()); continue; }

        if (c == '"')   { tokens.push_back(readString()); continue; }

        if (isalpha(c) || c == '_') { tokens.push_back(readIdentOrKeyword()); continue; }

        if (c == '@') {
            advance();
            tokens.emplace_back(TokenType::AT, "@", startLine, startCol);
            continue;
        }

        advance();
        switch(c){
            case '-':
                if (peek() == '>') { advance(); tokens.emplace_back(TokenType::ARROW,  "->", startLine, startCol); }
                else               {            tokens.emplace_back(TokenType::MINUS,   "-",  startLine, startCol); }
                break;
            case '.':
                if (peek() == '.') { advance(); tokens.emplace_back(TokenType::DOTDOT, "..", startLine, startCol); }
                else               {            tokens.emplace_back(TokenType::DOT,     ".",  startLine, startCol); }
                break;
            case '=':
                if (peek() == '=') { advance(); tokens.emplace_back(TokenType::EQ,     "==", startLine, startCol); }
                else               {            tokens.emplace_back(TokenType::ASSIGN,  "=",  startLine, startCol); }
                break;
            case '!':
                if (peek() == '=') { advance(); tokens.emplace_back(TokenType::NEQ, "!=", startLine, startCol); }
                else               {            tokens.emplace_back(TokenType::NOT, "!",  startLine, startCol); }
            break;
            case '<':
                if (peek() == '=') { advance(); tokens.emplace_back(TokenType::LTE,    "<=", startLine, startCol); }
                else               {            tokens.emplace_back(TokenType::LT,      "<",  startLine, startCol); }
                break;
            case '>':
                if (peek() == '=') { advance(); tokens.emplace_back(TokenType::GTE,    ">=", startLine, startCol); }
                else               {            tokens.emplace_back(TokenType::GT,      ">",  startLine, startCol); }
                break;
            case '+': tokens.emplace_back(TokenType::PLUS,      "+", startLine, startCol); break;
            case '*': tokens.emplace_back(TokenType::STAR,      "*", startLine, startCol); break;
            case '/': tokens.emplace_back(TokenType::SLASH,     "/", startLine, startCol); break;
            case '%': tokens.emplace_back(TokenType::PERCENT,   "%", startLine, startCol); break;
            case '^': tokens.emplace_back(TokenType::CARET,     "^", startLine, startCol); break;
            case '(': tokens.emplace_back(TokenType::LPAREN,    "(", startLine, startCol); break;
            case ')': tokens.emplace_back(TokenType::RPAREN,    ")", startLine, startCol); break;
            case '{': tokens.emplace_back(TokenType::LBRACE,    "{", startLine, startCol); break;
            case '}': tokens.emplace_back(TokenType::RBRACE,    "}", startLine, startCol); break;
            case '[': tokens.emplace_back(TokenType::LBRACKET,  "[", startLine, startCol); break;
            case ']': tokens.emplace_back(TokenType::RBRACKET,  "]", startLine, startCol); break;
            case ';': tokens.emplace_back(TokenType::SEMICOLON, ";", startLine, startCol); break;
            case ':': tokens.emplace_back(TokenType::COLON,     ":", startLine, startCol); break;
            case ',': tokens.emplace_back(TokenType::COMMA,     ",", startLine, startCol); break;
            default:
                tokens.emplace_back(TokenType::UNKNOWN, std::string(1,c), startLine, startCol);
        }
    }
    return tokens;
}