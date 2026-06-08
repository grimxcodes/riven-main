#include "SemanticAnalyzer.h"
#include <iostream>
#include <stdexcept>

// ── Scope ──────────────────────────────────────────────────────────────────

bool Scope::declare(const Symbol& sym) {
    if (table.count(sym.name)) return false;
    table[sym.name] = sym;
    return true;
}

Symbol* Scope::lookup(const std::string& name) {
    auto it = table.find(name);
    if (it != table.end()) return &it->second;
    if (parent) return parent->lookup(name);
    return nullptr;
}

// ── SemanticAnalyzer ───────────────────────────────────────────────────────

SemanticAnalyzer::SemanticAnalyzer() {
    scopes.push_back(std::make_unique<Scope>(nullptr));
    currentScope = scopes.back().get();
    declareBuiltins();
}

void SemanticAnalyzer::declareBuiltins() {
    // Built-in functions
    for (auto& name : {"exit", "abs", "malloc", "free", "realloc",
                        "memcpy", "memset", "strlen", "strcmp", "strcpy",
                        "sprintf", "printf", "scanf"}) {
        Symbol s; s.name = name; s.kind = SymKind::Function; s.typeName = "void";
        currentScope->declare(s);
    }
}

void SemanticAnalyzer::pushScope() {
    scopes.push_back(std::make_unique<Scope>(currentScope));
    currentScope = scopes.back().get();
}

void SemanticAnalyzer::popScope() {
    scopes.pop_back();
    currentScope = scopes.empty() ? nullptr : scopes.back().get();
}

// ── Entry point ────────────────────────────────────────────────────────────

void SemanticAnalyzer::analyze(Program& prog) {
    // First pass: collect global declarations
    for (auto& d : prog.decls) {
        if (auto* fd = dynamic_cast<FuncDecl*>(d.get())) {
            Symbol s; s.name = fd->name; s.kind = SymKind::Function;
            s.typeName = fd->returnType ? fd->returnType->name : "void";
            s.declLine = fd->line;
            currentScope->declare(s);
        } else if (auto* gv = dynamic_cast<GlobalVarDecl*>(d.get())) {
            Symbol s; s.name = gv->name; s.kind = SymKind::Variable;
            s.typeName = gv->type ? gv->type->name : "auto";
            s.isFirm = gv->isFirm; s.declLine = gv->line;
            if (!currentScope->declare(s)) {
                std::cerr << "[Warning] Redeclaration of global '" << gv->name << "'\n";
            }
        } else if (auto* sd = dynamic_cast<StructDecl*>(d.get())) {
            Symbol s; s.name = sd->name; s.kind = SymKind::Type; s.typeName = sd->name;
            currentScope->declare(s);
        } else if (auto* cd = dynamic_cast<ClassDecl*>(d.get())) {
            Symbol s; s.name = cd->name; s.kind = SymKind::Type; s.typeName = cd->name;
            currentScope->declare(s);
        } else if (auto* ed = dynamic_cast<EnumDecl*>(d.get())) {
            Symbol s; s.name = ed->name; s.kind = SymKind::Type; s.typeName = ed->name;
            currentScope->declare(s);
            // Declare enumerators
            for (auto& en : ed->enumerators) {
                Symbol es; es.name = en.name; es.kind = SymKind::Constant;
                es.typeName = ed->name; es.isFirm = true;
                currentScope->declare(es);
            }
        }
    }

    // Second pass: full analysis
    for (auto& d : prog.decls) analyzeDecl(*d);
    if (prog.coreEntry) analyzeRivenCore(*prog.coreEntry);
}

// ── Declarations ──────────────────────────────────────────────────────────

void SemanticAnalyzer::analyzeDecl(Decl& d) {
    if (auto* fd = dynamic_cast<FuncDecl*>(&d))        analyzeFuncDecl(*fd);
    else if (auto* sd = dynamic_cast<StructDecl*>(&d)) analyzeStructDecl(*sd);
    else if (auto* cd = dynamic_cast<ClassDecl*>(&d))  analyzeClassDecl(*cd);
    else if (auto* ed = dynamic_cast<EnumDecl*>(&d))   analyzeEnumDecl(*ed);
    else if (auto* nd = dynamic_cast<NamespaceDecl*>(&d)) analyzeNamespaceDecl(*nd);
    else if (auto* gv = dynamic_cast<GlobalVarDecl*>(&d)) analyzeGlobalVar(*gv);
    else if (auto* td = dynamic_cast<TraitDecl*>(&d))  analyzeTraitDecl(*td);
    else if (auto* id = dynamic_cast<ImplDecl*>(&d))   analyzeImplDecl(*id);
    // ImportDecl: no semantic analysis needed here
}

void SemanticAnalyzer::analyzeFuncDecl(FuncDecl& f) {
    pushScope();
    std::string savedReturn = currentFuncReturn;
    currentFuncReturn = f.returnType ? f.returnType->name : "void";

    for (auto& p : f.params) {
        Symbol s; s.name = p.name; s.kind = SymKind::Param;
        s.typeName = p.type ? p.type->name : "auto";
        s.declLine = f.line;
        if (!p.name.empty()) currentScope->declare(s);
    }
    if (f.body) analyzeBlock(*f.body);

    currentFuncReturn = savedReturn;
    popScope();
}

void SemanticAnalyzer::analyzeStructDecl(StructDecl& s) {
    pushScope();
    for (auto& field : s.fields) {
        Symbol sym; sym.name = field.name; sym.kind = SymKind::Variable;
        sym.typeName = field.type ? field.type->name : "auto";
        currentScope->declare(sym);
        if (field.defaultVal) analyzeExpr(*field.defaultVal);
    }
    for (auto& meth : s.methods) analyzeFuncDecl(*meth);
    popScope();
}

void SemanticAnalyzer::analyzeClassDecl(ClassDecl& c) {
    pushScope();
    // 'this' pointer
    Symbol thisS; thisS.name = "this"; thisS.kind = SymKind::Variable;
    thisS.typeName = c.name + "*";
    currentScope->declare(thisS);
    for (auto& mem : c.members) {
        Symbol sym; sym.name = mem.name; sym.kind = SymKind::Variable;
        sym.typeName = mem.type ? mem.type->name : "auto";
        currentScope->declare(sym);
        if (mem.defaultVal) analyzeExpr(*mem.defaultVal);
    }
    for (auto& meth : c.methods) analyzeFuncDecl(*meth);
    popScope();
}

void SemanticAnalyzer::analyzeEnumDecl(EnumDecl& e) {
    for (auto& en : e.enumerators)
        if (en.value) analyzeExpr(*en.value);
}

void SemanticAnalyzer::analyzeNamespaceDecl(NamespaceDecl& n) {
    pushScope();
    for (auto& d : n.body) analyzeDecl(*d);
    popScope();
}

void SemanticAnalyzer::analyzeGlobalVar(GlobalVarDecl& g) {
    if (g.init) analyzeExpr(*g.init);
}

void SemanticAnalyzer::analyzeTraitDecl(TraitDecl& t) {
    for (auto& m : t.methods) analyzeFuncDecl(*m);
}

void SemanticAnalyzer::analyzeImplDecl(ImplDecl& i) {
    for (auto& m : i.methods) analyzeFuncDecl(*m);
}

void SemanticAnalyzer::analyzeRivenCore(RivenCoreEntry& entry) {
    pushScope();
    std::string saved = currentFuncReturn;
    currentFuncReturn = "int";
    analyzeBlock(*entry.body);
    currentFuncReturn = saved;
    popScope();
}

// ── Statements ────────────────────────────────────────────────────────────

void SemanticAnalyzer::analyzeStmt(Stmt& s) {
    if (auto* bs = dynamic_cast<BlockStmt*>(&s))        analyzeBlock(*bs);
    else if (auto* vs = dynamic_cast<VarDecl*>(&s))     analyzeVarDecl(*vs);
    else if (auto* is = dynamic_cast<IfStmt*>(&s))      analyzeIf(*is);
    else if (auto* ws = dynamic_cast<WhileStmt*>(&s))   analyzeWhile(*ws);
    else if (auto* dw = dynamic_cast<DoWhileStmt*>(&s)) analyzeDoWhile(*dw);
    else if (auto* fs = dynamic_cast<ForStmt*>(&s))     analyzeFor(*fs);
    else if (auto* rs = dynamic_cast<ReturnStmt*>(&s))  analyzeReturn(*rs);
    else if (auto* ip = dynamic_cast<ImprintStmt*>(&s)) analyzeImprint(*ip);
    else if (auto* sq = dynamic_cast<ScanqStmt*>(&s))   analyzeScanq(*sq);
    else if (auto* ms = dynamic_cast<MatchStmt*>(&s))   analyzeMatch(*ms);
    else if (auto* ts = dynamic_cast<TryStmt*>(&s))     analyzeTry(*ts);
    else if (auto* ub = dynamic_cast<UnsafeBlock*>(&s)) analyzeUnsafe(*ub);
    else if (auto* ab = dynamic_cast<AsyncBlock*>(&s))  analyzeAsync(*ab);
    else if (auto* es = dynamic_cast<ExprStmt*>(&s))    analyzeExpr(*es->expr);
    else if (auto* th = dynamic_cast<ThrowStmt*>(&s))   analyzeExpr(*th->expr);
    else if (auto* ds = dynamic_cast<DeleteStmt*>(&s))  analyzeExpr(*ds->ptr);
    // BreakStmt, ContinueStmt: just check we're in a loop
}

void SemanticAnalyzer::analyzeBlock(BlockStmt& b) {
    pushScope();
    for (auto& st : b.stmts) analyzeStmt(*st);
    popScope();
}

void SemanticAnalyzer::analyzeVarDecl(VarDecl& v) {
    if (v.init) analyzeExpr(*v.init);
    Symbol s; s.name = v.name; s.kind = SymKind::Variable;
    s.typeName = v.type ? v.type->name : "auto";
    s.isFirm = v.isFirm; s.isMut = v.isMut || !v.isFirm;
    s.declLine = v.line;
    if (!currentScope->declare(s)) {
        std::cerr << "[Warning line " << v.line << "] Redeclaration of '" << v.name << "'\n";
    }
}

void SemanticAnalyzer::analyzeIf(IfStmt& i) {
    analyzeExpr(*i.cond);
    analyzeStmt(*i.thenBranch);
    for (auto& alt : i.altifs) { analyzeExpr(*alt.cond); analyzeStmt(*alt.body); }
    if (i.elseBranch) analyzeStmt(*i.elseBranch);
}

void SemanticAnalyzer::analyzeWhile(WhileStmt& w) {
    analyzeExpr(*w.cond);
    bool saved = inLoop; inLoop = true;
    analyzeStmt(*w.body);
    inLoop = saved;
}

void SemanticAnalyzer::analyzeDoWhile(DoWhileStmt& d) {
    bool saved = inLoop; inLoop = true;
    analyzeStmt(*d.body);
    inLoop = saved;
    analyzeExpr(*d.cond);
}

void SemanticAnalyzer::analyzeFor(ForStmt& f) {
    pushScope();
    if (f.init) analyzeStmt(*f.init);
    if (f.cond) analyzeExpr(*f.cond);
    if (f.update) analyzeExpr(*f.update);
    bool saved = inLoop; inLoop = true;
    analyzeStmt(*f.body);
    inLoop = saved;
    popScope();
}

void SemanticAnalyzer::analyzeReturn(ReturnStmt& r) {
    if (r.value) analyzeExpr(*r.value);
}

void SemanticAnalyzer::analyzeImprint(ImprintStmt& i) {
    for (auto& arg : i.args) analyzeExpr(*arg);
}

void SemanticAnalyzer::analyzeScanq(ScanqStmt& s) {
    if (s.target) analyzeExpr(*s.target);
}

void SemanticAnalyzer::analyzeMatch(MatchStmt& m) {
    analyzeExpr(*m.expr);
    for (auto& c : m.cases) {
        if (c.pattern) analyzeExpr(*c.pattern);
        analyzeStmt(*c.body);
    }
}

void SemanticAnalyzer::analyzeTry(TryStmt& t) {
    analyzeBlock(*t.tryBlock);
    for (auto& cc : t.catches) {
        pushScope();
        if (!cc.name.empty()) {
            Symbol s; s.name = cc.name; s.kind = SymKind::Variable;
            s.typeName = cc.type ? cc.type->name : "auto";
            currentScope->declare(s);
        }
        analyzeBlock(*cc.body);
        popScope();
    }
}

void SemanticAnalyzer::analyzeUnsafe(UnsafeBlock& u) {
    bool saved = inUnsafe; inUnsafe = true;
    analyzeBlock(*u.body);
    inUnsafe = saved;
}

void SemanticAnalyzer::analyzeAsync(AsyncBlock& a) {
    bool saved = inAsync; inAsync = true;
    analyzeBlock(*a.body);
    inAsync = saved;
}

// ── Expressions ───────────────────────────────────────────────────────────

std::string SemanticAnalyzer::analyzeExpr(Expr& e) {
    if (auto* il = dynamic_cast<IntLiteral*>(&e))    return "int";
    if (auto* fl = dynamic_cast<FloatLiteral*>(&e))  return "float";
    if (auto* sl = dynamic_cast<StringLiteral*>(&e)) return "string";
    if (auto* cl = dynamic_cast<CharLiteral*>(&e))   return "char";
    if (auto* bl = dynamic_cast<BoolLiteral*>(&e))   return "bool";
    if (dynamic_cast<NullLiteral*>(&e))               return "null";

    if (auto* id = dynamic_cast<Identifier*>(&e)) {
        Symbol* sym = currentScope->lookup(id->name);
        if (!sym) {
            std::cerr << "[Warning line " << e.line << "] Undeclared identifier '" << id->name << "'\n";
            return "auto";
        }
        return sym->typeName;
    }
    if (auto* be = dynamic_cast<BinaryExpr*>(&e))   return analyzeBinary(*be);
    if (auto* ue = dynamic_cast<UnaryExpr*>(&e))    return analyzeUnary(*ue);
    if (auto* ae = dynamic_cast<AssignExpr*>(&e))   return analyzeAssign(*ae);
    if (auto* ce = dynamic_cast<CallExpr*>(&e))     return analyzeCall(*ce);
    if (auto* ie = dynamic_cast<IndexExpr*>(&e))    return analyzeIndex(*ie);
    if (auto* me = dynamic_cast<MemberExpr*>(&e))   return analyzeMember(*me);
    if (auto* te = dynamic_cast<TernaryExpr*>(&e))  return analyzeTernary(*te);
    if (auto* ne = dynamic_cast<NewExpr*>(&e)) {
        for (auto& a : ne->args) analyzeExpr(*a);
        return ne->type ? ne->type->name + "*" : "void*";
    }
    if (auto* ce = dynamic_cast<CastExpr*>(&e)) {
        analyzeExpr(*ce->expr);
        return ce->targetType ? ce->targetType->name : "auto";
    }
    return "auto";
}

std::string SemanticAnalyzer::analyzeBinary(BinaryExpr& b) {
    std::string lt = analyzeExpr(*b.left);
    std::string rt = analyzeExpr(*b.right);
    if (b.op == "==" || b.op == "!=" || b.op == "<" || b.op == ">" ||
        b.op == "<=" || b.op == ">=" || b.op == "&&" || b.op == "||") return "bool";
    if (isNumericType(lt) && isNumericType(rt)) {
        if (lt == "float" || rt == "float") return "float";
        return "int";
    }
    if (b.op == "+" && (lt == "string" || rt == "string")) return "string";
    return lt;
}

std::string SemanticAnalyzer::analyzeUnary(UnaryExpr& u) {
    std::string t = analyzeExpr(*u.operand);
    if (u.op == "!") return "bool";
    if (u.op == "+>" || u.op == "-<" || u.op == "++" || u.op == "--") return t;
    if (u.op == "*") {
        if (t.size() > 0 && t.back() == '*') return t.substr(0, t.size()-1);
        return t;
    }
    return t;
}

std::string SemanticAnalyzer::analyzeAssign(AssignExpr& a) {
    analyzeExpr(*a.target);
    return analyzeExpr(*a.value);
}

std::string SemanticAnalyzer::analyzeCall(CallExpr& c) {
    analyzeExpr(*c.callee);
    for (auto& arg : c.args) analyzeExpr(*arg);
    return "auto";
}

std::string SemanticAnalyzer::analyzeIndex(IndexExpr& i) {
    analyzeExpr(*i.object);
    analyzeExpr(*i.index);
    return "auto";
}

std::string SemanticAnalyzer::analyzeMember(MemberExpr& m) {
    analyzeExpr(*m.object);
    return "auto";
}

std::string SemanticAnalyzer::analyzeTernary(TernaryExpr& t) {
    analyzeExpr(*t.cond);
    std::string tt = analyzeExpr(*t.thenExpr);
    analyzeExpr(*t.elseExpr);
    return tt;
}

// ── Type helpers ──────────────────────────────────────────────────────────

bool SemanticAnalyzer::isNumericType(const std::string& t) const {
    return t == "int" || t == "float" || t == "char" || t == "bool";
}
bool SemanticAnalyzer::typesCompatible(const std::string& a, const std::string& b) const {
    if (a == b) return true;
    if (isNumericType(a) && isNumericType(b)) return true;
    if (a == "auto" || b == "auto") return true;
    return false;
}

