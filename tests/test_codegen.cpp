#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include <iostream>
#include <cassert>

static int passed = 0, failed = 0;
#define CHECK(cond, msg) \
    do { if (!(cond)) { std::cerr << "FAIL: " << msg << "\n"; ++failed; } \
         else { ++passed; } } while(0)

static BytecodeImage compileSource(const std::string& src) {
    Lexer l(src); Parser p(l.tokenize());
    auto prog = p.parse();
    CodeGen cg; return cg.generate(*prog);
}

void test_produces_code() {
    auto img = compileSource("void main() { }");
    CHECK(!img.code.empty(), "generates instructions");
}

void test_has_halt() {
    auto img = compileSource("void main() { }");
    bool found = false;
    for (auto& ins : img.code)
        if (ins.op == Op::HALT) { found = true; break; }
    CHECK(found, "has HALT instruction");
}

void test_main_in_funcs() {
    auto img = compileSource("void main() { }");
    CHECK(!img.funcs.empty(), "has function metadata");
    bool found = false;
    for (auto& f : img.funcs) if (f.name == "main") found = true;
    CHECK(found, "main is in funcs table");
}

void test_global_count() {
    auto img = compileSource("int a; int b; void main() {}");
    CHECK(img.globalCount == 2, "two globals");
}

void test_string_pool() {
    auto img = compileSource(R"(void main() { print("hello"); })");
    CHECK(!img.strings.empty(),      "string pool has entry");
    CHECK(img.strings[0] == "hello", "string value correct");
}

void test_push_instruction() {
    auto img = compileSource("void main() { int x = 42; }");
    bool found = false;
    for (auto& ins : img.code)
        if (ins.op == Op::PUSH && ins.operand == 42) { found = true; break; }
    CHECK(found, "PUSH 42 emitted");
}

void test_function_call_emitted() {
    auto img = compileSource("int add(int a, int b) { return a + b; } void main() { int r = add(1, 2); }");
    bool hasCall = false;
    for (auto& ins : img.code)
        if (ins.op == Op::CALL) { hasCall = true; break; }
    CHECK(hasCall, "CALL instruction emitted");
}

int main() {
    test_produces_code();
    test_has_halt();
    test_main_in_funcs();
    test_global_count();
    test_string_pool();
    test_push_instruction();
    test_function_call_emitted();

    std::cout << "CodeGen tests: " << passed << " passed, " << failed << " failed\n";
    return (failed == 0) ? 0 : 1;
}
