#pragma once
#include <string>
#include <unordered_map>

enum class TokenType {
    // Literals
    INT_LITERAL, STRING_LITERAL,

    // Identifiers & keywords
    IDENTIFIER,
    KW_INT, KW_CHAR, KW_VOID,
    KW_IF, KW_ELSE,
    KW_WHILE, KW_FOR,
    KW_RETURN,
    KW_PRINT,   // built-in print statement

    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQ, NEQ, LT, LTE, GT, GTE,
    AND, OR, NOT,
    ASSIGN,
    PLUS_ASSIGN, MINUS_ASSIGN,
    INCREMENT, DECREMENT,

    // Delimiters
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    LBRACKET, RBRACKET,
    SEMICOLON, COMMA,

    // Special
    EOF_TOK,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int col;

    Token(TokenType t, std::string v, int l, int c)
        : type(t), value(std::move(v)), line(l), col(c) {}

    std::string toString() const;
};

// Maps keyword strings to token types
inline TokenType lookupKeyword(const std::string& s) {
    static const std::unordered_map<std::string, TokenType> kw = {
        {"int",    TokenType::KW_INT},
        {"char",   TokenType::KW_CHAR},
        {"void",   TokenType::KW_VOID},
        {"if",     TokenType::KW_IF},
        {"else",   TokenType::KW_ELSE},
        {"while",  TokenType::KW_WHILE},
        {"for",    TokenType::KW_FOR},
        {"return", TokenType::KW_RETURN},
        {"print",  TokenType::KW_PRINT},
    };
    auto it = kw.find(s);
    return (it != kw.end()) ? it->second : TokenType::IDENTIFIER;
}

inline std::string tokenTypeName(TokenType t) {
    switch (t) {
#define X(n) case TokenType::n: return #n;
        X(INT_LITERAL) X(STRING_LITERAL) X(IDENTIFIER)
        X(KW_INT) X(KW_CHAR) X(KW_VOID)
        X(KW_IF) X(KW_ELSE) X(KW_WHILE) X(KW_FOR) X(KW_RETURN) X(KW_PRINT)
        X(PLUS) X(MINUS) X(STAR) X(SLASH) X(PERCENT)
        X(EQ) X(NEQ) X(LT) X(LTE) X(GT) X(GTE)
        X(AND) X(OR) X(NOT) X(ASSIGN)
        X(PLUS_ASSIGN) X(MINUS_ASSIGN) X(INCREMENT) X(DECREMENT)
        X(LPAREN) X(RPAREN) X(LBRACE) X(RBRACE)
        X(LBRACKET) X(RBRACKET) X(SEMICOLON) X(COMMA)
        X(EOF_TOK) X(UNKNOWN)
#undef X
        default: return "?";
    }
}
