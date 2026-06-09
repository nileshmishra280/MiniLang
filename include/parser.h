#pragma once
#include "token.h"
#include "ast.h"
#include <vector>
#include <stdexcept>

struct ParseError : std::runtime_error {
    int line, col;
    ParseError(const std::string& msg, int l, int c)
        : std::runtime_error(msg), line(l), col(c) {}
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<Program> parse();

private:
    std::vector<Token> tokens;
    size_t pos = 0;

    Token& peek(int offset = 0);
    Token  consume();
    Token  expect(TokenType t, const std::string& msg = "");
    bool   check(TokenType t) const;
    bool   match(TokenType t);

    // Top-level
    std::unique_ptr<FuncDecl> parseFuncDecl();
    std::unique_ptr<VarDecl>  parseGlobalVar();

    // Statements
    std::unique_ptr<Block>      parseBlock();
    NodePtr                     parseStatement();
    std::unique_ptr<VarDecl>    parseLocalVar();
    std::unique_ptr<IfStmt>     parseIf();
    std::unique_ptr<WhileStmt>  parseWhile();
    std::unique_ptr<ForStmt>    parseFor();
    std::unique_ptr<ReturnStmt> parseReturn();
    std::unique_ptr<PrintStmt>  parsePrint();

    // Expressions (precedence climbing)
    NodePtr parseExpr();
    NodePtr parseAssign();
    NodePtr parseOr();
    NodePtr parseAnd();
    NodePtr parseEquality();
    NodePtr parseComparison();
    NodePtr parseAddSub();
    NodePtr parseMulDiv();
    NodePtr parseUnary();
    NodePtr parsePostfix();
    NodePtr parsePrimary();

    bool isTypeName(TokenType t) const;
    std::string typeString(TokenType t) const;
};
