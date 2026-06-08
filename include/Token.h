#pragma once
#include <string>
#include <unordered_map>

enum class TokenType {
    // Literals
    INTEGER, FLOAT, STRING, CHAR, BOOL_TRUE, BOOL_FALSE,

    // Identifiers & Keywords
    IDENTIFIER,
    RIVEN, CORE,         // entry point: riven core { }
    FIRM,                // constant: firm
    IMPRINT,             // print: Imprint(...)
    SCANQ,               // input: scanq
    IF, ALTIF, ELSE,     // conditionals
    WHILE, FOR, DO,
    RETURN_FIN,          // return fin
    CONSISTOF,           // import: consistof "file.rh"
    BREAK, CONTINUE,
    VOID, INT, FLOAT_T, CHAR_T, BOOL_T, STRING_T,
    STRUCT, ENUM, UNION,
    UNSAFE, ASYNC, AWAIT,
    NEW, DELETE_KW,
    NULL_KW, THIS,
    MATCH, CASE, DEFAULT,
    NAMESPACE, USING,
    TEMPLATE, TYPENAME,
    INLINE, EXTERN, STATIC,
    PUB, PRIV, PROT,     // access modifiers
    TRY, CATCH, THROW,
    TRAIT, IMPL,
    CLASS,
    SIZEOF, TYPEOF,
    LAMBDA,
    LET, MUT,

    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    PLUS_ARROW,          // +> increment
    MINUS_LESS,          // -< decrement
    ASSIGN,              // =
    EQ, NEQ,             // == !=
    LT, GT, LTE, GTE,   // < > <= >=
    AND, OR, NOT,        // && || !
    BITAND, BITOR, BITXOR, BITNOT,   // & | ^ ~
    LSHIFT, RSHIFT,      // << >>
    PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN,
    ARROW,               // ->
    DOUBLE_ARROW,        // =>
    DOT, DOUBLE_COLON,   // . ::
    QUESTION, COLON,     // ? :
    ELLIPSIS,            // ...

    // Delimiters
    LPAREN, RPAREN,
    LBRACE, RBRACE,
    LBRACKET, RBRACKET,
    SEMICOLON, COMMA,
    HASH,

    // Special
    EOF_TOKEN,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int col;

    Token(TokenType t, std::string v, int l, int c)
        : type(t), value(std::move(v)), line(l), col(c) {}

    std::string typeToString() const;
};

static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"riven",      TokenType::RIVEN},
    {"core",       TokenType::CORE},
    {"firm",       TokenType::FIRM},
    {"Imprint",    TokenType::IMPRINT},
    {"scanq",      TokenType::SCANQ},
    {"if",         TokenType::IF},
    {"altif",      TokenType::ALTIF},
    {"else",       TokenType::ELSE},
    {"while",      TokenType::WHILE},
    {"for",        TokenType::FOR},
    {"do",         TokenType::DO},
    {"return",     TokenType::RETURN_FIN},
    {"fin",        TokenType::RETURN_FIN},
    {"consistof",  TokenType::CONSISTOF},
    {"break",      TokenType::BREAK},
    {"continue",   TokenType::CONTINUE},
    {"void",       TokenType::VOID},
    {"int",        TokenType::INT},
    {"float",      TokenType::FLOAT_T},
    {"char",       TokenType::CHAR_T},
    {"bool",       TokenType::BOOL_T},
    {"string",     TokenType::STRING_T},
    {"struct",     TokenType::STRUCT},
    {"enum",       TokenType::ENUM},
    {"union",      TokenType::UNION},
    {"unsafe",     TokenType::UNSAFE},
    {"async",      TokenType::ASYNC},
    {"await",      TokenType::AWAIT},
    {"new",        TokenType::NEW},
    {"delete",     TokenType::DELETE_KW},
    {"null",       TokenType::NULL_KW},
    {"this",       TokenType::THIS},
    {"match",      TokenType::MATCH},
    {"case",       TokenType::CASE},
    {"default",    TokenType::DEFAULT},
    {"namespace",  TokenType::NAMESPACE},
    {"using",      TokenType::USING},
    {"template",   TokenType::TEMPLATE},
    {"typename",   TokenType::TYPENAME},
    {"inline",     TokenType::INLINE},
    {"extern",     TokenType::EXTERN},
    {"static",     TokenType::STATIC},
    {"pub",        TokenType::PUB},
    {"priv",       TokenType::PRIV},
    {"prot",       TokenType::PROT},
    {"try",        TokenType::TRY},
    {"catch",      TokenType::CATCH},
    {"throw",      TokenType::THROW},
    {"trait",      TokenType::TRAIT},
    {"impl",       TokenType::IMPL},
    {"class",      TokenType::CLASS},
    {"sizeof",     TokenType::SIZEOF},
    {"typeof",     TokenType::TYPEOF},
    {"lambda",     TokenType::LAMBDA},
    {"let",        TokenType::LET},
    {"mut",        TokenType::MUT},
    {"true",       TokenType::BOOL_TRUE},
    {"false",      TokenType::BOOL_FALSE},
};

