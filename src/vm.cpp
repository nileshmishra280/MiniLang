#include "vm.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <cassert>

VM::VM(bool trace) : trace(trace) {
    stack.resize(STACK_SIZE, 0);
    heap.resize(HEAP_SIZE, 0);
}

void VM::push(int32_t v) {
    if (sp + 1 >= STACK_SIZE)
        throw VMError("Stack overflow", pc);
    stack[++sp] = v;
}

int32_t VM::pop() {
    if (sp < 0)
        throw VMError("Stack underflow", pc);
    return stack[sp--];
}

int32_t VM::top() const {
    if (sp < 0) throw VMError("Stack underflow on top()", pc);
    return stack[sp];
}

int VM::heapAlloc(int size) {
    int base = nextHeap;
    nextHeap += size;
    if (nextHeap >= HEAP_SIZE)
        throw VMError("Heap overflow", pc);
    return base;
}

// ── Disassembler ─────────────────────────────────────────────────────────────
void VM::disassemble(const BytecodeImage& img) {
    std::cout << "=== Bytecode Disassembly ===\n";
    std::cout << "Functions:\n";
    for (auto& f : img.funcs) {
        std::cout << "  " << f.name << "  entry=" << f.entry
                  << "  arity=" << f.arity
                  << "  locals=" << f.locals << "\n";
    }
    std::cout << "\nString pool:\n";
    for (int i = 0; i < (int)img.strings.size(); ++i)
        std::cout << "  [" << i << "] \"" << img.strings[i] << "\"\n";

    std::cout << "\nInstructions:\n";
    for (int i = 0; i < (int)img.code.size(); ++i) {
        const Instr& ins = img.code[i];
        // Mark function entries
        for (auto& f : img.funcs)
            if (f.entry == i) std::cout << "\n<" << f.name << ">:\n";

        std::cout << std::setw(5) << i << "  "
                  << std::left << std::setw(14) << opName(ins.op);
        switch (ins.op) {
            case Op::JMP: case Op::JZ: case Op::JNZ:
                std::cout << " → " << ins.operand; break;
            case Op::CALL:
                std::cout << " @" << ins.operand; break;
            case Op::PUSH_STR:
                if (ins.operand < (int)img.strings.size())
                    std::cout << " [" << ins.operand << "] \"" << img.strings[ins.operand] << "\"";
                break;
            case Op::PRINT_STR:
                if (ins.operand < (int)img.strings.size())
                    std::cout << " \"" << img.strings[ins.operand] << "\"";
                break;
            default:
                // Only print operand if the opcode uses it
                #define X(name, has_op) case Op::name: if (has_op) std::cout << " " << ins.operand; break;
                switch (ins.op) { OPCODE_LIST(X) }
                #undef X
        }
        if (ins.line) std::cout << "   ; line " << ins.line;
        std::cout << "\n";
    }
}

// ── Main execution loop ───────────────────────────────────────────────────────
int VM::run(const BytecodeImage& img) {
    globals.assign(img.globalCount, 0);
    const auto& code = img.code;

    pc = 0;
    sp = -1;
    fp = -1;

    // Find function metadata quickly by entry point
    // Build entry → FuncMeta map
    std::unordered_map<int,const FuncMeta*> funcByEntry;
    for (auto& f : img.funcs) funcByEntry[f.entry] = &f;

    while (true) {
        if (pc < 0 || pc >= (int)code.size())
            throw VMError("PC out of bounds: " + std::to_string(pc), pc);

        const Instr& ins = code[pc];

        if (trace) {
            std::cerr << "[PC=" << std::setw(4) << pc << " SP=" << std::setw(3) << sp
                      << " FP=" << std::setw(3) << fp << "]  "
                      << std::left << std::setw(14) << opName(ins.op);
            switch (ins.op) {
                case Op::PUSH: case Op::LOAD: case Op::STORE:
                case Op::GLOAD: case Op::GSTORE: case Op::JMP:
                case Op::JZ: case Op::JNZ: case Op::CALL:
                    std::cerr << ins.operand;
                    break;
                default: break;
            }
            std::cerr << "\n";
        }

        ++pc;

        switch (ins.op) {

        case Op::PUSH:
            push(ins.operand);
            break;

        case Op::POP:
            pop();
            break;

        case Op::DUP:
            push(top());
            break;

        // ── Locals ────────────────────────────────────────────────────────
        case Op::LOAD: {
            int slot = ins.operand;
            int base = fp;
            // locals start at fp + 3 (return_addr, saved_fp, num_locals)
            push(stack[base + 3 + slot]);
            break;
        }
        case Op::STORE: {
            int slot = ins.operand;
            int base = fp;
            stack[base + 3 + slot] = pop();
            break;
        }

        // ── Globals ───────────────────────────────────────────────────────
        case Op::GLOAD:
            push(globals[ins.operand]);
            break;

        case Op::GSTORE:
            globals[ins.operand] = pop();
            break;

        // ── Arrays ────────────────────────────────────────────────────────
        case Op::ANEW: {
            int base = heapAlloc(ins.operand);
            push(base);  // push heap address
            break;
        }
        case Op::ALOAD: {
            int idx  = pop();
            int base = pop();
            if (idx < 0)
                throw VMError("Negative array index", pc-1);
            push(heap[base + idx]);
            break;
        }
        case Op::ASTORE: {
            int idx  = pop();
            int base = pop();
            int val  = pop();
            if (idx < 0)
                throw VMError("Negative array index", pc-1);
            heap[base + idx] = val;
            break;
        }

        // ── Arithmetic ────────────────────────────────────────────────────
        case Op::ADD: { int32_t b = pop(), a = pop(); push(a + b); break; }
        case Op::SUB: { int32_t b = pop(), a = pop(); push(a - b); break; }
        case Op::MUL: { int32_t b = pop(), a = pop(); push(a * b); break; }
        case Op::DIV: {
            int32_t b = pop(), a = pop();
            if (b == 0) throw VMError("Division by zero", pc-1);
            push(a / b); break;
        }
        case Op::MOD: {
            int32_t b = pop(), a = pop();
            if (b == 0) throw VMError("Modulo by zero", pc-1);
            push(a % b); break;
        }
        case Op::NEG: push(-pop()); break;

        // ── Comparison ────────────────────────────────────────────────────
        case Op::EQ:  { int32_t b = pop(), a = pop(); push(a == b ? 1 : 0); break; }
        case Op::NEQ: { int32_t b = pop(), a = pop(); push(a != b ? 1 : 0); break; }
        case Op::LT:  { int32_t b = pop(), a = pop(); push(a  < b ? 1 : 0); break; }
        case Op::LTE: { int32_t b = pop(), a = pop(); push(a <= b ? 1 : 0); break; }
        case Op::GT:  { int32_t b = pop(), a = pop(); push(a  > b ? 1 : 0); break; }
        case Op::GTE: { int32_t b = pop(), a = pop(); push(a >= b ? 1 : 0); break; }

        // ── Logical ───────────────────────────────────────────────────────
        case Op::NOT: push(pop() == 0 ? 1 : 0); break;
        case Op::AND: { int32_t b = pop(), a = pop(); push((a && b) ? 1 : 0); break; }
        case Op::OR:  { int32_t b = pop(), a = pop(); push((a || b) ? 1 : 0); break; }

        // ── Control flow ──────────────────────────────────────────────────
        case Op::JMP:  pc = ins.operand; break;
        case Op::JZ:   if (pop() == 0) pc = ins.operand; break;
        case Op::JNZ:  if (pop() != 0) pc = ins.operand; break;

        // ── Function call / return ────────────────────────────────────────
        // Call convention:
        //   Before CALL: args are on stack (leftmost arg pushed first)
        //   CALL pushes: [return_pc, saved_fp, num_locals, ...local slots...]
        //   After CALL: fp points to the frame base (where return_pc is)
        case Op::CALL: {
            int target = ins.operand;
            // Find function metadata
            auto it = funcByEntry.find(target);
            if (it == funcByEntry.end())
                throw VMError("No function at address " + std::to_string(target), pc-1);
            const FuncMeta& meta = *it->second;

            // Pop args (they're on top of stack, rightmost first to pop)
            std::vector<int32_t> args(meta.arity);
            for (int i = meta.arity - 1; i >= 0; --i)
                args[i] = pop();

            // Push frame
            int savedFP = fp;
            fp = sp + 1;

            push(pc);         // return address
            push(savedFP);    // saved frame pointer
            push(meta.locals);// local count (debug info)

            // Allocate locals
            for (int i = 0; i < meta.locals; ++i) push(0);

            // Write params into first local slots
            for (int i = 0; i < meta.arity; ++i)
                stack[fp + 3 + i] = args[i];

            pc = target;
            break;
        }

        case Op::RET_VAL: {
            int32_t retVal = pop();
            // Restore stack to before locals
            int locals = stack[fp + 2];
            int retAddr = stack[fp];
            int savedFP = stack[fp + 1];

            sp = fp - 1;
            fp = savedFP;
            pc = retAddr;
            push(retVal);
            break;
        }

        case Op::RET: {
            int retAddr = stack[fp];
            int savedFP = stack[fp + 1];
            sp = fp - 1;
            fp = savedFP;
            pc = retAddr;
            push(0); // void return → push 0
            break;
        }

        // ── I/O ───────────────────────────────────────────────────────────
        case Op::PRINT_INT:
            std::cout << pop() << "\n";
            break;

        case Op::PRINT_STR:
            if (ins.operand >= 0 && ins.operand < (int)img.strings.size())
                std::cout << img.strings[ins.operand] << "\n";
            break;

        case Op::PUSH_STR:
            // Push the string index as an integer (for passing to functions etc.)
            push(ins.operand);
            break;

        // ── Halt ──────────────────────────────────────────────────────────
        case Op::HALT:
            // Return top of stack as exit code (or 0)
            return (sp >= 0) ? pop() : 0;

        default:
            throw VMError("Unknown opcode " + std::to_string((int)ins.op), pc-1);
        }
    }
}
