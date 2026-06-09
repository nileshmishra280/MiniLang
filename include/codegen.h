#pragma once
#include "ast.h"
#include "bytecode.h"
#include "symbol_table.h"
#include <vector>
#include <string>
#include <unordered_map>

class CodeGen : public Visitor {
public:
    BytecodeImage generate(Program& prog);

    void visit(Program&)        override;
    void visit(FuncDecl&)       override;
    void visit(VarDecl&)        override;
    void visit(Block&)          override;
    void visit(IfStmt&)         override;
    void visit(WhileStmt&)      override;
    void visit(ForStmt&)        override;
    void visit(ReturnStmt&)     override;
    void visit(PrintStmt&)      override;
    void visit(ExprStmt&)       override;
    void visit(BinaryExpr&)     override;
    void visit(UnaryExpr&)      override;
    void visit(AssignExpr&)     override;
    void visit(CallExpr&)       override;
    void visit(VarExpr&)        override;
    void visit(IntLiteral&)     override;
    void visit(StringLiteral&)  override;
    void visit(ArraySubscript&) override;

private:
    BytecodeImage  image;
    SymbolTable    sym;
    std::unordered_map<std::string, int> funcIndex;  // name → index in image.funcs
    bool           inFunc = false;

    // Two-pass call resolution: (instruction_index, callee_name)
    std::vector<std::pair<int,std::string>> callPatches;

    int  emit(Op op, int32_t arg = 0, int line = 0);
    void patch(int idx, int32_t newArg);
    int  here() const;
    int  internString(const std::string& s);

    std::vector<int> breakPatches;
    std::vector<int> continuePatches;
};
