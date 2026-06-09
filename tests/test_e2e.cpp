#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "optimizer.h"
#include "vm.h"

#include <iostream>
#include <sstream>
#include <string>

static int passed = 0, failed = 0;
#define CHECK(cond, msg) \
    do { if (!(cond)) { std::cerr << "FAIL: " << msg << "\n"; ++failed; } \
         else { ++passed; } } while(0)

// Compile source, run it, return exit code
static int runSource(const std::string& src, bool optimize = true) {
    Lexer l(src);
    Parser p(l.tokenize());
    auto prog = p.parse();
    CodeGen cg;
    auto img = cg.generate(*prog);
    if (optimize) { Optimizer opt; opt.optimize(img); }
    VM vm;
    return vm.run(img);
}

void test_return_constant() {
    CHECK(runSource("int main() { return 5; }") == 5, "return constant 5");
}

void test_arithmetic_return() {
    CHECK(runSource("int main() { return 3 + 4; }") == 7, "3+4=7");
    CHECK(runSource("int main() { return 10 - 3; }") == 7, "10-3=7");
    CHECK(runSource("int main() { return 6 * 7; }") == 42, "6*7=42");
    CHECK(runSource("int main() { return 20 / 4; }") == 5, "20/4=5");
    CHECK(runSource("int main() { return 17 % 5; }") == 2, "17%5=2");
}

void test_variables() {
    CHECK(runSource("int main() { int x = 10; int y = 20; return x + y; }") == 30,
          "local variables add to 30");
}

void test_if_true() {
    CHECK(runSource("int main() { if (1) { return 1; } return 0; }") == 1, "if true");
}

void test_if_false() {
    CHECK(runSource("int main() { if (0) { return 1; } return 0; }") == 0, "if false");
}

void test_if_else() {
    CHECK(runSource("int main() { if (0) { return 1; } else { return 2; } }") == 2,
          "if-else takes else branch");
}

void test_while_loop() {
    const char* src = R"(
        int main() {
            int i = 0;
            int sum = 0;
            while (i < 5) {
                sum = sum + i;
                i = i + 1;
            }
            return sum;
        }
    )";
    CHECK(runSource(src) == 10, "while loop sum 0..4 = 10");
}

void test_for_loop() {
    const char* src = R"(
        int main() {
            int sum = 0;
            for (int i = 1; i <= 10; i++) {
                sum += i;
            }
            return sum;
        }
    )";
    CHECK(runSource(src) == 55, "for loop 1..10 = 55");
}

void test_function_call() {
    const char* src = R"(
        int square(int n) { return n * n; }
        int main() { return square(7); }
    )";
    CHECK(runSource(src) == 49, "function call square(7)=49");
}

void test_recursive_function() {
    const char* src = R"(
        int fib(int n) {
            if (n <= 1) { return n; }
            return fib(n - 1) + fib(n - 2);
        }
        int main() { return fib(10); }
    )";
    CHECK(runSource(src) == 55, "recursive fibonacci fib(10)=55");
}

void test_factorial() {
    const char* src = R"(
        int fact(int n) {
            if (n <= 1) { return 1; }
            return n * fact(n - 1);
        }
        int main() { return fact(6); }
    )";
    CHECK(runSource(src) == 720, "factorial fact(6)=720");
}

void test_global_variable() {
    const char* src = R"(
        int counter = 0;
        int increment() { counter = counter + 1; return counter; }
        int main() {
            increment();
            increment();
            return increment();
        }
    )";
    CHECK(runSource(src) == 3, "global variable incremented 3 times");
}

void test_array() {
    const char* src = R"(
        int main() {
            int arr[5];
            arr[0] = 10;
            arr[1] = 20;
            arr[2] = 30;
            return arr[0] + arr[1] + arr[2];
        }
    )";
    CHECK(runSource(src) == 60, "array store and load");
}

void test_comparison_ops() {
    CHECK(runSource("int main() { return 3 == 3; }") == 1, "== true");
    CHECK(runSource("int main() { return 3 == 4; }") == 0, "== false");
    CHECK(runSource("int main() { return 3 != 4; }") == 1, "!= true");
    CHECK(runSource("int main() { return 3 <  4; }") == 1, "<  true");
    CHECK(runSource("int main() { return 4 >  3; }") == 1, ">  true");
    CHECK(runSource("int main() { return 3 <= 3; }") == 1, "<= equal");
    CHECK(runSource("int main() { return 3 >= 4; }") == 0, ">= false");
}

void test_logical_ops() {
    CHECK(runSource("int main() { return 1 && 1; }") == 1, "&& T&&T");
    CHECK(runSource("int main() { return 1 && 0; }") == 0, "&& T&&F");
    CHECK(runSource("int main() { return 0 || 1; }") == 1, "|| F||T");
    CHECK(runSource("int main() { return !0; }")     == 1, "! of 0");
    CHECK(runSource("int main() { return !1; }")     == 0, "! of 1");
}

void test_increment_decrement() {
    CHECK(runSource("int main() { int x = 5; x++; return x; }") == 6, "x++ increments");
    CHECK(runSource("int main() { int x = 5; x--; return x; }") == 4, "x-- decrements");
    CHECK(runSource("int main() { int x = 5; return ++x; }") == 6, "++x returns new val");
}

void test_nested_function_calls() {
    const char* src = R"(
        int add(int a, int b) { return a + b; }
        int mul(int a, int b) { return a * b; }
        int main() { return add(mul(2, 3), mul(4, 5)); }
    )";
    CHECK(runSource(src) == 26, "nested calls: add(6,20)=26");
}

void test_optimizer_constant_fold() {
    // With optimizer, PUSH 3 PUSH 4 ADD should become PUSH 7
    int result1 = runSource("int main() { return 3 + 4; }", true);
    int result2 = runSource("int main() { return 3 + 4; }", false);
    CHECK(result1 == 7 && result2 == 7, "optimizer and non-optimizer give same result");
}

void test_multiple_returns() {
    const char* src = R"(
        int abs_val(int x) {
            if (x < 0) { return -x; }
            return x;
        }
        int main() { return abs_val(-7) + abs_val(3); }
    )";
    CHECK(runSource(src) == 10, "multiple return paths: abs(-7)+abs(3)=10");
}

int main() {
    test_return_constant();
    test_arithmetic_return();
    test_variables();
    test_if_true();
    test_if_false();
    test_if_else();
    test_while_loop();
    test_for_loop();
    test_function_call();
    test_recursive_function();
    test_factorial();
    test_global_variable();
    test_array();
    test_comparison_ops();
    test_logical_ops();
    test_increment_decrement();
    test_nested_function_calls();
    test_optimizer_constant_fold();
    test_multiple_returns();

    std::cout << "E2E tests: " << passed << " passed, " << failed << " failed\n";
    return (failed == 0) ? 0 : 1;
}
