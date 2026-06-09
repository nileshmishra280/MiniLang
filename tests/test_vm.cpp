#include "vm.h"
#include "bytecode.h"
#include <iostream>
#include <cassert>

static int passed = 0, failed = 0;
#define CHECK(cond, msg) \
    do { if (!(cond)) { std::cerr << "FAIL: " << msg << "\n"; ++failed; } \
         else { ++passed; } } while(0)

// Helper: build a minimal image that pushes N and halts
static BytecodeImage makeHaltImage(int32_t val) {
    BytecodeImage img;
    img.globalCount = 0;
    img.code.emplace_back(Op::PUSH, val);
    img.code.emplace_back(Op::HALT);
    img.entryPoint = 0;
    return img;
}

void test_halt_returns_value() {
    VM vm;
    auto img = makeHaltImage(42);
    int ret = vm.run(img);
    CHECK(ret == 42, "HALT returns top-of-stack value");
}

void test_arithmetic() {
    // PUSH 10, PUSH 3, ADD → 13
    BytecodeImage img;
    img.code.emplace_back(Op::PUSH, 10);
    img.code.emplace_back(Op::PUSH, 3);
    img.code.emplace_back(Op::ADD);
    img.code.emplace_back(Op::HALT);
    VM vm; CHECK(vm.run(img) == 13, "ADD: 10+3=13");
}

void test_sub() {
    BytecodeImage img;
    img.code.emplace_back(Op::PUSH, 10);
    img.code.emplace_back(Op::PUSH, 3);
    img.code.emplace_back(Op::SUB);
    img.code.emplace_back(Op::HALT);
    VM vm; CHECK(vm.run(img) == 7, "SUB: 10-3=7");
}

void test_mul() {
    BytecodeImage img;
    img.code.emplace_back(Op::PUSH, 6);
    img.code.emplace_back(Op::PUSH, 7);
    img.code.emplace_back(Op::MUL);
    img.code.emplace_back(Op::HALT);
    VM vm; CHECK(vm.run(img) == 42, "MUL: 6*7=42");
}

void test_div() {
    BytecodeImage img;
    img.code.emplace_back(Op::PUSH, 20);
    img.code.emplace_back(Op::PUSH, 4);
    img.code.emplace_back(Op::DIV);
    img.code.emplace_back(Op::HALT);
    VM vm; CHECK(vm.run(img) == 5, "DIV: 20/4=5");
}

void test_comparison() {
    BytecodeImage img;
    img.code.emplace_back(Op::PUSH, 3);
    img.code.emplace_back(Op::PUSH, 5);
    img.code.emplace_back(Op::LT);
    img.code.emplace_back(Op::HALT);
    VM vm; CHECK(vm.run(img) == 1, "LT: 3<5 = 1");
}

void test_jz() {
    // PUSH 0, JZ 4, PUSH 99, HALT, PUSH 1, HALT
    BytecodeImage img;
    img.code.emplace_back(Op::PUSH, 0);
    img.code.emplace_back(Op::JZ, 4);   // jump to index 4
    img.code.emplace_back(Op::PUSH, 99);
    img.code.emplace_back(Op::HALT);
    img.code.emplace_back(Op::PUSH, 1); // index 4
    img.code.emplace_back(Op::HALT);
    VM vm; CHECK(vm.run(img) == 1, "JZ jumps when zero");
}

void test_globals() {
    BytecodeImage img;
    img.globalCount = 1;
    img.code.emplace_back(Op::PUSH, 77);
    img.code.emplace_back(Op::GSTORE, 0);
    img.code.emplace_back(Op::GLOAD, 0);
    img.code.emplace_back(Op::HALT);
    VM vm; CHECK(vm.run(img) == 77, "global store/load");
}

void test_div_by_zero_throws() {
    BytecodeImage img;
    img.code.emplace_back(Op::PUSH, 5);
    img.code.emplace_back(Op::PUSH, 0);
    img.code.emplace_back(Op::DIV);
    img.code.emplace_back(Op::HALT);
    VM vm;
    bool threw = false;
    try { vm.run(img); } catch (VMError&) { threw = true; }
    CHECK(threw, "division by zero throws VMError");
}

int main() {
    test_halt_returns_value();
    test_arithmetic();
    test_sub();
    test_mul();
    test_div();
    test_comparison();
    test_jz();
    test_globals();
    test_div_by_zero_throws();

    std::cout << "VM tests: " << passed << " passed, " << failed << " failed\n";
    return (failed == 0) ? 0 : 1;
}
