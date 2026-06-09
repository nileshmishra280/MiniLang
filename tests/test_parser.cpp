#include "lexer.h"
#include "parser.h"
#include <cassert>
#include <iostream>

static int passed = 0, failed = 0;
#define CHECK(cond, msg) \
    do { if (!(cond)) { std::cerr << "FAIL: " << msg << "\n"; ++failed; } \
         else { ++passed; } } while(0)

static std::unique_ptr<Program> parseSource(const std::string& src) {
    Lexer   l(src);
    Parser  p(l.tokenize());
    return  p.parse();
}

void test_empty_main() {
    auto prog = parseSource("void main() {}");
    CHECK(prog->funcs.size() == 1,         "one function");
    CHECK(prog->funcs[0]->name == "main",  "named main");
    CHECK(prog->funcs[0]->retType=="void", "return type void");
    CHECK(prog->funcs[0]->body->stmts.empty(), "empty body");
}

void test_global_var() {
    auto prog = parseSource("int g = 5; void main() {}");
    CHECK(prog->globals.size() == 1,       "one global");
    CHECK(prog->globals[0]->name == "g",   "global name");
    CHECK(prog->globals[0]->init != nullptr, "global has init");
}

void test_function_with_params() {
    auto prog = parseSource("int add(int a, int b) { return a + b; } void main() {}");
    CHECK(prog->funcs[0]->params.size() == 2, "two params");
    CHECK(prog->funcs[0]->params[0].second == "a", "param a");
    CHECK(prog->funcs[0]->params[1].second == "b", "param b");
    CHECK(prog->funcs[0]->body->stmts.size() == 1, "one statement");
}

void test_if_else() {
    auto prog = parseSource("void main() { if (1) { } else { } }");
    CHECK(!prog->funcs[0]->body->stmts.empty(), "has statement");
    auto* ifStmt = dynamic_cast<IfStmt*>(prog->funcs[0]->body->stmts[0].get());
    CHECK(ifStmt != nullptr,            "is IfStmt");
    CHECK(ifStmt->elseBranch != nullptr,"has else branch");
}

void test_while() {
    auto prog = parseSource("void main() { while (1) { } }");
    auto* ws = dynamic_cast<WhileStmt*>(prog->funcs[0]->body->stmts[0].get());
    CHECK(ws != nullptr, "is WhileStmt");
}

void test_for() {
    auto prog = parseSource("void main() { for (int i = 0; i < 10; i++) { } }");
    auto* fs = dynamic_cast<ForStmt*>(prog->funcs[0]->body->stmts[0].get());
    CHECK(fs != nullptr, "is ForStmt");
    CHECK(fs->init != nullptr, "for has init");
    CHECK(fs->cond != nullptr, "for has cond");
    CHECK(fs->post != nullptr, "for has post");
}

void test_array_decl() {
    auto prog = parseSource("void main() { int arr[5]; }");
    auto* vd = dynamic_cast<VarDecl*>(prog->funcs[0]->body->stmts[0].get());
    CHECK(vd != nullptr,        "is VarDecl");
    CHECK(vd->arraySize == 5,   "array size 5");
}

void test_parse_error() {
    bool threw = false;
    try { parseSource("void main( { }"); }
    catch (ParseError&) { threw = true; }
    CHECK(threw, "ParseError on syntax error");
}

void test_binary_precedence() {
    // 2 + 3 * 4 should parse as 2 + (3*4)
    auto prog = parseSource("void main() { int x = 2 + 3 * 4; }");
    auto* vd = dynamic_cast<VarDecl*>(prog->funcs[0]->body->stmts[0].get());
    CHECK(vd != nullptr, "VarDecl");
    auto* add = dynamic_cast<BinaryExpr*>(vd->init.get());
    CHECK(add != nullptr && add->op == "+", "top is add");
    auto* mul = dynamic_cast<BinaryExpr*>(add->rhs.get());
    CHECK(mul != nullptr && mul->op == "*", "rhs of add is mul (correct precedence)");
}

int main() {
    test_empty_main();
    test_global_var();
    test_function_with_params();
    test_if_else();
    test_while();
    test_for();
    test_array_decl();
    test_parse_error();
    test_binary_precedence();

    std::cout << "Parser tests: " << passed << " passed, " << failed << " failed\n";
    return (failed == 0) ? 0 : 1;
}
