#pragma once
#include "Token.h"
#include <string>
#include <vector>
#include <stdexcept>

class LexerError : public std::runtime_error {
public:
    int line, col;
    LexerError(const std::string& msg, int l, int c)
        : std::runtime_error(msg), line(l), col(c) {}
};

class Lexer {
public:
    explicit Lexer(const std::string& source, const std::string& filename = "<stdin>");
    std::vector<Token> tokenize();

private:
    std::string src;
    std::string filename;
    size_t pos;
    int line, col;

    char current() const;
    char peek(int offset = 1) const;
    char advance();
    bool isEOF() const;

    void skipWhitespace();
    void skipLineComment();   // ~~ comment
    void skipBlockComment();  // << comment >>

    Token readString();
    Token readChar();
    Token readNumber();
    Token readIdentifierOrKeyword();
    Token readOperatorOrPunct();

    void error(const std::string& msg);
};

