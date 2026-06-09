#include "optimizer.h"
#include <algorithm>

void Optimizer::optimize(BytecodeImage& image) {
    bool changed = true;
    // Run passes until nothing changes (fixpoint)
    while (changed) {
        changed  = false;
        changed |= foldConstants(image.code);
        changed |= eliminatePushPop(image.code);
        changed |= eliminateDoubleNeg(image.code);
        changed |= eliminateDeadJumps(image.code);
    }
}

// ── Pass 1: Constant folding ──────────────────────────────────────────────────
// PUSH a, PUSH b, <op>  →  PUSH result
bool Optimizer::foldConstants(std::vector<Instr>& code) {
    bool changed = false;
    for (size_t i = 2; i < code.size(); ++i) {
        if (code[i-2].op != Op::PUSH) continue;
        if (code[i-1].op != Op::PUSH) continue;

        int32_t a = code[i-2].operand;
        int32_t b = code[i-1].operand;
        int32_t result = 0;
        bool doFold = true;

        switch (code[i].op) {
            case Op::ADD: result = a + b; break;
            case Op::SUB: result = a - b; break;
            case Op::MUL: result = a * b; break;
            case Op::DIV:
                if (b == 0) { doFold = false; break; }
                result = a / b; break;
            case Op::MOD:
                if (b == 0) { doFold = false; break; }
                result = a % b; break;
            case Op::EQ:  result = (a == b) ? 1 : 0; break;
            case Op::NEQ: result = (a != b) ? 1 : 0; break;
            case Op::LT:  result = (a  < b) ? 1 : 0; break;
            case Op::LTE: result = (a <= b) ? 1 : 0; break;
            case Op::GT:  result = (a  > b) ? 1 : 0; break;
            case Op::GTE: result = (a >= b) ? 1 : 0; break;
            default: doFold = false;
        }

        if (doFold) {
            // Replace 3 instructions with PUSH result + two NOPs
            // We'll mark removed instructions as NOP (use HALT with a special operand)
            // Simpler: rebuild without them
            code[i-2] = Instr(Op::PUSH, result, code[i-2].line);
            code[i-1] = Instr(Op::HALT, -1, 0);  // sentinel for removal
            code[i]   = Instr(Op::HALT, -1, 0);
            changed = true;
            i += 2; // skip ahead
        }
    }
    // Remove sentinel HALTs (HALT with operand -1)
    auto it = std::remove_if(code.begin(), code.end(),
        [](const Instr& ins){ return ins.op == Op::HALT && ins.operand == -1; });
    if (it != code.end()) {
        // Need to rewrite jump targets after removal
        std::vector<int> remap(code.size() + (code.end() - it));
        int newIdx = 0;
        for (size_t i = 0; i < code.size(); ++i)
            remap[i] = (code[i].op == Op::HALT && code[i].operand == -1) ? -1 : newIdx++;
        code.erase(it, code.end());
        rewriteJumpTargets(code, remap);
    }
    return changed;
}

// ── Pass 2: Dead PUSH+POP elimination ────────────────────────────────────────
bool Optimizer::eliminatePushPop(std::vector<Instr>& code) {
    bool changed = false;
    for (size_t i = 1; i < code.size(); ++i) {
        if ((code[i-1].op == Op::PUSH || code[i-1].op == Op::LOAD || code[i-1].op == Op::GLOAD) &&
             code[i].op == Op::POP) {
            code[i-1] = Instr(Op::HALT, -1, 0);
            code[i]   = Instr(Op::HALT, -1, 0);
            changed = true;
        }
    }
    auto it = std::remove_if(code.begin(), code.end(),
        [](const Instr& ins){ return ins.op == Op::HALT && ins.operand == -1; });
    if (it != code.end()) {
        std::vector<int> remap(code.size() + (code.end() - it));
        int newIdx = 0;
        for (size_t i = 0; i < code.size(); ++i)
            remap[i] = (code[i].op == Op::HALT && code[i].operand == -1) ? -1 : newIdx++;
        code.erase(it, code.end());
        rewriteJumpTargets(code, remap);
    }
    return changed;
}

// ── Pass 3: Jump-to-next elimination ─────────────────────────────────────────
bool Optimizer::eliminateDeadJumps(std::vector<Instr>& code) {
    bool changed = false;
    for (size_t i = 0; i < code.size(); ++i) {
        if (code[i].op == Op::JMP && code[i].operand == (int)i + 1) {
            code[i] = Instr(Op::HALT, -1, 0);
            changed = true;
        }
    }
    auto it = std::remove_if(code.begin(), code.end(),
        [](const Instr& ins){ return ins.op == Op::HALT && ins.operand == -1; });
    if (it != code.end()) {
        std::vector<int> remap(code.size() + (code.end() - it));
        int newIdx = 0;
        for (size_t i = 0; i < code.size(); ++i)
            remap[i] = (code[i].op == Op::HALT && code[i].operand == -1) ? -1 : newIdx++;
        code.erase(it, code.end());
        rewriteJumpTargets(code, remap);
    }
    return changed;
}

// ── Pass 4: Double-negation removal ──────────────────────────────────────────
bool Optimizer::eliminateDoubleNeg(std::vector<Instr>& code) {
    bool changed = false;
    for (size_t i = 1; i < code.size(); ++i) {
        if (code[i-1].op == Op::NEG && code[i].op == Op::NEG) {
            code[i-1] = Instr(Op::HALT, -1, 0);
            code[i]   = Instr(Op::HALT, -1, 0);
            changed = true;
        }
    }
    auto it = std::remove_if(code.begin(), code.end(),
        [](const Instr& ins){ return ins.op == Op::HALT && ins.operand == -1; });
    if (it != code.end()) {
        std::vector<int> remap(code.size() + (code.end() - it));
        int newIdx = 0;
        for (size_t i = 0; i < code.size(); ++i)
            remap[i] = (code[i].op == Op::HALT && code[i].operand == -1) ? -1 : newIdx++;
        code.erase(it, code.end());
        rewriteJumpTargets(code, remap);
    }
    return changed;
}

// ── Rewrite all jump targets after instruction removal ────────────────────────
void Optimizer::rewriteJumpTargets(std::vector<Instr>& code,
                                    const std::vector<int>& remap) {
    for (auto& ins : code) {
        switch (ins.op) {
            case Op::JMP: case Op::JZ: case Op::JNZ: case Op::CALL:
                if (ins.operand >= 0 && ins.operand < (int)remap.size()) {
                    int newTarget = remap[ins.operand];
                    if (newTarget >= 0) ins.operand = newTarget;
                }
                break;
            default: break;
        }
    }
}
