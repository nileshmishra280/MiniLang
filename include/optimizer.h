#pragma once
#include "bytecode.h"
#include <vector>

// Peephole optimizer: works on the flat instruction stream.
// Passes:
//   1. Constant folding  (PUSH a, PUSH b, ADD  →  PUSH (a+b))
//   2. Dead PUSH+POP elimination
//   3. Jump-to-next elimination (JMP to immediate next instruction)
//   4. Double-negation removal  (NEG, NEG → nothing)
class Optimizer {
public:
    void optimize(BytecodeImage& image);

private:
    bool foldConstants(std::vector<Instr>& code);
    bool eliminatePushPop(std::vector<Instr>& code);
    bool eliminateDeadJumps(std::vector<Instr>& code);
    bool eliminateDoubleNeg(std::vector<Instr>& code);
    void rewriteJumpTargets(std::vector<Instr>& code,
                            const std::vector<int>& remap);
};
