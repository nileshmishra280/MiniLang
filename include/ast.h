#pragma once
#include <memory>
#include <string>
#include <vector>

// ── Forward declarations ──────────────────────────────────────────────────────
struct ASTNode;
struct Program;
struct FuncDecl;
struct VarDecl;
struct Block;
struct IfStmt;
struct WhileStmt;
struct ForStmt;
struct ReturnStmt;
struct PrintStmt;
struct ExprStmt;
struct BinaryExpr;
struct UnaryExpr;
struct AssignExpr;
struct CallExpr;
struct VarExpr;
struct IntLiteral;
struct StringLiteral;
struct ArraySubscript;

using NodePtr = std::unique_ptr<ASTNode>;

// ── Visitor interface ─────────────────────────────────────────────────────────
struct Visitor {
    virtual ~Visitor() = default;
    virtual void visit(Program&)        = 0;
    virtual void visit(FuncDecl&)       = 0;
    virtual void visit(VarDecl&)        = 0;
    virtual void visit(Block&)          = 0;
    virtual void visit(IfStmt&)         = 0;
    virtual void visit(WhileStmt&)      = 0;
    virtual void visit(ForStmt&)        = 0;
    virtual void visit(ReturnStmt&)     = 0;
    virtual void visit(PrintStmt&)      = 0;
    virtual void visit(ExprStmt&)       = 0;
    virtual void visit(BinaryExpr&)     = 0;
    virtual void visit(UnaryExpr&)      = 0;
    virtual void visit(AssignExpr&)     = 0;
    virtual void visit(CallExpr&)       = 0;
    virtual void visit(VarExpr&)        = 0;
    virtual void visit(IntLiteral&)     = 0;
    virtual void visit(StringLiteral&)  = 0;
    virtual void visit(ArraySubscript&) = 0;
};

// ── Base node ─────────────────────────────────────────────────────────────────
struct ASTNode {
    int line = 0;
    virtual ~ASTNode() = default;
    virtual void accept(Visitor& v) = 0;
};

#define ACCEPT void accept(Visitor& v) override { v.visit(*this); }

// ── Top-level ─────────────────────────────────────────────────────────────────
struct Program : ASTNode {
    std::vector<std::unique_ptr<FuncDecl>> funcs;
    std::vector<std::unique_ptr<VarDecl>>  globals;
    ACCEPT
};

// ── Declarations ─────────────────────────────────────────────────────────────
struct VarDecl : ASTNode {
    std::string type;      // "int" | "char"
    std::string name;
    int         arraySize = 0;          // 0 = scalar
    NodePtr     init;                   // optional initializer
    bool        isGlobal = false;
    ACCEPT
};

struct FuncDecl : ASTNode {
    std::string retType;
    std::string name;
    std::vector<std::pair<std::string,std::string>> params; // (type, name)
    std::unique_ptr<Block> body;
    ACCEPT
};

// ── Statements ────────────────────────────────────────────────────────────────
struct Block : ASTNode {
    std::vector<NodePtr> stmts;
    ACCEPT
};

struct IfStmt : ASTNode {
    NodePtr cond;
    std::unique_ptr<Block> thenBranch;
    std::unique_ptr<Block> elseBranch;  // may be null
    ACCEPT
};

struct WhileStmt : ASTNode {
    NodePtr cond;
    std::unique_ptr<Block> body;
    ACCEPT
};

struct ForStmt : ASTNode {
    NodePtr init;   // VarDecl or ExprStmt (may be null)
    NodePtr cond;   // expr (may be null)
    NodePtr post;   // expr (may be null)
    std::unique_ptr<Block> body;
    ACCEPT
};

struct ReturnStmt : ASTNode {
    NodePtr expr;   // may be null (void return)
    ACCEPT
};

struct PrintStmt : ASTNode {
    NodePtr expr;
    ACCEPT
};

struct ExprStmt : ASTNode {
    NodePtr expr;
    ACCEPT
};

// ── Expressions ───────────────────────────────────────────────────────────────
struct BinaryExpr : ASTNode {
    std::string op;
    NodePtr lhs, rhs;
    ACCEPT
};

struct UnaryExpr : ASTNode {
    std::string op;     // "-", "!", "++", "--"
    NodePtr operand;
    bool prefix = true;
    ACCEPT
};

struct AssignExpr : ASTNode {
    std::string op;     // "=", "+=", "-="
    NodePtr target;     // VarExpr or ArraySubscript
    NodePtr value;
    ACCEPT
};

struct CallExpr : ASTNode {
    std::string callee;
    std::vector<NodePtr> args;
    ACCEPT
};

struct VarExpr : ASTNode {
    std::string name;
    ACCEPT
};

struct IntLiteral : ASTNode {
    int value;
    ACCEPT
};

struct StringLiteral : ASTNode {
    std::string value;
    ACCEPT
};

struct ArraySubscript : ASTNode {
    std::string name;
    NodePtr index;
    ACCEPT
};

#undef ACCEPT
