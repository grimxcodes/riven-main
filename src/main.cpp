#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <filesystem>

#include "Lexer.h"
#include "Parser.h"
#include "SemanticAnalyzer.h"
#include "CodeGen.h"

// ── ANSI colors ────────────────────────────────────────────────────────────
#define CLR_RED     "\033[1;31m"
#define CLR_GREEN   "\033[1;32m"
#define CLR_YELLOW  "\033[1;33m"
#define CLR_CYAN    "\033[1;36m"
#define CLR_BOLD    "\033[1m"
#define CLR_RESET   "\033[0m"

static void banner() {
    std::cout << CLR_CYAN <<
R"(
 ██████╗ ██╗██╗   ██╗███████╗███╗   ██╗
 ██╔══██╗██║██║   ██║██╔════╝████╗  ██║
 ██████╔╝██║██║   ██║█████╗  ██╔██╗ ██║
 ██╔══██╗██║╚██╗ ██╔╝██╔══╝  ██║╚██╗██║
 ██║  ██║██║ ╚████╔╝ ███████╗██║ ╚████║
 ╚═╝  ╚═╝╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝
)" << CLR_RESET;
    std::cout << CLR_BOLD << "  Riven Compiler v1.0  |  rivc" << CLR_RESET
              << "  |  Riven → C++\n\n";
}

static void usage(const char* prog) {
    std::cerr << CLR_BOLD << "Usage:" << CLR_RESET << "\n";
    std::cerr << "  " << prog << " [options] <input.rn>\n\n";
    std::cerr << CLR_BOLD << "Options:" << CLR_RESET << "\n";
    std::cerr << "  -o <file>        Output file (default: a.out or out.cpp with --emit-cpp)\n";
    std::cerr << "  --emit-cpp       Emit C++ source instead of binary\n";
    std::cerr << "  --emit-tokens    Dump lexer token stream\n";
    std::cerr << "  --no-semantic    Skip semantic analysis\n";
    std::cerr << "  --cc <compiler>  C++ compiler to use (default: g++)\n";
    std::cerr << "  --cflags <flags> Extra C++ compiler flags\n";
    std::cerr << "  -v, --verbose    Verbose output\n";
    std::cerr << "  -h, --help       Show this help\n";
    std::cerr << "  --version        Show version\n";
    std::cerr << "\n";
    std::cerr << CLR_BOLD << "Examples:" << CLR_RESET << "\n";
    std::cerr << "  " << prog << " hello.rn               # Compile to binary\n";
    std::cerr << "  " << prog << " hello.rn -o hello       # Named binary\n";
    std::cerr << "  " << prog << " hello.rn --emit-cpp     # Show generated C++\n";
    std::cerr << "  " << prog << " hello.rn --emit-tokens  # Dump tokens\n";
}

static std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) throw std::runtime_error("Cannot open file: " + path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string tokenTypeName(TokenType t) {
    switch (t) {
#define CASE(x) case TokenType::x: return #x;
        CASE(INTEGER) CASE(FLOAT) CASE(STRING) CASE(CHAR)
        CASE(IDENTIFIER) CASE(RIVEN) CASE(CORE) CASE(FIRM)
        CASE(IMPRINT) CASE(SCANQ) CASE(IF) CASE(ALTIF) CASE(ELSE)
        CASE(WHILE) CASE(FOR) CASE(DO) CASE(RETURN_FIN)
        CASE(CONSISTOF) CASE(BREAK) CASE(CONTINUE) CASE(VOID)
        CASE(INT) CASE(FLOAT_T) CASE(CHAR_T) CASE(BOOL_T) CASE(STRING_T)
        CASE(STRUCT) CASE(CLASS) CASE(ENUM) CASE(NAMESPACE)
        CASE(PLUS) CASE(MINUS) CASE(STAR) CASE(SLASH) CASE(PERCENT)
        CASE(PLUS_ARROW) CASE(MINUS_LESS) CASE(ASSIGN) CASE(EQ) CASE(NEQ)
        CASE(LT) CASE(GT) CASE(LTE) CASE(GTE) CASE(AND) CASE(OR) CASE(NOT)
        CASE(LPAREN) CASE(RPAREN) CASE(LBRACE) CASE(RBRACE)
        CASE(LBRACKET) CASE(RBRACKET) CASE(SEMICOLON) CASE(COMMA)
        CASE(EOF_TOKEN) CASE(BOOL_TRUE) CASE(BOOL_FALSE) CASE(NULL_KW)
        CASE(ARROW) CASE(DOT) CASE(DOUBLE_COLON)
        CASE(UNSAFE) CASE(ASYNC) CASE(AWAIT) CASE(NEW) CASE(DELETE_KW)
        CASE(MATCH) CASE(CASE) CASE(DEFAULT) CASE(TRY) CASE(CATCH) CASE(THROW)
        CASE(TRAIT) CASE(IMPL) CASE(STATIC) CASE(EXTERN) CASE(INLINE)
        CASE(PUB) CASE(PRIV) CASE(PROT) CASE(LET) CASE(MUT)
        CASE(SIZEOF) CASE(TYPEOF) CASE(LAMBDA)
#undef CASE
        default: return "UNKNOWN";
    }
}

int main(int argc, char* argv[]) {
    // ── Parse CLI ──────────────────────────────────────────────────────────
    std::string inputFile;
    std::string outputFile;
    std::string cppCompiler = "g++";
    std::string extraFlags  = "-std=c++17 -O2";
    bool emitCpp      = false;
    bool emitTokens   = false;
    bool noSemantic   = false;
    bool verbose      = false;

    if (argc < 2) { banner(); usage(argv[0]); return 1; }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help")    { banner(); usage(argv[0]); return 0; }
        if (arg == "--version")                { std::cout << "rivc 1.0.0\n"; return 0; }
        if (arg == "--emit-cpp")               { emitCpp = true; continue; }
        if (arg == "--emit-tokens")            { emitTokens = true; continue; }
        if (arg == "--no-semantic")            { noSemantic = true; continue; }
        if (arg == "-v" || arg == "--verbose") { verbose = true; continue; }
        if (arg == "-o" && i + 1 < argc)      { outputFile = argv[++i]; continue; }
        if (arg == "--cc" && i + 1 < argc)    { cppCompiler = argv[++i]; continue; }
        if (arg == "--cflags" && i + 1 < argc){ extraFlags = argv[++i]; continue; }
        if (arg[0] != '-')                     { inputFile = arg; continue; }
        std::cerr << CLR_RED << "Unknown option: " << arg << CLR_RESET << "\n";
        return 1;
    }

    if (inputFile.empty()) {
        std::cerr << CLR_RED << "Error: No input file specified.\n" << CLR_RESET;
        usage(argv[0]);
        return 1;
    }

    // Validate extension
    auto ext = std::filesystem::path(inputFile).extension().string();
    if (ext != ".rn" && ext != ".rh" && ext != ".rvh") {
        std::cerr << CLR_YELLOW << "[Warning] Input file extension '" << ext
                  << "' is unusual for Riven. Expected .rn\n" << CLR_RESET;
    }

    banner();

    // ── Read source ────────────────────────────────────────────────────────
    std::string source;
    try {
        source = readFile(inputFile);
    } catch (const std::exception& e) {
        std::cerr << CLR_RED << "Error: " << e.what() << CLR_RESET << "\n";
        return 1;
    }

    if (verbose) std::cout << CLR_CYAN << "[1/4] Lexing " << inputFile << "...\n" << CLR_RESET;

    // ── Lex ────────────────────────────────────────────────────────────────
    std::vector<Token> tokens;
    try {
        Lexer lexer(source, inputFile);
        tokens = lexer.tokenize();
    } catch (const LexerError& e) {
        std::cerr << CLR_RED << "Lexer Error: " << e.what() << CLR_RESET << "\n";
        return 1;
    }

    if (emitTokens) {
        std::cout << CLR_BOLD << "=== Token Stream ===\n" << CLR_RESET;
        for (auto& t : tokens) {
            std::cout << "[" << t.line << ":" << t.col << "] "
                      << CLR_YELLOW << tokenTypeName(t.type) << CLR_RESET
                      << " = '" << t.value << "'\n";
        }
        return 0;
    }

    if (verbose) std::cout << CLR_CYAN << "    Lexed " << tokens.size() << " tokens.\n" << CLR_RESET;

    // ── Parse ──────────────────────────────────────────────────────────────
    if (verbose) std::cout << CLR_CYAN << "[2/4] Parsing...\n" << CLR_RESET;
    Program prog;
    try {
        Parser parser(std::move(tokens));
        prog = parser.parse();
    } catch (const ParseError& e) {
        std::cerr << CLR_RED << "Parse Error: " << e.what() << CLR_RESET << "\n";
        return 1;
    }

    if (verbose) {
        std::cout << CLR_CYAN << "    Parsed " << prog.decls.size() << " top-level declarations.\n"
                  << CLR_RESET;
    }

    // ── Semantic Analysis ──────────────────────────────────────────────────
    if (!noSemantic) {
        if (verbose) std::cout << CLR_CYAN << "[3/4] Semantic analysis...\n" << CLR_RESET;
        try {
            SemanticAnalyzer sa;
            sa.analyze(prog);
        } catch (const SemanticError& e) {
            std::cerr << CLR_RED << "Semantic Error: " << e.what() << CLR_RESET << "\n";
            return 1;
        }
    }

    // ── Code Generation ────────────────────────────────────────────────────
    if (verbose) std::cout << CLR_CYAN << "[4/4] Generating C++ code...\n" << CLR_RESET;
    std::string cppCode;
    try {
        CodeGenerator cg;
        cppCode = cg.generate(prog);
    } catch (const std::exception& e) {
        std::cerr << CLR_RED << "Code Generation Error: " << e.what() << CLR_RESET << "\n";
        return 1;
    }

    // ── Output ─────────────────────────────────────────────────────────────
    if (emitCpp) {
        std::string cppOut = outputFile.empty() ? "out.cpp" : outputFile;
        std::ofstream f(cppOut);
        if (!f) {
            std::cerr << CLR_RED << "Error: Cannot write to " << cppOut << CLR_RESET << "\n";
            return 1;
        }
        f << cppCode;
        std::cout << CLR_GREEN << "✓ C++ source written to: " << cppOut << CLR_RESET << "\n";
        return 0;
    }

    // Write to temp .cpp file and compile
    std::string stem = std::filesystem::path(inputFile).stem().string();
    std::string tmpCpp = "/tmp/__riven_" + stem + ".cpp";
    std::string binOut = outputFile.empty() ? stem : outputFile;

    {
        std::ofstream f(tmpCpp);
        if (!f) {
            std::cerr << CLR_RED << "Error: Cannot write temp file " << tmpCpp << CLR_RESET << "\n";
            return 1;
        }
        f << cppCode;
    }

    std::string cmd = cppCompiler + " " + extraFlags + " -o " + binOut + " " + tmpCpp;
    if (verbose) std::cout << CLR_CYAN << "  C++ cmd: " << cmd << "\n" << CLR_RESET;

    int rc = std::system(cmd.c_str());
    // Cleanup temp file
    std::remove(tmpCpp.c_str());

    if (rc != 0) {
        std::cerr << CLR_RED << "Error: C++ compilation failed (exit " << rc << ")\n" << CLR_RESET;
        std::cerr << CLR_YELLOW << "Tip: Try --emit-cpp to inspect the generated source.\n" << CLR_RESET;
        return 1;
    }

    std::cout << CLR_GREEN << "✓ Compiled successfully → " << binOut << CLR_RESET << "\n";
    return 0;
}

