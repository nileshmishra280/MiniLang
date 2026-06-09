#include "parser.h"
#include <sstream>

Parser::Parser(std::vector<Token> toks) : tokens(std::move(toks)) {}

// ── Helpers ───────────────────────────────────────────────────────────────────
Token& Parser::peek(int offset) {
    size_t idx = pos + offset;
    if (idx >= tokens.size()) return tokens.back(); // EOF_TOK
    return tokens[idx];
}

Token Parser::consume() {
    Token t = tokens[pos];
    if (pos + 1 < tokens.size()) ++pos;
    return t;
}

Token Parser::expect(TokenType t, const std::string& msg) {
    if (!check(t)) {
        std::string m = msg.empty()
            ? ("Expected " + tokenTypeName(t) + " but got '" + peek().value + "'")
            : msg;
        throw ParseError(m, peek().line, peek().col);
    }
    return consume();
}

bool Parser::check(TokenType t) const {
    return tokens[pos].type == t;
}

bool Parser::match(TokenType t) {
    if (!check(t)) return false;
    consume();
    return true;
}

bool Parser::isTypeName(TokenType t) const {
    return t == TokenType::KW_INT || t == TokenType::KW_CHAR || t == TokenType::KW_VOID;
}

std::string Parser::typeString(TokenType t) const {
    switch (t) {
        case TokenType::KW_INT:  return "int";
        case TokenType::KW_CHAR: return "char";
        case TokenType::KW_VOID: return "void";
        default: return "unknown";
    }
}

// ── Top-level ─────────────────────────────────────────────────────────────────
std::unique_ptr<Program> Parser::parse() {
    auto prog = std::make_unique<Program>();

    while (!check(TokenType::EOF_TOK)) {
        // Lookahead: type identifier ( → function; type identifier ; → global var
        if (isTypeName(peek().type) && peek(1).type == TokenType::IDENTIFIER) {
            // is it a function (has paren after name) or a var?
            if (peek(2).type == TokenType::LPAREN) {
                prog->funcs.push_back(parseFuncDecl());
            } else {
                prog->globals.push_back(parseGlobalVar());
            }
        } else {
            throw ParseError("Expected type at top level, got '" + peek().value + "'",
                             peek().line, peek().col);
        }
    }
    return prog;
}

std::unique_ptr<VarDecl> Parser::parseGlobalVar() {
    auto decl = std::make_unique<VarDecl>();
    decl->line = peek().line;
    decl->isGlobal = true;
    decl->type = typeString(consume().type);
    decl->name = expect(TokenType::IDENTIFIER).value;

    if (match(TokenType::LBRACKET)) {
        decl->arraySize = std::stoi(expect(TokenType::INT_LITERAL).value);
        expect(TokenType::RBRACKET);
    }
    if (match(TokenType::ASSIGN)) {
        decl->init = parseExpr();
    }
    expect(TokenType::SEMICOLON);
    return decl;
}

std::unique_ptr<FuncDecl> Parser::parseFuncDecl() {
    auto decl = std::make_unique<FuncDecl>();
    decl->line    = peek().line;
    decl->retType = typeString(consume().type);
    decl->name    = expect(TokenType::IDENTIFIER).value;

    expect(TokenType::LPAREN);
    if (!check(TokenType::RPAREN)) {
        do {
            std::string ptype = typeString(peek().type);
            if (!isTypeName(consume().type))
                throw ParseError("Expected type in parameter list", peek().line, peek().col);
            std::string pname = expect(TokenType::IDENTIFIER).value;
            decl->params.push_back({ptype, pname});
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::RPAREN);
    decl->body = parseBlock();
    return decl;
}

// ── Statements ────────────────────────────────────────────────────────────────
std::unique_ptr<Block> Parser::parseBlock() {
    expect(TokenType::LBRACE);
    auto block = std::make_unique<Block>();
    block->line = peek().line;
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK)) {
        block->stmts.push_back(parseStatement());
    }
    expect(TokenType::RBRACE);
    return block;
}

NodePtr Parser::parseStatement() {
    TokenType t = peek().type;

    if (t == TokenType::LBRACE)     return parseBlock();
    if (t == TokenType::KW_IF)      return parseIf();
    if (t == TokenType::KW_WHILE)   return parseWhile();
    if (t == TokenType::KW_FOR)     return parseFor();
    if (t == TokenType::KW_RETURN)  return parseReturn();
    if (t == TokenType::KW_PRINT)   return parsePrint();
    if (isTypeName(t))              return parseLocalVar();

    // Expression statement
    auto es = std::make_unique<ExprStmt>();
    es->line = peek().line;
    es->expr = parseExpr();
    expect(TokenType::SEMICOLON);
    return es;
}

std::unique_ptr<VarDecl> Parser::parseLocalVar() {
    auto decl = std::make_unique<VarDecl>();
    decl->line = peek().line;
    decl->isGlobal = false;
    decl->type = typeString(consume().type);
    decl->name = expect(TokenType::IDENTIFIER).value;

    if (match(TokenType::LBRACKET)) {
        decl->arraySize = std::stoi(expect(TokenType::INT_LITERAL).value);
        expect(TokenType::RBRACKET);
    }
    if (match(TokenType::ASSIGN)) {
        decl->init = parseExpr();
    }
    expect(TokenType::SEMICOLON);
    return decl;
}

std::unique_ptr<IfStmt> Parser::parseIf() {
    auto node = std::make_unique<IfStmt>();
    node->line = peek().line;
    expect(TokenType::KW_IF);
    expect(TokenType::LPAREN);
    node->cond = parseExpr();
    expect(TokenType::RPAREN);
    node->thenBranch = parseBlock();
    if (match(TokenType::KW_ELSE)) {
        node->elseBranch = parseBlock();
    }
    return node;
}

std::unique_ptr<WhileStmt> Parser::parseWhile() {
    auto node = std::make_unique<WhileStmt>();
    node->line = peek().line;
    expect(TokenType::KW_WHILE);
    expect(TokenType::LPAREN);
    node->cond = parseExpr();
    expect(TokenType::RPAREN);
    node->body = parseBlock();
    return node;
}

std::unique_ptr<ForStmt> Parser::parseFor() {
    auto node = std::make_unique<ForStmt>();
    node->line = peek().line;
    expect(TokenType::KW_FOR);
    expect(TokenType::LPAREN);

    // init
    if (!check(TokenType::SEMICOLON)) {
        if (isTypeName(peek().type)) {
            node->init = parseLocalVar();
        } else {
            auto es = std::make_unique<ExprStmt>();
            es->expr = parseExpr();
            expect(TokenType::SEMICOLON);
            node->init = std::move(es);
        }
    } else { consume(); }

    // cond
    if (!check(TokenType::SEMICOLON)) node->cond = parseExpr();
    expect(TokenType::SEMICOLON);

    // post
    if (!check(TokenType::RPAREN)) node->post = parseExpr();
    expect(TokenType::RPAREN);

    node->body = parseBlock();
    return node;
}

std::unique_ptr<ReturnStmt> Parser::parseReturn() {
    auto node = std::make_unique<ReturnStmt>();
    node->line = peek().line;
    expect(TokenType::KW_RETURN);
    if (!check(TokenType::SEMICOLON)) node->expr = parseExpr();
    expect(TokenType::SEMICOLON);
    return node;
}

std::unique_ptr<PrintStmt> Parser::parsePrint() {
    auto node = std::make_unique<PrintStmt>();
    node->line = peek().line;
    expect(TokenType::KW_PRINT);
    expect(TokenType::LPAREN);
    node->expr = parseExpr();
    expect(TokenType::RPAREN);
    expect(TokenType::SEMICOLON);
    return node;
}

// ── Expressions (Pratt / recursive-descent) ──────────────────────────────────
NodePtr Parser::parseExpr()       { return parseAssign(); }

NodePtr Parser::parseAssign() {
    NodePtr lhs = parseOr();

    TokenType t = peek().type;
    if (t == TokenType::ASSIGN ||
        t == TokenType::PLUS_ASSIGN ||
        t == TokenType::MINUS_ASSIGN) {
        std::string op = consume().value;
        NodePtr rhs = parseAssign();
        auto node = std::make_unique<AssignExpr>();
        node->op     = op;
        node->target = std::move(lhs);
        node->value  = std::move(rhs);
        return node;
    }
    return lhs;
}

NodePtr Parser::parseOr() {
    NodePtr lhs = parseAnd();
    while (check(TokenType::OR)) {
        std::string op = consume().value;
        auto node  = std::make_unique<BinaryExpr>();
        node->op   = op;
        node->lhs  = std::move(lhs);
        node->rhs  = parseAnd();
        lhs = std::move(node);
    }
    return lhs;
}

NodePtr Parser::parseAnd() {
    NodePtr lhs = parseEquality();
    while (check(TokenType::AND)) {
        std::string op = consume().value;
        auto node = std::make_unique<BinaryExpr>();
        node->op  = op;
        node->lhs = std::move(lhs);
        node->rhs = parseEquality();
        lhs = std::move(node);
    }
    return lhs;
}

NodePtr Parser::parseEquality() {
    NodePtr lhs = parseComparison();
    while (check(TokenType::EQ) || check(TokenType::NEQ)) {
        std::string op = consume().value;
        auto node = std::make_unique<BinaryExpr>();
        node->op  = op;
        node->lhs = std::move(lhs);
        node->rhs = parseComparison();
        lhs = std::move(node);
    }
    return lhs;
}

NodePtr Parser::parseComparison() {
    NodePtr lhs = parseAddSub();
    while (check(TokenType::LT) || check(TokenType::LTE) ||
           check(TokenType::GT) || check(TokenType::GTE)) {
        std::string op = consume().value;
        auto node = std::make_unique<BinaryExpr>();
        node->op  = op;
        node->lhs = std::move(lhs);
        node->rhs = parseAddSub();
        lhs = std::move(node);
    }
    return lhs;
}

NodePtr Parser::parseAddSub() {
    NodePtr lhs = parseMulDiv();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = consume().value;
        auto node = std::make_unique<BinaryExpr>();
        node->op  = op;
        node->lhs = std::move(lhs);
        node->rhs = parseMulDiv();
        lhs = std::move(node);
    }
    return lhs;
}

NodePtr Parser::parseMulDiv() {
    NodePtr lhs = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
        std::string op = consume().value;
        auto node = std::make_unique<BinaryExpr>();
        node->op  = op;
        node->lhs = std::move(lhs);
        node->rhs = parseUnary();
        lhs = std::move(node);
    }
    return lhs;
}

NodePtr Parser::parseUnary() {
    if (check(TokenType::MINUS) || check(TokenType::NOT)) {
        std::string op = consume().value;
        auto node = std::make_unique<UnaryExpr>();
        node->op      = op;
        node->prefix  = true;
        node->operand = parseUnary();
        return node;
    }
    if (check(TokenType::INCREMENT) || check(TokenType::DECREMENT)) {
        std::string op = consume().value;
        auto node = std::make_unique<UnaryExpr>();
        node->op      = op;
        node->prefix  = true;
        node->operand = parsePostfix();
        return node;
    }
    return parsePostfix();
}

NodePtr Parser::parsePostfix() {
    NodePtr base = parsePrimary();
    if (check(TokenType::INCREMENT) || check(TokenType::DECREMENT)) {
        std::string op = consume().value;
        auto node = std::make_unique<UnaryExpr>();
        node->op      = op;
        node->prefix  = false;
        node->operand = std::move(base);
        return node;
    }
    return base;
}

NodePtr Parser::parsePrimary() {
    TokenType t = peek().type;
    int ln = peek().line;

    if (t == TokenType::INT_LITERAL) {
        auto node  = std::make_unique<IntLiteral>();
        node->line  = ln;
        node->value = std::stoi(consume().value);
        return node;
    }
    if (t == TokenType::STRING_LITERAL) {
        auto node  = std::make_unique<StringLiteral>();
        node->line  = ln;
        node->value = consume().value;
        return node;
    }
    if (t == TokenType::IDENTIFIER) {
        std::string name = consume().value;

        // Function call
        if (check(TokenType::LPAREN)) {
            consume();
            auto node = std::make_unique<CallExpr>();
            node->line   = ln;
            node->callee = name;
            if (!check(TokenType::RPAREN)) {
                do {
                    node->args.push_back(parseExpr());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::RPAREN);
            return node;
        }

        // Array subscript
        if (check(TokenType::LBRACKET)) {
            consume();
            auto node = std::make_unique<ArraySubscript>();
            node->line  = ln;
            node->name  = name;
            node->index = parseExpr();
            expect(TokenType::RBRACKET);
            return node;
        }

        // Plain variable
        auto node  = std::make_unique<VarExpr>();
        node->line  = ln;
        node->name  = name;
        return node;
    }
    if (match(TokenType::LPAREN)) {
        NodePtr e = parseExpr();
        expect(TokenType::RPAREN);
        return e;
    }

    throw ParseError("Unexpected token '" + peek().value + "' in expression",
                     peek().line, peek().col);
}
