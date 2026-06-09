#include "lexer.h"
#include <cctype>
#include <sstream>

Lexer::Lexer(std::string source) : src(std::move(source)) {}

char Lexer::peek(int offset) const {
    size_t i = pos + offset;
    return (i < src.size()) ? src[i] : '\0';
}

char Lexer::advance() {
    char c = src[pos++];
    if (c == '\n') { ++line; col = 1; }
    else           { ++col; }
    return c;
}

void Lexer::skipWhitespaceAndComments() {
    while (pos < src.size()) {
        char c = peek();
        if (std::isspace(c)) {
            advance();
        } else if (c == '/' && peek(1) == '/') {
            // line comment
            while (pos < src.size() && peek() != '\n') advance();
        } else if (c == '/' && peek(1) == '*') {
            // block comment
            advance(); advance();
            while (pos + 1 < src.size()) {
                if (peek() == '*' && peek(1) == '/') {
                    advance(); advance(); break;
                }
                advance();
            }
        } else {
            break;
        }
    }
}

Token Lexer::readNumber() {
    int startLine = line, startCol = col;
    std::string s;
    while (pos < src.size() && std::isdigit(peek()))
        s += advance();
    return Token(TokenType::INT_LITERAL, s, startLine, startCol);
}

Token Lexer::readString() {
    int startLine = line, startCol = col;
    advance(); // consume opening "
    std::string s;
    while (pos < src.size() && peek() != '"') {
        char c = advance();
        if (c == '\\') {
            char esc = advance();
            switch (esc) {
                case 'n':  s += '\n'; break;
                case 't':  s += '\t'; break;
                case '"':  s += '"';  break;
                case '\\': s += '\\'; break;
                default:   s += esc;
            }
        } else {
            s += c;
        }
    }
    if (pos >= src.size())
        throw LexError("Unterminated string literal", startLine, startCol);
    advance(); // consume closing "
    return Token(TokenType::STRING_LITERAL, s, startLine, startCol);
}

Token Lexer::readIdentifierOrKeyword() {
    int startLine = line, startCol = col;
    std::string s;
    while (pos < src.size() && (std::isalnum(peek()) || peek() == '_'))
        s += advance();
    TokenType t = lookupKeyword(s);
    return Token(t, s, startLine, startCol);
}

Token Lexer::readOperatorOrDelimiter() {
    int startLine = line, startCol = col;
    char c = advance();
    char n = peek();

    // Two-char operators
    if (c == '=' && n == '=') { advance(); return {TokenType::EQ,        "==", startLine, startCol}; }
    if (c == '!' && n == '=') { advance(); return {TokenType::NEQ,       "!=", startLine, startCol}; }
    if (c == '<' && n == '=') { advance(); return {TokenType::LTE,       "<=", startLine, startCol}; }
    if (c == '>' && n == '=') { advance(); return {TokenType::GTE,       ">=", startLine, startCol}; }
    if (c == '&' && n == '&') { advance(); return {TokenType::AND,       "&&", startLine, startCol}; }
    if (c == '|' && n == '|') { advance(); return {TokenType::OR,        "||", startLine, startCol}; }
    if (c == '+' && n == '+') { advance(); return {TokenType::INCREMENT, "++", startLine, startCol}; }
    if (c == '-' && n == '-') { advance(); return {TokenType::DECREMENT, "--", startLine, startCol}; }
    if (c == '+' && n == '=') { advance(); return {TokenType::PLUS_ASSIGN, "+=", startLine, startCol}; }
    if (c == '-' && n == '=') { advance(); return {TokenType::MINUS_ASSIGN, "-=", startLine, startCol}; }

    // Single-char
    switch (c) {
        case '+': return {TokenType::PLUS,     "+", startLine, startCol};
        case '-': return {TokenType::MINUS,    "-", startLine, startCol};
        case '*': return {TokenType::STAR,     "*", startLine, startCol};
        case '/': return {TokenType::SLASH,    "/", startLine, startCol};
        case '%': return {TokenType::PERCENT,  "%", startLine, startCol};
        case '<': return {TokenType::LT,       "<", startLine, startCol};
        case '>': return {TokenType::GT,       ">", startLine, startCol};
        case '!': return {TokenType::NOT,      "!", startLine, startCol};
        case '=': return {TokenType::ASSIGN,   "=", startLine, startCol};
        case '(': return {TokenType::LPAREN,   "(", startLine, startCol};
        case ')': return {TokenType::RPAREN,   ")", startLine, startCol};
        case '{': return {TokenType::LBRACE,   "{", startLine, startCol};
        case '}': return {TokenType::RBRACE,   "}", startLine, startCol};
        case '[': return {TokenType::LBRACKET, "[", startLine, startCol};
        case ']': return {TokenType::RBRACKET, "]", startLine, startCol};
        case ';': return {TokenType::SEMICOLON,";", startLine, startCol};
        case ',': return {TokenType::COMMA,    ",", startLine, startCol};
        default: {
            std::string msg = "Unknown character: '";
            msg += c; msg += "'";
            throw LexError(msg, startLine, startCol);
        }
    }
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (true) {
        skipWhitespaceAndComments();
        if (pos >= src.size()) {
            tokens.emplace_back(TokenType::EOF_TOK, "", line, col);
            break;
        }
        char c = peek();
        if (std::isdigit(c)) {
            tokens.push_back(readNumber());
        } else if (c == '"') {
            tokens.push_back(readString());
        } else if (std::isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
        } else {
            tokens.push_back(readOperatorOrDelimiter());
        }
    }
    return tokens;
}

std::string Token::toString() const {
    return tokenTypeName(type) + "(\"" + value + "\") @" +
           std::to_string(line) + ":" + std::to_string(col);
}
