#include "Parser.h"
#include <sstream>
#include <stdexcept>
#include <iostream>

// ── Helpers ────────────────────────────────────────────────────────────────

Parser::Parser(std::vector<Token> toks) : tokens(std::move(toks)), pos(0) {}

Token& Parser::current() { return tokens[pos]; }
Token& Parser::peek(int offset) {
    size_t p = pos + offset;
    if (p >= tokens.size()) return tokens.back();
    return tokens[p];
}
Token& Parser::advance() {
    Token& t = tokens[pos];
    if (pos + 1 < tokens.size()) ++pos;
    return t;
}
bool Parser::check(TokenType t) const { return tokens[pos].type == t; }
bool Parser::checkAny(std::initializer_list<TokenType> types) const {
    for (auto t : types) if (tokens[pos].type == t) return true;
    return false;
}
bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}
bool Parser::matchAny(std::initializer_list<TokenType> types) {
    for (auto t : types) if (match(t)) return true;
    return false;
}
Token Parser::expect(TokenType t, const std::string& msg) {
    if (!check(t)) {
        std::string m = msg.empty()
            ? "Expected '" + Token(t,"",0,0).typeToString() + "' but got '" + current().value + "'"
            : msg;
        error(m);
    }
    return advance();
}
bool Parser::isAtEnd() const { return tokens[pos].type == TokenType::EOF_TOKEN; }
void Parser::error(const std::string& msg) {
    throw ParseError("[line " + std::to_string(current().line) + "] Parse error: " + msg,
                     current().line, current().col);
}
void Parser::error(const std::string& msg, const Token& t) {
    throw ParseError("[line " + std::to_string(t.line) + "] Parse error: " + msg, t.line, t.col);
}

// ── Top-level parse ────────────────────────────────────────────────────────

Program Parser::parse() {
    Program prog;
    while (!isAtEnd()) {
        // consistof import
        if (check(TokenType::CONSISTOF)) {
            prog.decls.push_back(parseImport());
            continue;
        }
        // riven core entry point
        if (check(TokenType::RIVEN)) {
            if (prog.coreEntry) error("Duplicate 'riven core' entry point");
            prog.coreEntry = parseRivenCore();
            continue;
        }
        prog.decls.push_back(parseTopLevel());
    }
    if (!prog.coreEntry) {
        std::cerr << "[Warning] No 'riven core { }' entry point found.\n";
    }
    return prog;
}

DeclPtr Parser::parseTopLevel() {
    // Access modifiers
    std::string access;
    if (checkAny({TokenType::PUB, TokenType::PRIV, TokenType::PROT})) {
        access = current().value;
        advance();
    }

    if (check(TokenType::STRUCT))    return parseStructDecl();
    if (check(TokenType::CLASS))     return parseClassDecl();
    if (check(TokenType::ENUM))      return parseEnumDecl();
    if (check(TokenType::NAMESPACE)) return parseNamespaceDecl();
    if (check(TokenType::TRAIT))     return parseTraitDecl();
    if (check(TokenType::IMPL))      return parseImplDecl();

    // inline/static/extern/async prefixes before function
    bool isInline = false, isStatic = false, isExtern = false, isAsync = false;
    while (checkAny({TokenType::INLINE, TokenType::STATIC, TokenType::EXTERN, TokenType::ASYNC})) {
        if (check(TokenType::INLINE))  { isInline = true; advance(); }
        else if (check(TokenType::STATIC)) { isStatic = true; advance(); }
        else if (check(TokenType::EXTERN)) { isExtern = true; advance(); }
        else if (check(TokenType::ASYNC))  { isAsync  = true; advance(); }
    }

    // Look ahead: type IDENTIFIER '(' → function
    // type IDENTIFIER ';'/'=' → global var
    // We need to parse the type then decide

    // firm global constant
    if (check(TokenType::FIRM)) {
        auto gv = parseGlobalVar();
        return gv;
    }

    // Try parsing as type + name → func or global var
    size_t saved = pos;
    try {
        TypeNodePtr ty = parseType();
        if (check(TokenType::IDENTIFIER)) {
            std::string name = current().value;
            advance();
            if (check(TokenType::LPAREN)) {
                // Function declaration
                pos = saved;
                auto fd = parseFuncDecl(access);
                fd->isInline = isInline;
                fd->isStatic = isStatic;
                fd->isExtern = isExtern;
                fd->isAsync  = isAsync;
                return fd;
            } else {
                // Global variable
                pos = saved;
                return parseGlobalVar();
            }
        }
    } catch (...) {
        pos = saved;
    }

    // Last resort: try as function
    return parseFuncDecl(access);
}

std::unique_ptr<ImportDecl> Parser::parseImport() {
    expect(TokenType::CONSISTOF);
    auto imp = std::make_unique<ImportDecl>();
    imp->line = current().line;
    if (check(TokenType::STRING)) {
        imp->path = current().value;
        advance();
    } else if (check(TokenType::IDENTIFIER)) {
        // consistof file.rh (no quotes)
        imp->path = current().value; advance();
        while (match(TokenType::DOT)) {
            imp->path += ".";
            imp->path += current().value; advance();
        }
    } else {
        error("Expected file path after 'consistof'");
    }
    match(TokenType::SEMICOLON);
    return imp;
}

std::unique_ptr<RivenCoreEntry> Parser::parseRivenCore() {
    int ln = current().line;
    expect(TokenType::RIVEN);
    expect(TokenType::CORE);
    auto entry = std::make_unique<RivenCoreEntry>();
    entry->line = ln;
    entry->body = parseBlock();
    return entry;
}

std::unique_ptr<FuncDecl> Parser::parseFuncDecl(const std::string& access) {
    auto fd = std::make_unique<FuncDecl>();
    fd->access = access;
    fd->line   = current().line;
    fd->returnType = parseType();
    fd->name = expect(TokenType::IDENTIFIER).value;
    expect(TokenType::LPAREN);
    fd->params = parseParamList();
    expect(TokenType::RPAREN);
    if (check(TokenType::SEMICOLON)) { advance(); return fd; } // forward decl
    fd->body = parseBlock();
    return fd;
}

std::unique_ptr<GlobalVarDecl> Parser::parseGlobalVar() {
    auto gv = std::make_unique<GlobalVarDecl>();
    gv->line = current().line;
    if (check(TokenType::FIRM)) { gv->isFirm = true; advance(); }
    if (check(TokenType::STATIC)) { gv->isStatic = true; advance(); }
    gv->type = parseType();
    gv->name = expect(TokenType::IDENTIFIER).value;
    if (match(TokenType::ASSIGN)) gv->init = parseExpr();
    expect(TokenType::SEMICOLON);
    return gv;
}

std::unique_ptr<StructDecl> Parser::parseStructDecl() {
    expect(TokenType::STRUCT);
    auto sd = std::make_unique<StructDecl>();
    sd->name = expect(TokenType::IDENTIFIER).value;
    expect(TokenType::LBRACE);

    std::string currentAcc = "pub"; // Struct members default to public

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        // Access specifier
        if (checkAny({TokenType::PUB, TokenType::PRIV, TokenType::PROT})) {
            currentAcc = current().value; advance();
            match(TokenType::COLON);
            continue;
        }

        // Try to parse type + name, then check what follows
        size_t saved = pos;
        try {
            TypeNodePtr ty = parseType();
            if (check(TokenType::IDENTIFIER)) {
                std::string name = current().value;
                advance();
                if (check(TokenType::LPAREN)) {
                    // It's a method: rewind and parse as full function
                    pos = saved;
                    auto meth = parseFuncDecl(currentAcc);
                    sd->methods.push_back(std::move(meth));
                } else {
                    // It's a field - check for C-style array suffix: int data[64]
                    StructDecl::Field f;
                    f.access = currentAcc;
                    f.type = std::move(ty);
                    f.name = name;
                    if (check(TokenType::LBRACKET)) {
                        advance();
                        f.type->isArray = true;
                        if (!check(TokenType::RBRACKET)) {
                            auto szExpr = parseExpr();
                            if (auto* il = dynamic_cast<IntLiteral*>(szExpr.get()))
                                f.type->arraySize = (int)il->value;
                        }
                        expect(TokenType::RBRACKET);
                    }
                    if (match(TokenType::ASSIGN)) f.defaultVal = parseExpr();
                    expect(TokenType::SEMICOLON);
                    sd->fields.push_back(std::move(f));
                }
            } else {
                pos = saved; advance(); // skip unknown
            }
        } catch (...) {
            pos = saved; advance(); // skip unknown to prevent infinite loop
        }
    }
    expect(TokenType::RBRACE);
    match(TokenType::SEMICOLON);
    return sd;
}

std::unique_ptr<ClassDecl> Parser::parseClassDecl() {
    expect(TokenType::CLASS);
    auto cd = std::make_unique<ClassDecl>();
    cd->name = expect(TokenType::IDENTIFIER).value;
    // Optional base class
    if (match(TokenType::COLON)) cd->base = expect(TokenType::IDENTIFIER).value;
    expect(TokenType::LBRACE);

    // In Riven class, default access is pub (like struct), changes with pub:/priv:/prot:
    std::string currentAcc = "pub";

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        // Check for access specifier labels: pub: priv: prot:
        if (checkAny({TokenType::PUB, TokenType::PRIV, TokenType::PROT})) {
            currentAcc = current().value; advance();
            match(TokenType::COLON);  // consume the colon after specifier
            continue;
        }
        size_t saved = pos;
        try {
            TypeNodePtr ty = parseType();
            if (check(TokenType::IDENTIFIER) && peek().type == TokenType::LPAREN) {
                pos = saved;
                auto meth = parseFuncDecl(currentAcc);
                cd->methods.push_back(std::move(meth));
            } else {
                ClassDecl::Member m;
                m.access = currentAcc;
                m.type = std::move(ty);
                m.name = expect(TokenType::IDENTIFIER).value;
                // C-style array suffix: int arr[5]
                if (check(TokenType::LBRACKET)) {
                    advance();
                    m.type->isArray = true;
                    if (!check(TokenType::RBRACKET)) {
                        auto szExpr = parseExpr();
                        if (auto* il = dynamic_cast<IntLiteral*>(szExpr.get()))
                            m.type->arraySize = (int)il->value;
                    }
                    expect(TokenType::RBRACKET);
                }
                if (match(TokenType::ASSIGN)) m.defaultVal = parseExpr();
                expect(TokenType::SEMICOLON);
                cd->members.push_back(std::move(m));
            }
        } catch (...) {
            pos = saved;
            advance(); // skip unknown token to avoid infinite loop
        }
    }
    expect(TokenType::RBRACE);
    match(TokenType::SEMICOLON);
    return cd;
}

std::unique_ptr<EnumDecl> Parser::parseEnumDecl() {
    expect(TokenType::ENUM);
    auto ed = std::make_unique<EnumDecl>();
    ed->name = expect(TokenType::IDENTIFIER).value;
    expect(TokenType::LBRACE);
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        EnumDecl::Enumerator en;
        en.name = expect(TokenType::IDENTIFIER).value;
        if (match(TokenType::ASSIGN)) en.value = parseExpr();
        match(TokenType::COMMA);
        ed->enumerators.push_back(std::move(en));
    }
    expect(TokenType::RBRACE);
    match(TokenType::SEMICOLON);
    return ed;
}

std::unique_ptr<NamespaceDecl> Parser::parseNamespaceDecl() {
    expect(TokenType::NAMESPACE);
    auto nd = std::make_unique<NamespaceDecl>();
    nd->name = expect(TokenType::IDENTIFIER).value;
    expect(TokenType::LBRACE);
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        nd->body.push_back(parseTopLevel());
    }
    expect(TokenType::RBRACE);
    return nd;
}

std::unique_ptr<TraitDecl> Parser::parseTraitDecl() {
    expect(TokenType::TRAIT);
    auto td = std::make_unique<TraitDecl>();
    td->name = expect(TokenType::IDENTIFIER).value;
    expect(TokenType::LBRACE);
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        td->methods.push_back(parseFuncDecl());
    }
    expect(TokenType::RBRACE);
    return td;
}

std::unique_ptr<ImplDecl> Parser::parseImplDecl() {
    expect(TokenType::IMPL);
    auto id = std::make_unique<ImplDecl>();
    id->typeName = expect(TokenType::IDENTIFIER).value;
    if (match(TokenType::COLON)) id->traitName = expect(TokenType::IDENTIFIER).value;
    expect(TokenType::LBRACE);
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        id->methods.push_back(parseFuncDecl());
    }
    expect(TokenType::RBRACE);
    return id;
}

// ── Types ──────────────────────────────────────────────────────────────────

TypeNodePtr Parser::parseType() {
    auto ty = std::make_unique<TypeNode>();
    if (check(TokenType::MUT)) { ty->isMut = true; advance(); }

    // Base type
    if (checkAny({TokenType::INT, TokenType::FLOAT_T, TokenType::CHAR_T,
                  TokenType::BOOL_T, TokenType::STRING_T, TokenType::VOID})) {
        ty->name = current().value; advance();
    } else if (check(TokenType::IDENTIFIER)) {
        ty->name = current().value; advance();
        // namespace::type
        while (check(TokenType::DOUBLE_COLON)) {
            advance();
            ty->name += "::" + current().value; advance();
        }
    } else {
        ty->name = "auto";
    }

    // Pointer
    while (check(TokenType::STAR)) {
        advance();
        auto ptr = std::make_unique<TypeNode>();
        ptr->isPointer = true;
        ptr->inner = std::move(ty);
        ty = std::move(ptr);
    }

    // Array
    if (check(TokenType::LBRACKET)) {
        advance();
        ty->isArray = true;
        if (!check(TokenType::RBRACKET)) {
            auto szExpr = parseExpr();
            if (auto* il = dynamic_cast<IntLiteral*>(szExpr.get()))
                ty->arraySize = (int)il->value;
        }
        expect(TokenType::RBRACKET);
    }

    return ty;
}

// ── Parameters ─────────────────────────────────────────────────────────────

std::vector<Param> Parser::parseParamList() {
    std::vector<Param> params;
    if (check(TokenType::RPAREN)) return params;
    params.push_back(parseParam());
    while (match(TokenType::COMMA)) {
        if (check(TokenType::ELLIPSIS)) { advance(); break; } // variadic
        params.push_back(parseParam());
    }
    return params;
}

Param Parser::parseParam() {
    Param p;
    p.type = parseType();
    if (check(TokenType::IDENTIFIER)) {
        p.name = current().value; advance();
    }
    if (match(TokenType::ASSIGN)) p.defaultVal = parseExpr();
    return p;
}

// ── Statements ─────────────────────────────────────────────────────────────

StmtPtr Parser::parseStmt() {
    if (check(TokenType::LBRACE))       return parseBlock();
    if (check(TokenType::IF))           return parseIf();
    if (check(TokenType::WHILE))        return parseWhile();
    if (check(TokenType::DO))           return parseDoWhile();
    if (check(TokenType::FOR))          return parseFor();
    if (check(TokenType::RETURN_FIN))   return parseReturn();
    if (check(TokenType::IMPRINT))      return parseImprint();
    if (check(TokenType::SCANQ))        return parseScanq();
    if (check(TokenType::MATCH))        return parseMatch();
    if (check(TokenType::TRY))          return parseTry();
    if (check(TokenType::THROW))        return parseThrow();
    if (check(TokenType::UNSAFE))       return parseUnsafe();
    if (check(TokenType::ASYNC))        return parseAsync();
    if (check(TokenType::DELETE_KW))    return parseDelete();
    if (check(TokenType::BREAK)) {
        advance(); match(TokenType::SEMICOLON);
        return std::make_unique<BreakStmt>();
    }
    if (check(TokenType::CONTINUE)) {
        advance(); match(TokenType::SEMICOLON);
        return std::make_unique<ContinueStmt>();
    }

    // Variable declaration: firm / let / mut
    if (check(TokenType::FIRM) || check(TokenType::LET) || check(TokenType::MUT)) {
        return parseVarDecl();
    }
    // Builtin type keyword followed by identifier = variable decl
    if (checkAny({TokenType::INT, TokenType::FLOAT_T, TokenType::CHAR_T,
                  TokenType::BOOL_T, TokenType::STRING_T, TokenType::VOID})) {
        size_t saved = pos;
        try {
            auto vd = parseVarDecl();
            return vd;
        } catch (...) { pos = saved; }
    }
    // IDENTIFIER IDENTIFIER  = variable decl (custom types like "MyStruct name")
    if (check(TokenType::IDENTIFIER) && peek().type == TokenType::IDENTIFIER) {
        return parseVarDecl();
    }
    // IDENTIFIER :: IDENTIFIER IDENTIFIER  = namespaced type var decl (e.g. "NS::Type name")
    if (check(TokenType::IDENTIFIER) && peek().type == TokenType::DOUBLE_COLON) {
        // Look ahead past all the :: to see if there's a final IDENTIFIER
        size_t saved = pos;
        try {
            auto vd = parseVarDecl();
            return vd;
        } catch (...) { pos = saved; }
    }

    // Expression statement
    auto expr = parseExpr();
    match(TokenType::SEMICOLON);
    return std::make_unique<ExprStmt>(std::move(expr));
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    expect(TokenType::LBRACE);
    auto blk = std::make_unique<BlockStmt>();
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        blk->stmts.push_back(parseStmt());
    }
    expect(TokenType::RBRACE);
    return blk;
}

std::unique_ptr<VarDecl> Parser::parseVarDecl() {
    auto vd = std::make_unique<VarDecl>();
    vd->line = current().line;
    if (check(TokenType::FIRM))   { vd->isFirm = true;  advance(); }
    if (check(TokenType::STATIC)) { vd->isStatic = true; advance(); }
    if (check(TokenType::LET))    { advance(); }
    if (check(TokenType::MUT))    { vd->isMut = true; advance(); }

    vd->type = parseType();
    vd->name = expect(TokenType::IDENTIFIER).value;

    // C-style array suffix: int arr[10]  (dimension after name, not after type)
    if (check(TokenType::LBRACKET)) {
        advance();
        if (!vd->type->isArray) {
            vd->type->isArray = true;
            if (!check(TokenType::RBRACKET)) {
                auto szExpr = parseExpr();
                if (auto* il = dynamic_cast<IntLiteral*>(szExpr.get()))
                    vd->type->arraySize = (int)il->value;
            }
        } else {
            // Already has array info from type, just skip
            if (!check(TokenType::RBRACKET)) parseExpr();
        }
        expect(TokenType::RBRACKET);
    }

    if (match(TokenType::ASSIGN)) vd->init = parseExpr();
    expect(TokenType::SEMICOLON);
    return vd;
}

std::unique_ptr<IfStmt> Parser::parseIf() {
    auto is = std::make_unique<IfStmt>();
    is->line = current().line;
    expect(TokenType::IF);
    expect(TokenType::LPAREN);
    is->cond = parseExpr();
    expect(TokenType::RPAREN);
    is->thenBranch = parseStmt();

    while (check(TokenType::ALTIF)) {
        advance();
        IfStmt::AltifClause alt;
        expect(TokenType::LPAREN);
        alt.cond = parseExpr();
        expect(TokenType::RPAREN);
        alt.body = parseStmt();
        is->altifs.push_back(std::move(alt));
    }

    if (match(TokenType::ELSE)) is->elseBranch = parseStmt();
    return is;
}

std::unique_ptr<WhileStmt> Parser::parseWhile() {
    auto ws = std::make_unique<WhileStmt>();
    ws->line = current().line;
    expect(TokenType::WHILE);
    expect(TokenType::LPAREN);
    ws->cond = parseExpr();
    expect(TokenType::RPAREN);
    ws->body = parseStmt();
    return ws;
}

std::unique_ptr<DoWhileStmt> Parser::parseDoWhile() {
    auto dw = std::make_unique<DoWhileStmt>();
    expect(TokenType::DO);
    dw->body = parseStmt();
    expect(TokenType::WHILE);
    expect(TokenType::LPAREN);
    dw->cond = parseExpr();
    expect(TokenType::RPAREN);
    expect(TokenType::SEMICOLON);
    return dw;
}

std::unique_ptr<ForStmt> Parser::parseFor() {
    auto fs = std::make_unique<ForStmt>();
    fs->line = current().line;
    expect(TokenType::FOR);
    expect(TokenType::LPAREN);
    if (!check(TokenType::SEMICOLON)) fs->init = parseVarDecl();
    else { expect(TokenType::SEMICOLON); }
    if (!check(TokenType::SEMICOLON)) fs->cond = parseExpr();
    expect(TokenType::SEMICOLON);
    if (!check(TokenType::RPAREN)) fs->update = parseExpr();
    expect(TokenType::RPAREN);
    fs->body = parseStmt();
    return fs;
}

std::unique_ptr<ReturnStmt> Parser::parseReturn() {
    int ln = current().line;
    // consume 'return' or 'fin' (both map to RETURN_FIN)
    advance();
    // optional 'fin' after 'return'
    if (check(TokenType::RETURN_FIN)) advance();

    auto rs = std::make_unique<ReturnStmt>();
    rs->line = ln;
    if (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE)) {
        rs->value = parseExpr();
    }
    match(TokenType::SEMICOLON);
    return rs;
}

std::unique_ptr<ImprintStmt> Parser::parseImprint() {
    int ln = current().line;
    expect(TokenType::IMPRINT);
    expect(TokenType::LPAREN);
    auto is = std::make_unique<ImprintStmt>();
    is->line = ln;
    if (!check(TokenType::RPAREN)) {
        is->args.push_back(parseExpr());
        while (match(TokenType::COMMA)) is->args.push_back(parseExpr());
    }
    expect(TokenType::RPAREN);
    match(TokenType::SEMICOLON);
    return is;
}

std::unique_ptr<ScanqStmt> Parser::parseScanq() {
    int ln = current().line;
    expect(TokenType::SCANQ);
    auto ss = std::make_unique<ScanqStmt>();
    ss->line = ln;
    expect(TokenType::LPAREN);
    ss->target = parseExpr();
    expect(TokenType::RPAREN);
    match(TokenType::SEMICOLON);
    return ss;
}

std::unique_ptr<MatchStmt> Parser::parseMatch() {
    expect(TokenType::MATCH);
    auto ms = std::make_unique<MatchStmt>();
    expect(TokenType::LPAREN);
    ms->expr = parseExpr();
    expect(TokenType::RPAREN);
    expect(TokenType::LBRACE);
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        MatchStmt::Case c;
        if (check(TokenType::DEFAULT)) { advance(); c.pattern = nullptr; }
        else { expect(TokenType::CASE); c.pattern = parseExpr(); }
        expect(TokenType::COLON);
        c.body = parseStmt();
        ms->cases.push_back(std::move(c));
    }
    expect(TokenType::RBRACE);
    return ms;
}

std::unique_ptr<TryStmt> Parser::parseTry() {
    expect(TokenType::TRY);
    auto ts = std::make_unique<TryStmt>();
    ts->tryBlock = parseBlock();
    while (check(TokenType::CATCH)) {
        advance();
        TryStmt::CatchClause cc;
        expect(TokenType::LPAREN);
        cc.type = parseType();
        if (check(TokenType::IDENTIFIER)) { cc.name = current().value; advance(); }
        expect(TokenType::RPAREN);
        cc.body = parseBlock();
        ts->catches.push_back(std::move(cc));
    }
    return ts;
}

std::unique_ptr<ThrowStmt> Parser::parseThrow() {
    expect(TokenType::THROW);
    auto ts = std::make_unique<ThrowStmt>();
    ts->expr = parseExpr();
    match(TokenType::SEMICOLON);
    return ts;
}

std::unique_ptr<UnsafeBlock> Parser::parseUnsafe() {
    expect(TokenType::UNSAFE);
    auto ub = std::make_unique<UnsafeBlock>();
    ub->body = parseBlock();
    return ub;
}

std::unique_ptr<AsyncBlock> Parser::parseAsync() {
    expect(TokenType::ASYNC);
    auto ab = std::make_unique<AsyncBlock>();
    ab->body = parseBlock();
    return ab;
}

std::unique_ptr<DeleteStmt> Parser::parseDelete() {
    expect(TokenType::DELETE_KW);
    auto ds = std::make_unique<DeleteStmt>();
    ds->ptr = parseExpr();
    match(TokenType::SEMICOLON);
    return ds;
}

// ── Pratt Expression Parser ────────────────────────────────────────────────

int Parser::getBinaryPrec(const Token& t) const {
    switch (t.type) {
        case TokenType::OR:           return 1;
        case TokenType::AND:          return 2;
        case TokenType::BITOR:        return 3;
        case TokenType::BITXOR:       return 4;
        case TokenType::BITAND:       return 5;
        case TokenType::EQ:
        case TokenType::NEQ:          return 6;
        case TokenType::LT:
        case TokenType::GT:
        case TokenType::LTE:
        case TokenType::GTE:          return 7;
        case TokenType::LSHIFT:
        case TokenType::RSHIFT:       return 8;
        case TokenType::PLUS:
        case TokenType::MINUS:        return 9;
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::PERCENT:      return 10;
        default:                      return -1;
    }
}

bool Parser::isBinaryOp(const Token& t) const { return getBinaryPrec(t) >= 0; }

std::string Parser::tokenToOp(const Token& t) const { return t.value; }

ExprPtr Parser::parseExpr(int minPrec) {
    // Assignment check (right-associative, lowest precedence)
    auto left = parseUnary();

    // Ternary
    if (check(TokenType::QUESTION)) {
        advance();
        auto thenE = parseExpr();
        expect(TokenType::COLON);
        auto elseE = parseExpr();
        return std::make_unique<TernaryExpr>(std::move(left), std::move(thenE), std::move(elseE));
    }

    // Assignment operators (right-associative)
    if (checkAny({TokenType::ASSIGN, TokenType::PLUS_ASSIGN, TokenType::MINUS_ASSIGN,
                   TokenType::STAR_ASSIGN, TokenType::SLASH_ASSIGN})) {
        std::string op = current().value; advance();
        auto right = parseExpr(0);
        return std::make_unique<AssignExpr>(std::move(left), op, std::move(right));
    }

    // Binary operators (Pratt)
    while (isBinaryOp(current()) && getBinaryPrec(current()) > minPrec) {
        int prec = getBinaryPrec(current());
        std::string op = tokenToOp(current()); advance();
        auto right = parseExpr(prec); // left-assoc: prec; right-assoc: prec-1
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }

    return left;
}

ExprPtr Parser::parseUnary() {
    // Prefix unary
    if (checkAny({TokenType::MINUS, TokenType::NOT, TokenType::BITNOT, TokenType::STAR, TokenType::BITAND})) {
        std::string op = current().value; int ln = current().line; advance();
        auto operand = parseUnary();
        auto ue = std::make_unique<UnaryExpr>(op, std::move(operand));
        ue->line = ln; return ue;
    }
    // Prefix increment/decrement
    if (check(TokenType::PLUS_ARROW)) {
        advance();
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>("+>", std::move(operand), false);
    }
    if (check(TokenType::MINUS_LESS)) {
        advance();
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>("-<", std::move(operand), false);
    }
    if (check(TokenType::AWAIT)) {
        advance();
        auto operand = parseUnary();
        return std::make_unique<UnaryExpr>("await", std::move(operand), false);
    }

    return parsePostfix(parsePrimary());
}

ExprPtr Parser::parsePostfix(ExprPtr left) {
    while (true) {
        if (check(TokenType::LPAREN)) {
            left = parseCallArgs(std::move(left));
        } else if (check(TokenType::LBRACKET)) {
            advance();
            auto idx = parseExpr();
            expect(TokenType::RBRACKET);
            left = std::make_unique<IndexExpr>(std::move(left), std::move(idx));
        } else if (check(TokenType::DOT)) {
            advance();
            std::string mem = expect(TokenType::IDENTIFIER).value;
            left = std::make_unique<MemberExpr>(std::move(left), mem, false);
        } else if (check(TokenType::ARROW)) {
            advance();
            std::string mem = expect(TokenType::IDENTIFIER).value;
            left = std::make_unique<MemberExpr>(std::move(left), mem, true);
        } else if (check(TokenType::PLUS_ARROW)) {
            advance();
            left = std::make_unique<UnaryExpr>("+>", std::move(left), true);
        } else if (check(TokenType::MINUS_LESS)) {
            advance();
            left = std::make_unique<UnaryExpr>("-<", std::move(left), true);
        } else break;
    }
    return left;
}

ExprPtr Parser::parsePrimary() {
    int ln = current().line;

    if (check(TokenType::INTEGER)) {
        long long v = std::stoll(current().value, nullptr, 0);
        auto e = std::make_unique<IntLiteral>(v); e->line = ln; advance(); return e;
    }
    if (check(TokenType::FLOAT)) {
        double v = std::stod(current().value);
        auto e = std::make_unique<FloatLiteral>(v); e->line = ln; advance(); return e;
    }
    if (check(TokenType::STRING)) {
        auto e = std::make_unique<StringLiteral>(current().value); e->line = ln; advance(); return e;
    }
    if (check(TokenType::CHAR)) {
        auto e = std::make_unique<CharLiteral>(current().value[0]); e->line = ln; advance(); return e;
    }
    if (check(TokenType::BOOL_TRUE))  { advance(); return std::make_unique<BoolLiteral>(true); }
    if (check(TokenType::BOOL_FALSE)) { advance(); return std::make_unique<BoolLiteral>(false); }
    if (check(TokenType::NULL_KW))    { advance(); return std::make_unique<NullLiteral>(); }

    if (check(TokenType::IDENTIFIER)) {
        // Handle Namespace::Member or Enum::Variant scope resolution
        std::string name = current().value; advance();
        while (check(TokenType::DOUBLE_COLON)) {
            advance();
            // next could be identifier
            if (check(TokenType::IDENTIFIER)) {
                name += "::" + current().value; advance();
            } else {
                // Unexpected token after ::
                error("Expected identifier after '::'");
            }
        }
        auto e = std::make_unique<Identifier>(name); e->line = ln; return e;
    }
    if (check(TokenType::THIS)) {
        auto e = std::make_unique<Identifier>("this"); e->line = ln; advance(); return e;
    }

    if (check(TokenType::LPAREN)) {
        advance();
        auto e = parseExpr();
        expect(TokenType::RPAREN);
        return e;
    }
    if (check(TokenType::NEW))    return parseNew();
    if (check(TokenType::SIZEOF)) return parseSizeof();
    if (check(TokenType::LAMBDA)) return parseLambda();

    error("Unexpected token '" + current().value + "' in expression");
    return nullptr; // unreachable
}

ExprPtr Parser::parseCallArgs(ExprPtr callee) {
    expect(TokenType::LPAREN);
    std::vector<ExprPtr> args;
    if (!check(TokenType::RPAREN)) {
        args.push_back(parseExpr());
        while (match(TokenType::COMMA)) args.push_back(parseExpr());
    }
    expect(TokenType::RPAREN);
    return std::make_unique<CallExpr>(std::move(callee), std::move(args));
}

ExprPtr Parser::parseNew() {
    expect(TokenType::NEW);
    auto ne = std::make_unique<NewExpr>();
    ne->type = parseType();
    if (check(TokenType::LPAREN)) {
        advance();
        if (!check(TokenType::RPAREN)) {
            ne->args.push_back(parseExpr());
            while (match(TokenType::COMMA)) ne->args.push_back(parseExpr());
        }
        expect(TokenType::RPAREN);
    }
    return ne;
}

ExprPtr Parser::parseSizeof() {
    expect(TokenType::SIZEOF);
    expect(TokenType::LPAREN);
    auto se = std::make_unique<SizeofExpr>();
    // Try to parse as type first
    size_t saved = pos;
    try {
        se->ofType = parseType();
        if (!check(TokenType::RPAREN)) throw std::runtime_error("not a type");
    } catch (...) {
        pos = saved;
        se->ofExpr = parseExpr();
    }
    expect(TokenType::RPAREN);
    return se;
}

ExprPtr Parser::parseLambda() {
    expect(TokenType::LAMBDA);
    auto le = std::make_unique<LambdaExpr>();
    expect(TokenType::LPAREN);
    // params
    if (!check(TokenType::RPAREN)) {
        LambdaExpr::Param p;
        p.type = parseType();
        if (check(TokenType::IDENTIFIER)) { p.name = current().value; advance(); }
        le->params.push_back(std::move(p));
        while (match(TokenType::COMMA)) {
            LambdaExpr::Param pp;
            pp.type = parseType();
            if (check(TokenType::IDENTIFIER)) { pp.name = current().value; advance(); }
            le->params.push_back(std::move(pp));
        }
    }
    expect(TokenType::RPAREN);
    if (match(TokenType::ARROW)) le->returnType = parseType();
    le->body = parseBlock();
    return le;
}

