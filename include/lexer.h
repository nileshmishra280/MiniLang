#pragma once
#include "token.h"
#include <string>
#include <vector>
#include <stdexcept>

struct LexError : std::runtime_error {
    int line, col;
    LexError(const std::string& msg, int l, int c)
        : std::runtime_error(msg), line(l), col(c) {}
};

class Lexer {
public:
    explicit Lexer(std::string source);
    std::vector<Token> tokenize();

private:
    std::string src;
    size_t pos = 0;
    int line = 1, col = 1;

    char peek(int offset = 0) const;
    char advance();
    void skipWhitespaceAndComments();
    Token readNumber();
    Token readString();
    Token readIdentifierOrKeyword();
    Token readOperatorOrDelimiter();
};
