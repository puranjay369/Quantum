#pragma once 

enum class TokenType {

    INT_LITERAL,        
    FLOAT_LITERAL,
    STRING_LITERAL,

    KW_LET,
    KW_FN,
    KW_RETURN,
    KW_IF,
    KW_ELSE,
    KW_FOR,
    KW_WHILE,
    KW_IN,
    KW_GO,
    KW_STRUCT,
    KW_IMPL,
    KW_HEAP,
    KW_FREE,
    KW_SECURE,

    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_BOOL,

    IDENT,

    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    CARET,
    EQ,
    NEQ,
    LT,
    GT,
    LTE,
    GTE,
    ASSIGN,
    ARROW,
    DOTDOT,

    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    SEMICOLON,
    COLON,
    COMMA,
    DOT,
    AT,

    EOF_TOKEN,
    UNKNOWN

};