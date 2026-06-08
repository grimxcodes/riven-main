#pragma once
#include "Token.h"
#include "AST.h"
#include <vector>
#include <stdexcept>
#include <string>

class ParseError : public std::runtime_error {
public:
    int line, col;
    ParseError(const std::string& msg, int l, int c)
        : std::runtime_error(msg), line(l), col(c) {}
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    Program parse();

private:
    std::vector<Token> tokens;
    size_t pos;

    // ── Token helpers ──────────────────────────────────────
    Token& current();
    Token& peek(int offset = 1);
    Token& advance();
    bool check(TokenType t) const;
    bool checkAny(std::initializer_list<TokenType> types) const;
    bool match(TokenType t);
    bool matchAny(std::initializer_list<TokenType> types);
    Token expect(TokenType t, const std::string& msg = "");
    bool isAtEnd() const;

    // ── Top-level ──────────────────────────────────────────
    DeclPtr parseTopLevel();
    std::unique_ptr<ImportDecl>     parseImport();
    std::unique_ptr<FuncDecl>       parseFuncDecl(const std::string& access = "");
    std::unique_ptr<StructDecl>     parseStructDecl();
    std::unique_ptr<ClassDecl>      parseClassDecl();
    std::unique_ptr<EnumDecl>       parseEnumDecl();
    std::unique_ptr<NamespaceDecl>  parseNamespaceDecl();
    std::unique_ptr<TraitDecl>      parseTraitDecl();
    std::unique_ptr<ImplDecl>       parseImplDecl();
    std::unique_ptr<GlobalVarDecl>  parseGlobalVar();
    std::unique_ptr<RivenCoreEntry> parseRivenCore();

    // ── Statements ─────────────────────────────────────────
    StmtPtr parseStmt();
    std::unique_ptr<BlockStmt>  parseBlock();
    std::unique_ptr<VarDecl>    parseVarDecl();
    std::unique_ptr<IfStmt>     parseIf();
    std::unique_ptr<WhileStmt>  parseWhile();
    std::unique_ptr<DoWhileStmt>parseDoWhile();
    std::unique_ptr<ForStmt>    parseFor();
    std::unique_ptr<ReturnStmt> parseReturn();
    std::unique_ptr<ImprintStmt>parseImprint();
    std::unique_ptr<ScanqStmt>  parseScanq();
    std::unique_ptr<MatchStmt>  parseMatch();
    std::unique_ptr<TryStmt>    parseTry();
    std::unique_ptr<ThrowStmt>  parseThrow();
    std::unique_ptr<UnsafeBlock>parseUnsafe();
    std::unique_ptr<AsyncBlock> parseAsync();
    std::unique_ptr<DeleteStmt> parseDelete();

    // ── Expressions (Pratt parser) ─────────────────────────
    ExprPtr parseExpr(int minPrec = 0);
    ExprPtr parseUnary();
    ExprPtr parsePostfix(ExprPtr left);
    ExprPtr parsePrimary();
    ExprPtr parseCallArgs(ExprPtr callee);
    ExprPtr parseLambda();
    ExprPtr parseNew();
    ExprPtr parseSizeof();

    int getBinaryPrec(const Token& t) const;
    bool isBinaryOp(const Token& t) const;
    std::string tokenToOp(const Token& t) const;

    // ── Types ──────────────────────────────────────────────
    TypeNodePtr parseType();

    // ── Params ─────────────────────────────────────────────
    std::vector<Param> parseParamList();
    Param parseParam();

    void error(const std::string& msg);
    void error(const std::string& msg, const Token& t);
};

