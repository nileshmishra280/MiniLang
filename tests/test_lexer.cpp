#include "lexer.h"
#include <cassert>
#include <iostream>
#include <string>

static int passed = 0, failed = 0;

#define CHECK(cond, msg) \
    do { if (!(cond)) { std::cerr << "FAIL: " << msg << "\n"; ++failed; } \
         else { ++passed; } } while(0)

void test_basic_tokens() {
    Lexer lex("int x = 42;");
    auto toks = lex.tokenize();
    CHECK(toks[0].type == TokenType::KW_INT,      "int keyword");
    CHECK(toks[1].type == TokenType::IDENTIFIER,  "identifier x");
    CHECK(toks[1].value == "x",                   "identifier value");
    CHECK(toks[2].type == TokenType::ASSIGN,      "assign =");
    CHECK(toks[3].type == TokenType::INT_LITERAL, "int literal");
    CHECK(toks[3].value == "42",                  "literal value");
    CHECK(toks[4].type == TokenType::SEMICOLON,   "semicolon");
    CHECK(toks[5].type == TokenType::EOF_TOK,     "EOF");
}

void test_operators() {
    Lexer lex("== != <= >= && || ++ --");
    auto toks = lex.tokenize();
    CHECK(toks[0].type == TokenType::EQ,        "==");
    CHECK(toks[1].type == TokenType::NEQ,       "!=");
    CHECK(toks[2].type == TokenType::LTE,       "<=");
    CHECK(toks[3].type == TokenType::GTE,       ">=");
    CHECK(toks[4].type == TokenType::AND,       "&&");
    CHECK(toks[5].type == TokenType::OR,        "||");
    CHECK(toks[6].type == TokenType::INCREMENT, "++");
    CHECK(toks[7].type == TokenType::DECREMENT, "--");
}

void test_string_literal() {
    Lexer lex(R"("hello\nworld")");
    auto toks = lex.tokenize();
    CHECK(toks[0].type == TokenType::STRING_LITERAL, "string literal");
    CHECK(toks[0].value == "hello\nworld",           "escape sequence");
}

void test_line_comment() {
    Lexer lex("int x; // this is a comment\nint y;");
    auto toks = lex.tokenize();
    CHECK(toks[0].type == TokenType::KW_INT,     "int before comment");
    CHECK(toks[2].type == TokenType::SEMICOLON,  "semicolon before comment");
    CHECK(toks[3].type == TokenType::KW_INT,     "int after comment");
}

void test_block_comment() {
    Lexer lex("int /* comment */ x;");
    auto toks = lex.tokenize();
    CHECK(toks[0].type == TokenType::KW_INT,    "int");
    CHECK(toks[1].type == TokenType::IDENTIFIER,"x after block comment");
}

void test_keywords() {
    Lexer lex("if else while for return void char print");
    auto toks = lex.tokenize();
    CHECK(toks[0].type == TokenType::KW_IF,     "if");
    CHECK(toks[1].type == TokenType::KW_ELSE,   "else");
    CHECK(toks[2].type == TokenType::KW_WHILE,  "while");
    CHECK(toks[3].type == TokenType::KW_FOR,    "for");
    CHECK(toks[4].type == TokenType::KW_RETURN, "return");
    CHECK(toks[5].type == TokenType::KW_VOID,   "void");
    CHECK(toks[6].type == TokenType::KW_CHAR,   "char");
    CHECK(toks[7].type == TokenType::KW_PRINT,  "print");
}

void test_line_numbers() {
    Lexer lex("int x;\nint y;\nint z;");
    auto toks = lex.tokenize();
    CHECK(toks[0].line == 1, "line 1");
    CHECK(toks[3].line == 2, "line 2");
    CHECK(toks[6].line == 3, "line 3");
}

void test_lex_error() {
    Lexer lex("int @x;");
    bool threw = false;
    try { lex.tokenize(); }
    catch (LexError&) { threw = true; }
    CHECK(threw, "LexError on unknown char @");
}

int main() {
    test_basic_tokens();
    test_operators();
    test_string_literal();
    test_line_comment();
    test_block_comment();
    test_keywords();
    test_line_numbers();
    test_lex_error();

    std::cout << "Lexer tests: " << passed << " passed, " << failed << " failed\n";
    return (failed == 0) ? 0 : 1;
}
