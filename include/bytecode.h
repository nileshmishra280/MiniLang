#pragma once
#include <cstdint>
#include <string>
#include <vector>

// ── Opcode table ──────────────────────────────────────────────────────────────
// Each entry: (name, has_operand)
#define OPCODE_LIST(X)          \
    /* Stack ops */             \
    X(PUSH,       true)         \
    X(POP,        false)        \
    X(DUP,        false)        \
    /* Locals */                \
    X(LOAD,       true)         \
    X(STORE,      true)         \
    /* Globals */               \
    X(GLOAD,      true)         \
    X(GSTORE,     true)         \
    /* Arrays */                \
    X(ALOAD,      true)         \
    X(ASTORE,     true)         \
    X(ANEW,       true)         \
    /* Arithmetic */            \
    X(ADD,        false)        \
    X(SUB,        false)        \
    X(MUL,        false)        \
    X(DIV,        false)        \
    X(MOD,        false)        \
    X(NEG,        false)        \
    /* Comparison → push 0/1 */ \
    X(EQ,         false)        \
    X(NEQ,        false)        \
    X(LT,         false)        \
    X(LTE,        false)        \
    X(GT,         false)        \
    X(GTE,        false)        \
    /* Logical */               \
    X(NOT,        false)        \
    X(AND,        false)        \
    X(OR,         false)        \
    /* Control flow */          \
    X(JMP,        true)         \
    X(JZ,         true)         \
    X(JNZ,        true)         \
    /* Functions */             \
    X(CALL,       true)         \
    X(RET,        false)        \
    X(RET_VAL,    false)        \
    /* I/O */                   \
    X(PRINT_INT,  false)        \
    X(PRINT_STR,  true)         \
    /* String pool */           \
    X(PUSH_STR,   true)         \
    /* Halt */                  \
    X(HALT,       false)

enum class Op : uint8_t {
#define X(name, _) name,
    OPCODE_LIST(X)
#undef X
};

inline std::string opName(Op op) {
    switch (op) {
#define X(name, _) case Op::name: return #name;
        OPCODE_LIST(X)
#undef X
        default: return "??";
    }
}

// ── Instruction ───────────────────────────────────────────────────────────────
struct Instr {
    Op      op;
    int32_t operand = 0;  // used when has_operand is true
    int     line    = 0;  // source line (for error messages)

    Instr(Op o, int32_t arg = 0, int ln = 0)
        : op(o), operand(arg), line(ln) {}
};

// ── Function descriptor (stored in bytecode image) ───────────────────────────
struct FuncMeta {
    std::string name;
    int         entry;      // index into instruction array
    int         arity;      // number of parameters
    int         locals;     // number of local slots
};

// ── Compiled image — everything the VM needs ─────────────────────────────────
struct BytecodeImage {
    std::vector<Instr>       code;
    std::vector<FuncMeta>    funcs;
    std::vector<std::string> strings;
    int                      globalCount = 0;
    int                      entryPoint  = -1;
};
