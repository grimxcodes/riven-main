#pragma once
#include "AST.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <memory>

class SemanticError : public std::runtime_error {
public:
    int line;
    SemanticError(const std::string& msg, int l = 0)
        : std::runtime_error(msg), line(l) {}
};

// Symbol kinds
enum class SymKind { Variable, Constant, Function, Type, Param };

struct Symbol {
    std::string name;
    std::string typeName;
    SymKind kind;
    bool isMut    = true;
    bool isFirm   = false;
    int  declLine = 0;
};

class Scope {
public:
    explicit Scope(Scope* parent = nullptr) : parent(parent) {}

    bool declare(const Symbol& sym);
    Symbol* lookup(const std::string& name);

    Scope* parent;
private:
    std::unordered_map<std::string, Symbol> table;
};

class SemanticAnalyzer {
public:
    SemanticAnalyzer();
    void analyze(Program& prog);

private:
    std::vector<std::unique_ptr<Scope>> scopes;
    Scope* currentScope;
    std::string currentFuncReturn;
    bool inUnsafe = false;
    bool inAsync  = false;
    bool inLoop   = false;

    void pushScope();
    void popScope();

    // ── Declarations ──────────────────────────────────────
    void analyzeDecl(Decl& d);
    void analyzeFuncDecl(FuncDecl& f);
    void analyzeStructDecl(StructDecl& s);
    void analyzeClassDecl(ClassDecl& c);
    void analyzeEnumDecl(EnumDecl& e);
    void analyzeNamespaceDecl(NamespaceDecl& n);
    void analyzeGlobalVar(GlobalVarDecl& g);
    void analyzeTraitDecl(TraitDecl& t);
    void analyzeImplDecl(ImplDecl& i);
    void analyzeRivenCore(RivenCoreEntry& entry);

    // ── Statements ────────────────────────────────────────
    void analyzeStmt(Stmt& s);
    void analyzeBlock(BlockStmt& b);
    void analyzeVarDecl(VarDecl& v);
    void analyzeIf(IfStmt& i);
    void analyzeWhile(WhileStmt& w);
    void analyzeDoWhile(DoWhileStmt& d);
    void analyzeFor(ForStmt& f);
    void analyzeReturn(ReturnStmt& r);
    void analyzeImprint(ImprintStmt& i);
    void analyzeScanq(ScanqStmt& s);
    void analyzeMatch(MatchStmt& m);
    void analyzeTry(TryStmt& t);
    void analyzeUnsafe(UnsafeBlock& u);
    void analyzeAsync(AsyncBlock& a);

    // ── Expressions ───────────────────────────────────────
    std::string analyzeExpr(Expr& e);
    std::string analyzeBinary(BinaryExpr& b);
    std::string analyzeUnary(UnaryExpr& u);
    std::string analyzeAssign(AssignExpr& a);
    std::string analyzeCall(CallExpr& c);
    std::string analyzeIndex(IndexExpr& i);
    std::string analyzeMember(MemberExpr& m);
    std::string analyzeTernary(TernaryExpr& t);

    // ── Helpers ───────────────────────────────────────────
    bool isNumericType(const std::string& t) const;
    bool typesCompatible(const std::string& a, const std::string& b) const;
    void declareBuiltins();
};

