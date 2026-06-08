#pragma once
#include "AST.h"
#include <string>
#include <sstream>
#include <unordered_set>

class CodeGenerator {
public:
    CodeGenerator();
    std::string generate(const Program& prog);

private:
    std::ostringstream out;
    int indentLevel;
    std::unordered_set<std::string> importedHeaders;
    bool inCoreEntry = false;          // true when generating riven core -> main()
    std::string currentReturnType;     // current function's return type

    // ── Helpers ───────────────────────────────────────────
    void emit(const std::string& s);
    void emitLine(const std::string& s = "");
    void indent();
    void dedent();
    std::string ind() const;

    // ── Types ─────────────────────────────────────────────
    std::string genType(const TypeNode& t) const;

    // ── Declarations ──────────────────────────────────────
    void genDecl(const Decl& d);
    void genFuncDecl(const FuncDecl& f);
    void genStructDecl(const StructDecl& s);
    void genClassDecl(const ClassDecl& c);
    void genEnumDecl(const EnumDecl& e);
    void genNamespaceDecl(const NamespaceDecl& n);
    void genGlobalVar(const GlobalVarDecl& g);
    void genImportDecl(const ImportDecl& imp);
    void genTraitDecl(const TraitDecl& t);
    void genImplDecl(const ImplDecl& i);
    void genRivenCore(const RivenCoreEntry& entry);

    // ── Statements ────────────────────────────────────────
    void genStmt(const Stmt& s);
    void genBlock(const BlockStmt& b);
    void genVarDecl(const VarDecl& v);
    void genIf(const IfStmt& i);
    void genWhile(const WhileStmt& w);
    void genDoWhile(const DoWhileStmt& d);
    void genFor(const ForStmt& f);
    void genReturn(const ReturnStmt& r);
    void genImprint(const ImprintStmt& i);
    void genScanq(const ScanqStmt& s);
    void genMatch(const MatchStmt& m);
    void genTry(const TryStmt& t);
    void genUnsafe(const UnsafeBlock& u);
    void genAsync(const AsyncBlock& a);
    void genDelete(const DeleteStmt& d);
    void genThrow(const ThrowStmt& t);

    // ── Expressions ───────────────────────────────────────
    std::string genExpr(const Expr& e);
    std::string genBinary(const BinaryExpr& b);
    std::string genUnary(const UnaryExpr& u);
    std::string genAssign(const AssignExpr& a);
    std::string genCall(const CallExpr& c);
    std::string genIndex(const IndexExpr& i);
    std::string genMember(const MemberExpr& m);
    std::string genTernary(const TernaryExpr& t);
    std::string genNew(const NewExpr& n);
    std::string genCast(const CastExpr& c);
    std::string genLambda(const LambdaExpr& l);
    std::string genSizeof(const SizeofExpr& s);

    // ── Preamble ──────────────────────────────────────────
    void emitPreamble();

    static std::string escapeString(const std::string& s);
};

