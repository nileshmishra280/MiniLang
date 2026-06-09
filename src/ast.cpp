#include "ast.h"
#include <iostream>
#include <string>

// Simple AST pretty-printer (useful for --dump-ast flag)
struct ASTPrinter : Visitor {
    int depth = 0;
    std::ostream& out;
    explicit ASTPrinter(std::ostream& o) : out(o) {}

    void indent() { for (int i = 0; i < depth; ++i) out << "  "; }

    void visit(Program& n) override {
        out << "Program\n";
        ++depth;
        for (auto& g : n.globals)  g->accept(*this);
        for (auto& f : n.funcs)    f->accept(*this);
        --depth;
    }
    void visit(FuncDecl& n) override {
        indent(); out << "FuncDecl " << n.retType << " " << n.name << "(";
        for (size_t i = 0; i < n.params.size(); ++i) {
            out << n.params[i].first << " " << n.params[i].second;
            if (i+1 < n.params.size()) out << ", ";
        }
        out << ")\n";
        ++depth; n.body->accept(*this); --depth;
    }
    void visit(VarDecl& n) override {
        indent(); out << "VarDecl " << n.type << " " << n.name;
        if (n.arraySize) out << "[" << n.arraySize << "]";
        out << "\n";
        if (n.init) { ++depth; n.init->accept(*this); --depth; }
    }
    void visit(Block& n) override {
        indent(); out << "Block\n";
        ++depth;
        for (auto& s : n.stmts) s->accept(*this);
        --depth;
    }
    void visit(IfStmt& n) override {
        indent(); out << "If\n";
        ++depth; n.cond->accept(*this); n.thenBranch->accept(*this);
        if (n.elseBranch) n.elseBranch->accept(*this);
        --depth;
    }
    void visit(WhileStmt& n) override {
        indent(); out << "While\n";
        ++depth; n.cond->accept(*this); n.body->accept(*this); --depth;
    }
    void visit(ForStmt& n) override {
        indent(); out << "For\n";
        ++depth;
        if (n.init) n.init->accept(*this);
        if (n.cond) n.cond->accept(*this);
        if (n.post) n.post->accept(*this);
        n.body->accept(*this);
        --depth;
    }
    void visit(ReturnStmt& n) override {
        indent(); out << "Return\n";
        if (n.expr) { ++depth; n.expr->accept(*this); --depth; }
    }
    void visit(PrintStmt& n) override {
        indent(); out << "Print\n";
        ++depth; n.expr->accept(*this); --depth;
    }
    void visit(ExprStmt& n) override {
        indent(); out << "ExprStmt\n";
        ++depth; n.expr->accept(*this); --depth;
    }
    void visit(BinaryExpr& n) override {
        indent(); out << "Binary(" << n.op << ")\n";
        ++depth; n.lhs->accept(*this); n.rhs->accept(*this); --depth;
    }
    void visit(UnaryExpr& n) override {
        indent(); out << "Unary(" << n.op << (n.prefix?",pre":",post") << ")\n";
        ++depth; n.operand->accept(*this); --depth;
    }
    void visit(AssignExpr& n) override {
        indent(); out << "Assign(" << n.op << ")\n";
        ++depth; n.target->accept(*this); n.value->accept(*this); --depth;
    }
    void visit(CallExpr& n) override {
        indent(); out << "Call " << n.callee << "\n";
        ++depth;
        for (auto& a : n.args) a->accept(*this);
        --depth;
    }
    void visit(VarExpr& n)        override { indent(); out << "Var " << n.name << "\n"; }
    void visit(IntLiteral& n)     override { indent(); out << "Int " << n.value << "\n"; }
    void visit(StringLiteral& n)  override { indent(); out << "Str \"" << n.value << "\"\n"; }
    void visit(ArraySubscript& n) override {
        indent(); out << "ArraySub " << n.name << "\n";
        ++depth; n.index->accept(*this); --depth;
    }
};

void printAST(ASTNode& root, std::ostream& out) {
    ASTPrinter p(out);
    root.accept(p);
}
