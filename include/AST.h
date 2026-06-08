#pragma once
#include "Token.h"
#include <string>
#include <vector>
#include <memory>
#include <optional>

// Forward declarations
struct ASTNode;
struct Expr;
struct Stmt;
struct Decl;

using ASTNodePtr = std::unique_ptr<ASTNode>;
using ExprPtr    = std::unique_ptr<Expr>;
using StmtPtr    = std::unique_ptr<Stmt>;
using DeclPtr    = std::unique_ptr<Decl>;

// ─── Type representation ───────────────────────────────────────────────────

struct TypeNode {
    std::string name;              // int, float, string, void, custom…
    bool isPointer  = false;
    bool isArray    = false;
    bool isMut      = false;
    std::optional<int> arraySize;
    std::unique_ptr<TypeNode> inner; // for pointer-to-type

    TypeNode() = default;
    TypeNode(std::string n) : name(std::move(n)) {}
};
using TypeNodePtr = std::unique_ptr<TypeNode>;

// ─── Base nodes ────────────────────────────────────────────────────────────

struct ASTNode {
    int line = 0;
    virtual ~ASTNode() = default;
};

struct Expr : ASTNode {
    virtual ~Expr() = default;
};

struct Stmt : ASTNode {
    virtual ~Stmt() = default;
};

struct Decl : ASTNode {
    virtual ~Decl() = default;
};

// ═══════════════════════════════════════════════════════
//  E X P R E S S I O N S
// ═══════════════════════════════════════════════════════

struct IntLiteral : Expr {
    long long value;
    explicit IntLiteral(long long v) : value(v) {}
};

struct FloatLiteral : Expr {
    double value;
    explicit FloatLiteral(double v) : value(v) {}
};

struct StringLiteral : Expr {
    std::string value;
    explicit StringLiteral(std::string v) : value(std::move(v)) {}
};

struct CharLiteral : Expr {
    char value;
    explicit CharLiteral(char v) : value(v) {}
};

struct BoolLiteral : Expr {
    bool value;
    explicit BoolLiteral(bool v) : value(v) {}
};

struct NullLiteral : Expr {};

struct Identifier : Expr {
    std::string name;
    explicit Identifier(std::string n) : name(std::move(n)) {}
};

struct BinaryExpr : Expr {
    std::string op;
    ExprPtr left, right;
    BinaryExpr(std::string o, ExprPtr l, ExprPtr r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
};

struct UnaryExpr : Expr {
    std::string op;
    ExprPtr operand;
    bool postfix;
    UnaryExpr(std::string o, ExprPtr expr, bool post = false)
        : op(std::move(o)), operand(std::move(expr)), postfix(post) {}
};

struct AssignExpr : Expr {
    ExprPtr target;
    std::string op;   // = += -= *= /=
    ExprPtr value;
    AssignExpr(ExprPtr t, std::string o, ExprPtr v)
        : target(std::move(t)), op(std::move(o)), value(std::move(v)) {}
};

struct CallExpr : Expr {
    ExprPtr callee;
    std::vector<ExprPtr> args;
    CallExpr(ExprPtr c, std::vector<ExprPtr> a)
        : callee(std::move(c)), args(std::move(a)) {}
};

struct IndexExpr : Expr {
    ExprPtr object, index;
    IndexExpr(ExprPtr o, ExprPtr i) : object(std::move(o)), index(std::move(i)) {}
};

struct MemberExpr : Expr {
    ExprPtr object;
    std::string member;
    bool isArrow;  // -> vs .
    MemberExpr(ExprPtr o, std::string m, bool arrow)
        : object(std::move(o)), member(std::move(m)), isArrow(arrow) {}
};

struct TernaryExpr : Expr {
    ExprPtr cond, thenExpr, elseExpr;
    TernaryExpr(ExprPtr c, ExprPtr t, ExprPtr e)
        : cond(std::move(c)), thenExpr(std::move(t)), elseExpr(std::move(e)) {}
};

struct SizeofExpr : Expr {
    TypeNodePtr ofType;
    ExprPtr ofExpr;
};

struct CastExpr : Expr {
    TypeNodePtr targetType;
    ExprPtr expr;
    CastExpr(TypeNodePtr t, ExprPtr e) : targetType(std::move(t)), expr(std::move(e)) {}
};

struct LambdaExpr : Expr {
    struct Param { TypeNodePtr type; std::string name; };
    std::vector<Param> params;
    TypeNodePtr returnType;
    std::unique_ptr<struct BlockStmt> body;
};

struct NewExpr : Expr {
    TypeNodePtr type;
    std::vector<ExprPtr> args;
};

// ═══════════════════════════════════════════════════════
//  S T A T E M E N T S
// ═══════════════════════════════════════════════════════

struct BlockStmt : Stmt {
    std::vector<StmtPtr> stmts;
};

struct ExprStmt : Stmt {
    ExprPtr expr;
    explicit ExprStmt(ExprPtr e) : expr(std::move(e)) {}
};

struct VarDecl : Stmt {
    TypeNodePtr type;
    std::string name;
    ExprPtr init;      // optional
    bool isFirm;       // firm = const
    bool isMut;
    bool isStatic;
    VarDecl() : isFirm(false), isMut(false), isStatic(false) {}
};

struct ReturnStmt : Stmt {
    ExprPtr value;     // null => return fin (void)
    explicit ReturnStmt(ExprPtr v = nullptr) : value(std::move(v)) {}
};

struct IfStmt : Stmt {
    ExprPtr cond;
    StmtPtr thenBranch;
    // altif chains
    struct AltifClause {
        ExprPtr cond;
        StmtPtr body;
    };
    std::vector<AltifClause> altifs;
    StmtPtr elseBranch;
};

struct WhileStmt : Stmt {
    ExprPtr cond;
    StmtPtr body;
};

struct DoWhileStmt : Stmt {
    StmtPtr body;
    ExprPtr cond;
};

struct ForStmt : Stmt {
    StmtPtr init;
    ExprPtr cond;
    ExprPtr update;
    StmtPtr body;
};

struct BreakStmt    : Stmt {};
struct ContinueStmt : Stmt {};

struct ImprintStmt : Stmt {
    std::vector<ExprPtr> args;
    bool newline = true;
};

struct ScanqStmt : Stmt {
    std::string varName;
    ExprPtr target;    // variable to store into
};

struct MatchStmt : Stmt {
    ExprPtr expr;
    struct Case {
        ExprPtr pattern;   // null = default
        StmtPtr body;
    };
    std::vector<Case> cases;
};

struct UnsafeBlock : Stmt {
    std::unique_ptr<BlockStmt> body;
};

struct AsyncBlock : Stmt {
    std::unique_ptr<BlockStmt> body;
};

struct TryStmt : Stmt {
    std::unique_ptr<BlockStmt> tryBlock;
    struct CatchClause {
        TypeNodePtr type;
        std::string name;
        std::unique_ptr<BlockStmt> body;
    };
    std::vector<CatchClause> catches;
};

struct ThrowStmt : Stmt {
    ExprPtr expr;
};

struct DeleteStmt : Stmt {
    ExprPtr ptr;
};

// ═══════════════════════════════════════════════════════
//  D E C L A R A T I O N S
// ═══════════════════════════════════════════════════════

struct Param {
    TypeNodePtr type;
    std::string name;
    ExprPtr defaultVal;
};

struct FuncDecl : Decl {
    std::string name;
    TypeNodePtr returnType;
    std::vector<Param> params;
    std::unique_ptr<BlockStmt> body;   // null = forward decl
    bool isInline, isStatic, isAsync, isExtern;
    std::string access;  // pub/priv/prot
    FuncDecl() : isInline(false), isStatic(false), isAsync(false), isExtern(false) {}
};

struct StructDecl : Decl {
    std::string name;
    struct Field {
        TypeNodePtr type;
        std::string name;
        ExprPtr defaultVal;
        std::string access;
    };
    std::vector<Field> fields;
    std::vector<std::unique_ptr<FuncDecl>> methods;
};

struct ClassDecl : Decl {
    std::string name;
    std::string base;     // single inheritance
    struct Member {
        TypeNodePtr type;
        std::string name;
        ExprPtr defaultVal;
        std::string access;
    };
    std::vector<Member> members;
    std::vector<std::unique_ptr<FuncDecl>> methods;
};

struct EnumDecl : Decl {
    std::string name;
    struct Enumerator {
        std::string name;
        ExprPtr value;
    };
    std::vector<Enumerator> enumerators;
};

struct NamespaceDecl : Decl {
    std::string name;
    std::vector<DeclPtr> body;
};

struct ImportDecl : Decl {
    std::string path;
};

struct GlobalVarDecl : Decl {
    TypeNodePtr type;
    std::string name;
    ExprPtr init;
    bool isFirm;
    bool isStatic;
    GlobalVarDecl() : isFirm(false), isStatic(false) {}
};

struct TraitDecl : Decl {
    std::string name;
    std::vector<std::unique_ptr<FuncDecl>> methods;
};

struct ImplDecl : Decl {
    std::string typeName;
    std::string traitName;  // optional
    std::vector<std::unique_ptr<FuncDecl>> methods;
};

// ═══════════════════════════════════════════════════════
//  P R O G R A M
// ═══════════════════════════════════════════════════════

struct RivenCoreEntry {
    std::unique_ptr<BlockStmt> body;
    int line = 0;
};

struct Program {
    std::vector<DeclPtr>  decls;
    std::unique_ptr<RivenCoreEntry> coreEntry;  // mandatory entry
};

