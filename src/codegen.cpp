#include "codegen.h"
#include <stdexcept>
#include <algorithm>

// ── Helpers ───────────────────────────────────────────────────────────────────
int CodeGen::emit(Op op, int32_t arg, int line) {
    int idx = (int)image.code.size();
    image.code.emplace_back(op, arg, line);
    return idx;
}

void CodeGen::patch(int idx, int32_t newArg) {
    image.code[idx].operand = newArg;
}

int CodeGen::here() const { return (int)image.code.size(); }

int CodeGen::internString(const std::string& s) {
    for (int i = 0; i < (int)image.strings.size(); ++i)
        if (image.strings[i] == s) return i;
    image.strings.push_back(s);
    return (int)image.strings.size() - 1;
}

// ── Entry point ───────────────────────────────────────────────────────────────
BytecodeImage CodeGen::generate(Program& prog) {
    prog.accept(*this);
    return std::move(image);
}

// ── Program ───────────────────────────────────────────────────────────────────
void CodeGen::visit(Program& prog) {
    // Pre-size funcs table
    image.funcs.resize(prog.funcs.size());
    for (int i = 0; i < (int)prog.funcs.size(); ++i)
        funcIndex[prog.funcs[i]->name] = i;

    // Register globals in symbol table
    sym.pushScope();
    image.globalCount = (int)prog.globals.size();
    int gslot = 0;
    for (auto& g : prog.globals) {
        Symbol s;
        s.type = g->type; s.slot = gslot++; s.arraySize = g->arraySize;
        s.isGlobal = true; s.isParam = false;
        sym.define(g->name, s);
    }

    // Bootstrap: CALL main, HALT
    int callIdx = emit(Op::CALL, 0);
    emit(Op::HALT);

    // Global initializers
    for (auto& g : prog.globals) {
        if (g->arraySize > 0) {
            emit(Op::ANEW, g->arraySize, g->line);
            emit(Op::GSTORE, sym.lookup(g->name).slot, g->line);
        } else if (g->init) {
            g->init->accept(*this);
            emit(Op::GSTORE, sym.lookup(g->name).slot, g->line);
        }
    }

    // Compile all functions — CALL patches happen inside visit(CallExpr)
    // using a two-pass: first pass records entry points, second patches
    // We collect call-patch sites per callee name
    for (auto& f : prog.funcs)
        f->accept(*this);

    // Patch all forward-call sites
    for (auto& [site, callee] : callPatches) {
        auto it = funcIndex.find(callee);
        if (it == funcIndex.end())
            throw std::runtime_error("Undefined function '" + callee + "'");
        patch(site, image.funcs[it->second].entry);
    }

    // Patch bootstrap CALL to main
    auto it = funcIndex.find("main");
    if (it == funcIndex.end())
        throw std::runtime_error("No 'main' function defined");
    patch(callIdx, image.funcs[it->second].entry);
    image.entryPoint = image.funcs[it->second].entry;
}

// ── Function ──────────────────────────────────────────────────────────────────
void CodeGen::visit(FuncDecl& f) {
    inFunc = true;
    sym.pushScope();
    sym.resetLocals();

    int entry = here();
    int fi = funcIndex[f.name];
    image.funcs[fi].name  = f.name;
    image.funcs[fi].entry = entry;
    image.funcs[fi].arity = (int)f.params.size();

    for (auto& [ptype, pname] : f.params) {
        int slot = sym.allocLocal();
        Symbol s;
        s.type = ptype; s.slot = slot; s.arraySize = 0;
        s.isGlobal = false; s.isParam = true;
        sym.define(pname, s);
    }

    f.body->accept(*this);

    // Implicit return 0
    emit(Op::PUSH, 0, f.line);
    emit(Op::RET_VAL, 0, f.line);

    image.funcs[fi].locals = sym.localCount();
    sym.popScope();
    inFunc = false;
}

// ── Variable declaration ──────────────────────────────────────────────────────
void CodeGen::visit(VarDecl& v) {
    if (v.isGlobal) return;

    int slot = sym.allocLocal();
    Symbol s;
    s.type = v.type; s.slot = slot; s.arraySize = v.arraySize;
    s.isGlobal = false; s.isParam = false;
    sym.define(v.name, s);

    if (v.arraySize > 0) {
        emit(Op::ANEW, v.arraySize, v.line);
        emit(Op::STORE, slot, v.line);
    } else if (v.init) {
        v.init->accept(*this);
        emit(Op::STORE, slot, v.line);
    } else {
        emit(Op::PUSH, 0, v.line);
        emit(Op::STORE, slot, v.line);
    }
}

// ── Block ─────────────────────────────────────────────────────────────────────
void CodeGen::visit(Block& b) {
    sym.pushScope();
    for (auto& s : b.stmts) s->accept(*this);
    sym.popScope();
}

// ── If ────────────────────────────────────────────────────────────────────────
void CodeGen::visit(IfStmt& n) {
    n.cond->accept(*this);
    int jzIdx = emit(Op::JZ, 0, n.line);
    n.thenBranch->accept(*this);
    if (n.elseBranch) {
        int jmpIdx = emit(Op::JMP, 0, n.line);
        patch(jzIdx, here());
        n.elseBranch->accept(*this);
        patch(jmpIdx, here());
    } else {
        patch(jzIdx, here());
    }
}

// ── While ─────────────────────────────────────────────────────────────────────
void CodeGen::visit(WhileStmt& n) {
    int loopTop = here();
    n.cond->accept(*this);
    int jzIdx = emit(Op::JZ, 0, n.line);

    auto savedBreaks    = std::move(breakPatches);
    auto savedContinues = std::move(continuePatches);
    breakPatches.clear(); continuePatches.clear();

    n.body->accept(*this);

    for (int idx : continuePatches) patch(idx, loopTop);
    emit(Op::JMP, loopTop, n.line);
    patch(jzIdx, here());
    for (int idx : breakPatches) patch(idx, here());

    breakPatches    = std::move(savedBreaks);
    continuePatches = std::move(savedContinues);
}

// ── For ───────────────────────────────────────────────────────────────────────
void CodeGen::visit(ForStmt& n) {
    sym.pushScope();
    if (n.init) n.init->accept(*this);

    int loopTop = here();
    int jzIdx = -1;
    if (n.cond) {
        n.cond->accept(*this);
        jzIdx = emit(Op::JZ, 0, n.line);
    }

    auto savedBreaks    = std::move(breakPatches);
    auto savedContinues = std::move(continuePatches);
    breakPatches.clear(); continuePatches.clear();

    n.body->accept(*this);

    int continueTarget = here();
    for (int idx : continuePatches) patch(idx, continueTarget);

    if (n.post) {
        n.post->accept(*this);
        emit(Op::POP, 0, n.line);
    }

    emit(Op::JMP, loopTop, n.line);
    if (jzIdx >= 0) patch(jzIdx, here());
    for (int idx : breakPatches) patch(idx, here());

    breakPatches    = std::move(savedBreaks);
    continuePatches = std::move(savedContinues);
    sym.popScope();
}

// ── Return ────────────────────────────────────────────────────────────────────
void CodeGen::visit(ReturnStmt& n) {
    if (n.expr) {
        n.expr->accept(*this);
        emit(Op::RET_VAL, 0, n.line);
    } else {
        emit(Op::PUSH, 0, n.line);
        emit(Op::RET_VAL, 0, n.line);
    }
}

// ── Print ─────────────────────────────────────────────────────────────────────
void CodeGen::visit(PrintStmt& n) {
    if (auto* sl = dynamic_cast<StringLiteral*>(n.expr.get())) {
        int idx = internString(sl->value);
        emit(Op::PRINT_STR, idx, n.line);
    } else {
        n.expr->accept(*this);
        emit(Op::PRINT_INT, 0, n.line);
    }
}

// ── ExprStmt ──────────────────────────────────────────────────────────────────
void CodeGen::visit(ExprStmt& n) {
    n.expr->accept(*this);
    emit(Op::POP, 0, n.line);
}

// ── BinaryExpr ────────────────────────────────────────────────────────────────
void CodeGen::visit(BinaryExpr& n) {
    n.lhs->accept(*this);
    n.rhs->accept(*this);
    if      (n.op == "+")  emit(Op::ADD,  0, n.line);
    else if (n.op == "-")  emit(Op::SUB,  0, n.line);
    else if (n.op == "*")  emit(Op::MUL,  0, n.line);
    else if (n.op == "/")  emit(Op::DIV,  0, n.line);
    else if (n.op == "%")  emit(Op::MOD,  0, n.line);
    else if (n.op == "==") emit(Op::EQ,   0, n.line);
    else if (n.op == "!=") emit(Op::NEQ,  0, n.line);
    else if (n.op == "<")  emit(Op::LT,   0, n.line);
    else if (n.op == "<=") emit(Op::LTE,  0, n.line);
    else if (n.op == ">")  emit(Op::GT,   0, n.line);
    else if (n.op == ">=") emit(Op::GTE,  0, n.line);
    else if (n.op == "&&") emit(Op::AND,  0, n.line);
    else if (n.op == "||") emit(Op::OR,   0, n.line);
    else throw std::runtime_error("Unknown binary op: " + n.op);
}

// ── UnaryExpr ─────────────────────────────────────────────────────────────────
void CodeGen::visit(UnaryExpr& n) {
    if (n.op == "-") { n.operand->accept(*this); emit(Op::NEG, 0, n.line); return; }
    if (n.op == "!") { n.operand->accept(*this); emit(Op::NOT, 0, n.line); return; }

    // ++ / --
    auto getSlot = [&](bool isGlobal) -> int {
        if (auto* ve = dynamic_cast<VarExpr*>(n.operand.get()))
            return sym.lookup(ve->name).slot;
        throw std::runtime_error("++ / -- requires variable operand");
    };
    auto isGlobal = [&]() -> bool {
        if (auto* ve = dynamic_cast<VarExpr*>(n.operand.get()))
            return sym.lookup(ve->name).isGlobal;
        return false;
    };

    bool global = isGlobal();
    int  slot   = getSlot(global);

    if (global) emit(Op::GLOAD, slot, n.line);
    else        emit(Op::LOAD,  slot, n.line);

    if (n.prefix) {
        emit(Op::PUSH, 1, n.line);
        if (n.op == "++") emit(Op::ADD, 0, n.line);
        else               emit(Op::SUB, 0, n.line);
        emit(Op::DUP, 0, n.line);  // result to return
        if (global) emit(Op::GSTORE, slot, n.line);
        else        emit(Op::STORE,  slot, n.line);
    } else {
        // postfix: return old value, store new
        emit(Op::DUP, 0, n.line);  // original stays on stack as return value
        emit(Op::PUSH, 1, n.line);
        if (n.op == "++") emit(Op::ADD, 0, n.line);
        else               emit(Op::SUB, 0, n.line);
        if (global) emit(Op::GSTORE, slot, n.line);
        else        emit(Op::STORE,  slot, n.line);
        // original value is still on stack
    }
}

// ── AssignExpr ────────────────────────────────────────────────────────────────
void CodeGen::visit(AssignExpr& n) {
    if (n.op == "+=" || n.op == "-=") {
        n.target->accept(*this);
        n.value->accept(*this);
        if (n.op == "+=") emit(Op::ADD, 0, n.line);
        else               emit(Op::SUB, 0, n.line);
    } else {
        n.value->accept(*this);
    }
    emit(Op::DUP, 0, n.line);  // assignment is an expression, leave value on stack

    if (auto* ve = dynamic_cast<VarExpr*>(n.target.get())) {
        const Symbol& s = sym.lookup(ve->name);
        if (s.isGlobal) emit(Op::GSTORE, s.slot, n.line);
        else            emit(Op::STORE,  s.slot, n.line);
    } else if (auto* ae = dynamic_cast<ArraySubscript*>(n.target.get())) {
        // Stack: [val, val(dup)]
        // We need: [val, array_base, index] for ASTORE
        // Currently: [val(result), val(to_store)] — swap so to_store is below
        // Easier: emit store sequence manually without DUP for arrays
        // Pop the extra dup, then do array store
        emit(Op::POP, 0, n.line);  // discard dup
        // Now re-evaluate to get [val] for result, but we've already consumed it
        // Better approach: load array base and index BEFORE the dup
        // Restructure: value is on stack. Do: DUP, load_base, idx, ASTORE
        // But we already emitted DUP above. Let's just:
        //   stack now has: [val(to-store)]
        //   emit: DUP, base, index, ASTORE  → leaves [val] on stack
        emit(Op::DUP, 0, n.line);
        const Symbol& s = sym.lookup(ae->name);
        if (s.isGlobal) emit(Op::GLOAD, s.slot, n.line);
        else            emit(Op::LOAD,  s.slot, n.line);
        ae->index->accept(*this);
        emit(Op::ASTORE, 0, n.line);
    } else {
        throw std::runtime_error("Invalid assignment target");
    }
}

// ── CallExpr ──────────────────────────────────────────────────────────────────
void CodeGen::visit(CallExpr& n) {
    for (auto& arg : n.args) arg->accept(*this);
    // Emit CALL with placeholder; record for patching after all funcs compiled
    int site = emit(Op::CALL, 0, n.line);
    callPatches.emplace_back(site, n.callee);
}

// ── VarExpr ───────────────────────────────────────────────────────────────────
void CodeGen::visit(VarExpr& n) {
    const Symbol& s = sym.lookup(n.name);
    if (s.isGlobal) emit(Op::GLOAD, s.slot, n.line);
    else            emit(Op::LOAD,  s.slot, n.line);
}

// ── Literals ──────────────────────────────────────────────────────────────────
void CodeGen::visit(IntLiteral& n)    { emit(Op::PUSH, n.value, n.line); }
void CodeGen::visit(StringLiteral& n) { emit(Op::PUSH_STR, internString(n.value), n.line); }

// ── ArraySubscript (read) ────────────────────────────────────────────────────
void CodeGen::visit(ArraySubscript& n) {
    const Symbol& s = sym.lookup(n.name);
    if (s.isGlobal) emit(Op::GLOAD, s.slot, n.line);
    else            emit(Op::LOAD,  s.slot, n.line);
    n.index->accept(*this);
    emit(Op::ALOAD, 0, n.line);
}
