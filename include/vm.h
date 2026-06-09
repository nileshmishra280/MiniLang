#pragma once
#include "bytecode.h"
#include <vector>
#include <string>
#include <stdexcept>
#include <unordered_map>

struct VMError : std::runtime_error {
    int pc;
    VMError(const std::string& msg, int pc_)
        : std::runtime_error(msg), pc(pc_) {}
};

class VM {
public:
    static constexpr int STACK_SIZE  = 65536;
    static constexpr int HEAP_SIZE   = 65536;  // cells for arrays

    explicit VM(bool trace = false);

    // Load a compiled image and run it, returning the exit code
    int run(const BytecodeImage& image);

    // Disassemble to stdout (for --dump flag)
    static void disassemble(const BytecodeImage& image);

private:
    bool trace;

    // Runtime state
    std::vector<int32_t> stack;
    std::vector<int32_t> heap;      // array storage
    int32_t sp  = -1;               // stack pointer
    int32_t fp  = -1;               // frame pointer
    int32_t pc  =  0;               // program counter
    std::vector<int32_t> globals;

    // Helpers
    void   push(int32_t v);
    int32_t pop();
    int32_t top() const;

    int    heapAlloc(int size);     // returns base address on heap
    int    nextHeap = 0;
};
