#pragma once
#include <string>
#include <vector>
#include "token.h"

// Lexer breaks raw source code into a sequential stream of Tokens.
class Lexer {
public:
    explicit Lexer(const std::string& source);
    
    // Core entry point to process the entire source string
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t      pos  = 0;
    int         line = 1;
    int         col  = 1;

    char        peek();       // Look at current char
    char        peeknext();   // Look at next char (for multi-char tokens)
    char        advance();    // Consume and return current char
    bool        isAtEnd();

    void        skipWhitespaceAndComments();
    Token       readNumber();
    Token       readString();
    Token       readIdentOrKeyword();
    TokenType   keywordOrIdent(const std::string& word);
};