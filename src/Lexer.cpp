#include "Lexer.h"
#include <cctype>
#include <stdexcept>
#include <sstream>

// ── Token string helper ────────────────────────────────────────────────────
std::string Token::typeToString() const {
    switch (type) {
        case TokenType::INTEGER:     return "INTEGER";
        case TokenType::FLOAT:       return "FLOAT";
        case TokenType::STRING:      return "STRING";
        case TokenType::CHAR:        return "CHAR";
        case TokenType::IDENTIFIER:  return "IDENTIFIER(" + value + ")";
        case TokenType::EOF_TOKEN:   return "EOF";
        default:                     return "'" + value + "'";
    }
}

// ── Lexer ──────────────────────────────────────────────────────────────────

Lexer::Lexer(const std::string& source, const std::string& fn)
    : src(source), filename(fn), pos(0), line(1), col(1) {}

bool Lexer::isEOF() const { return pos >= src.size(); }

char Lexer::current() const {
    if (isEOF()) return '\0';
    return src[pos];
}

char Lexer::peek(int offset) const {
    size_t p = pos + offset;
    if (p >= src.size()) return '\0';
    return src[p];
}

char Lexer::advance() {
    char c = src[pos++];
    if (c == '\n') { ++line; col = 1; }
    else ++col;
    return c;
}

void Lexer::error(const std::string& msg) {
    throw LexerError("[" + filename + ":" + std::to_string(line) + ":" +
                     std::to_string(col) + "] Lexer error: " + msg, line, col);
}

// ── Whitespace & comments ──────────────────────────────────────────────────

void Lexer::skipWhitespace() {
    while (!isEOF() && std::isspace(current())) advance();
}

void Lexer::skipLineComment() {
    // ~~ comment until end of line
    while (!isEOF() && current() != '\n') advance();
}

void Lexer::skipBlockComment() {
    // << ... >> block comment
    // We enter right after the opening '<<', skip until '>>'
    int start_line = line, start_col = col;
    while (!isEOF()) {
        if (current() == '>' && peek(1) == '>') {
            advance(); advance(); // consume >>
            return;
        }
        advance();
    }
    throw LexerError("[" + filename + ":" + std::to_string(start_line) + ":" +
                     std::to_string(start_col) + "] Unterminated block comment", start_line, start_col);
}

// ── Readers ────────────────────────────────────────────────────────────────

Token Lexer::readString() {
    int sl = line, sc = col;
    char delim = advance(); // consume " or '
    std::string val;
    while (!isEOF() && current() != delim) {
        if (current() == '\\') {
            advance();
            switch (current()) {
                case 'n':  val += '\n'; break;
                case 't':  val += '\t'; break;
                case 'r':  val += '\r'; break;
                case '\\': val += '\\'; break;
                case '"':  val += '"';  break;
                case '\'': val += '\''; break;
                case '0':  val += '\0'; break;
                default:   val += current(); break;
            }
            advance();
        } else {
            val += advance();
        }
    }
    if (isEOF()) error("Unterminated string literal");
    advance(); // closing quote
    if (delim == '"') return Token(TokenType::STRING, val, sl, sc);
    if (val.size() != 1) error("Char literal must contain exactly one character");
    return Token(TokenType::CHAR, val, sl, sc);
}

Token Lexer::readNumber() {
    int sl = line, sc = col;
    std::string num;
    bool isFloat = false;

    if (current() == '0' && (peek() == 'x' || peek() == 'X')) {
        num += advance(); num += advance();
        while (!isEOF() && std::isxdigit(current())) num += advance();
        return Token(TokenType::INTEGER, num, sl, sc);
    }
    if (current() == '0' && (peek() == 'b' || peek() == 'B')) {
        num += advance(); num += advance();
        while (!isEOF() && (current() == '0' || current() == '1')) num += advance();
        return Token(TokenType::INTEGER, num, sl, sc);
    }

    while (!isEOF() && std::isdigit(current())) num += advance();
    if (!isEOF() && current() == '.' && std::isdigit(peek())) {
        isFloat = true;
        num += advance();
        while (!isEOF() && std::isdigit(current())) num += advance();
    }
    if (!isEOF() && (current() == 'e' || current() == 'E')) {
        isFloat = true;
        num += advance();
        if (!isEOF() && (current() == '+' || current() == '-')) num += advance();
        while (!isEOF() && std::isdigit(current())) num += advance();
    }
    // optional suffix f / u / l / ul
    while (!isEOF() && (current() == 'f' || current() == 'u' ||
                         current() == 'l' || current() == 'L')) {
        char s = current();
        if (s == 'f') isFloat = true;
        advance();
    }
    return Token(isFloat ? TokenType::FLOAT : TokenType::INTEGER, num, sl, sc);
}

Token Lexer::readIdentifierOrKeyword() {
    int sl = line, sc = col;
    std::string id;
    while (!isEOF() && (std::isalnum(current()) || current() == '_')) id += advance();

    auto it = KEYWORDS.find(id);
    if (it != KEYWORDS.end()) return Token(it->second, id, sl, sc);
    return Token(TokenType::IDENTIFIER, id, sl, sc);
}

Token Lexer::readOperatorOrPunct() {
    int sl = line, sc = col;
    char c = advance();

    // Two-char operators first
    char n = current();

    if (c == '+') {
        if (n == '>') { advance(); return Token(TokenType::PLUS_ARROW, "+>", sl, sc); }
        if (n == '=') { advance(); return Token(TokenType::PLUS_ASSIGN, "+=", sl, sc); }
        if (n == '+') { advance(); return Token(TokenType::PLUS_ARROW, "++", sl, sc); }
        return Token(TokenType::PLUS, "+", sl, sc);
    }
    if (c == '-') {
        if (n == '<') { advance(); return Token(TokenType::MINUS_LESS, "-<", sl, sc); }
        if (n == '=') { advance(); return Token(TokenType::MINUS_ASSIGN, "-=", sl, sc); }
        if (n == '>') { advance(); return Token(TokenType::ARROW, "->", sl, sc); }
        if (n == '-') { advance(); return Token(TokenType::MINUS_LESS, "--", sl, sc); }
        return Token(TokenType::MINUS, "-", sl, sc);
    }
    if (c == '*') {
        if (n == '=') { advance(); return Token(TokenType::STAR_ASSIGN, "*=", sl, sc); }
        return Token(TokenType::STAR, "*", sl, sc);
    }
    if (c == '/') {
        if (n == '=') { advance(); return Token(TokenType::SLASH_ASSIGN, "/=", sl, sc); }
        return Token(TokenType::SLASH, "/", sl, sc);
    }
    if (c == '%') return Token(TokenType::PERCENT, "%", sl, sc);
    if (c == '=') {
        if (n == '=') { advance(); return Token(TokenType::EQ, "==", sl, sc); }
        if (n == '>') { advance(); return Token(TokenType::DOUBLE_ARROW, "=>", sl, sc); }
        return Token(TokenType::ASSIGN, "=", sl, sc);
    }
    if (c == '!') {
        if (n == '=') { advance(); return Token(TokenType::NEQ, "!=", sl, sc); }
        return Token(TokenType::NOT, "!", sl, sc);
    }
    if (c == '<') {
        if (n == '=') { advance(); return Token(TokenType::LTE, "<=", sl, sc); }
        if (n == '<') { advance(); return Token(TokenType::LSHIFT, "<<", sl, sc); }
        return Token(TokenType::LT, "<", sl, sc);
    }
    if (c == '>') {
        if (n == '=') { advance(); return Token(TokenType::GTE, ">=", sl, sc); }
        if (n == '>') { advance(); return Token(TokenType::RSHIFT, ">>", sl, sc); }
        return Token(TokenType::GT, ">", sl, sc);
    }
    if (c == '&') {
        if (n == '&') { advance(); return Token(TokenType::AND, "&&", sl, sc); }
        return Token(TokenType::BITAND, "&", sl, sc);
    }
    if (c == '|') {
        if (n == '|') { advance(); return Token(TokenType::OR, "||", sl, sc); }
        return Token(TokenType::BITOR, "|", sl, sc);
    }
    if (c == '^') return Token(TokenType::BITXOR, "^", sl, sc);
    if (c == '~') {
        // ~~ = line comment
        if (n == '~') { advance(); skipLineComment(); return Token(TokenType::UNKNOWN, "", sl, sc); }
        return Token(TokenType::BITNOT, "~", sl, sc);
    }
    if (c == '.') {
        if (n == '.' && peek(1) == '.') { advance(); advance(); return Token(TokenType::ELLIPSIS, "...", sl, sc); }
        return Token(TokenType::DOT, ".", sl, sc);
    }
    if (c == ':') {
        if (n == ':') { advance(); return Token(TokenType::DOUBLE_COLON, "::", sl, sc); }
        return Token(TokenType::COLON, ":", sl, sc);
    }
    if (c == '?') return Token(TokenType::QUESTION, "?", sl, sc);
    if (c == '(') return Token(TokenType::LPAREN, "(", sl, sc);
    if (c == ')') return Token(TokenType::RPAREN, ")", sl, sc);
    if (c == '{') return Token(TokenType::LBRACE, "{", sl, sc);
    if (c == '}') return Token(TokenType::RBRACE, "}", sl, sc);
    if (c == '[') return Token(TokenType::LBRACKET, "[", sl, sc);
    if (c == ']') return Token(TokenType::RBRACKET, "]", sl, sc);
    if (c == ';') return Token(TokenType::SEMICOLON, ";", sl, sc);
    if (c == ',') return Token(TokenType::COMMA, ",", sl, sc);
    if (c == '#') return Token(TokenType::HASH, "#", sl, sc);

    // Unknown character
    return Token(TokenType::UNKNOWN, std::string(1, c), sl, sc);
}

// ── Main tokenize ──────────────────────────────────────────────────────────

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespace();
        if (isEOF()) break;

        char c = current();

        // ~~ line comment
        if (c == '~' && peek() == '~') {
            advance(); advance();
            skipLineComment();
            continue;
        }

        // << block comment >> (must be before '<' handling)
        if (c == '<' && peek() == '<') {
            int sl = line, sc = col;
            advance(); advance();
            skipBlockComment();
            continue;
        }

        if (std::isdigit(c)) {
            tokens.push_back(readNumber());
            continue;
        }
        if (std::isalpha(c) || c == '_') {
            tokens.push_back(readIdentifierOrKeyword());
            continue;
        }
        if (c == '"' || c == '\'') {
            tokens.push_back(readString());
            continue;
        }

        Token t = readOperatorOrPunct();
        if (t.type != TokenType::UNKNOWN || !t.value.empty()) {
            tokens.push_back(t);
        }
    }

    tokens.emplace_back(TokenType::EOF_TOKEN, "", line, col);
    return tokens;
}

