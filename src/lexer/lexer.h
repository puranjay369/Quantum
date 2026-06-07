#pragma once
#include <string>
#include <vector>
#include "token.h"

class Lexer {
public:
    explicit Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t      pos  = 0;
    int         line = 1;
    int         col  = 1;

    char        peek();
    char        peek2();
    char        advance();
    bool        isAtEnd();

    void        skipWhitespaceAndComments();
    Token       readNumber();
    Token       readString();
    Token       readIdentOrKeyword();
    TokenType   keywordOrIdent(const std::string& word);

};