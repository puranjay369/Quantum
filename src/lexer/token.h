#pragma once
#include <string>
#include "token_types.h"

// Represents a single lexical unit with source context (for error reporting).
struct Token {
    TokenType   type;
    std::string value;
    int         line;
    int         col;

    Token(TokenType type, std::string value, int line, int col)
        : type(type), value(std::move(value)), line(line), col(col) {}
};