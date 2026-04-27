#include "lexer.h"
#include <stdexcept>
#include <cctype>

Lexer::Lexer(const std::string& src) : source(src), pos(0), line(1) {}

char Lexer::current() {
    return (pos < (int)source.size()) ? source[pos] : '\0';
}

char Lexer::peek(int offset) {
    int p = pos + offset;
    return (p < (int)source.size()) ? source[p] : '\0';
}

char Lexer::advance() {
    char c = source[pos++];
    if (c == '\n') line++;
    return c;
}

void Lexer::skipWhitespace() {
    while (pos < (int)source.size() && std::isspace(current())) advance();
}

void Lexer::skipLineComment() {
    while (pos < (int)source.size() && current() != '\n') advance();
}

void Lexer::skipBlockComment() {
    advance(); advance(); // consume /*
    while (pos < (int)source.size()) {
        if (current() == '*' && peek() == '/') { advance(); advance(); return; }
        advance();
    }
}

Token Lexer::readNumber() {
    std::string num;
    int startLine = line;
    bool isFloat = false;
    while (pos < (int)source.size() && (std::isdigit(current()) || current() == '.')) {
        if (current() == '.') isFloat = true;
        num += advance();
    }
    return { isFloat ? TokenType::FLOAT_NUM : TokenType::NUMBER, num, startLine };
}

Token Lexer::readString() {
    int startLine = line;
    advance(); // consume opening "
    std::string s;
    while (pos < (int)source.size() && current() != '"') {
        if (current() == '\\' && peek() == 'n') { advance(); advance(); s += '\n'; }
        else s += advance();
    }
    if (current() == '"') advance(); // consume closing "
    return { TokenType::STRING_LIT, s, startLine };
}

Token Lexer::readIdentifierOrKeyword() {
    int startLine = line;
    std::string word;
    while (pos < (int)source.size() && (std::isalnum(current()) || current() == '_')) {
        word += advance();
    }
    TokenType kw = classifyKeyword(word);
    return { kw, word, startLine };
}

TokenType Lexer::classifyKeyword(const std::string& w) {
    if (w == "int")    return TokenType::KW_INT;
    if (w == "float")  return TokenType::KW_FLOAT;
    if (w == "bool")   return TokenType::KW_BOOL;
    if (w == "true")   return TokenType::KW_TRUE;
    if (w == "false")  return TokenType::KW_FALSE;
    if (w == "if")     return TokenType::KW_IF;
    if (w == "else")   return TokenType::KW_ELSE;
    if (w == "cout")   return TokenType::KW_COUT;
    if (w == "endl")   return TokenType::KW_ENDL;
    if (w == "include") return TokenType::KW_INCLUDE;
    if (w == "main")   return TokenType::KW_MAIN;
    if (w == "return") return TokenType::KW_RETURN;
    return TokenType::IDENTIFIER;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos < (int)source.size()) {
        skipWhitespace();
        if (pos >= (int)source.size()) break;

        int startLine = line;
        char c = current();

        // Comments
        if (c == '/' && peek() == '/') { skipLineComment(); continue; }
        if (c == '/' && peek() == '*') { skipBlockComment(); continue; }

        // String literals
        if (c == '"') { tokens.push_back(readString()); continue; }

        // Numbers
        if (std::isdigit(c)) { tokens.push_back(readNumber()); continue; }

        // Identifiers / keywords
        if (std::isalpha(c) || c == '_') { tokens.push_back(readIdentifierOrKeyword()); continue; }

        // Multi-char operators
        advance();
        switch (c) {
            case '+':
                if (current() == '+') { advance(); tokens.push_back({TokenType::INC, "++", startLine}); }
                else tokens.push_back({TokenType::PLUS, "+", startLine});
                break;
            case '-':
                if (current() == '-') { advance(); tokens.push_back({TokenType::DEC, "--", startLine}); }
                else tokens.push_back({TokenType::MINUS, "-", startLine});
                break;
            case '*': tokens.push_back({TokenType::STAR,    "*",  startLine}); break;
            case '/': tokens.push_back({TokenType::SLASH,   "/",  startLine}); break;
            case '%': tokens.push_back({TokenType::PERCENT, "%",  startLine}); break;
            case '=':
                if (current() == '=') { advance(); tokens.push_back({TokenType::EQ,     "==", startLine}); }
                else tokens.push_back({TokenType::ASSIGN, "=", startLine});
                break;
            case '!':
                if (current() == '=') { advance(); tokens.push_back({TokenType::NEQ,    "!=", startLine}); }
                else tokens.push_back({TokenType::NOT, "!", startLine});
                break;
            case '<':
                if (current() == '=') { advance(); tokens.push_back({TokenType::LEQ,    "<=", startLine}); }
                else if (current() == '<') { advance(); tokens.push_back({TokenType::LSHIFT, "<<", startLine}); }
                else tokens.push_back({TokenType::LT, "<", startLine});
                break;
            case '>':
                if (current() == '=') { advance(); tokens.push_back({TokenType::GEQ,    ">=", startLine}); }
                else tokens.push_back({TokenType::GT, ">", startLine});
                break;
            case '&':
                if (current() == '&') { advance(); tokens.push_back({TokenType::AND,    "&&", startLine}); }
                else tokens.push_back({TokenType::UNKNOWN, "&", startLine});
                break;
            case '|':
                if (current() == '|') { advance(); tokens.push_back({TokenType::OR,     "||", startLine}); }
                else tokens.push_back({TokenType::UNKNOWN, "|", startLine});
                break;
            case '(': tokens.push_back({TokenType::LPAREN,    "(", startLine}); break;
            case ')': tokens.push_back({TokenType::RPAREN,    ")", startLine}); break;
            case '{': tokens.push_back({TokenType::LBRACE,    "{", startLine}); break;
            case '}': tokens.push_back({TokenType::RBRACE,    "}", startLine}); break;
            case ';': tokens.push_back({TokenType::SEMICOLON, ";", startLine}); break;
            case ',': tokens.push_back({TokenType::COMMA,     ",", startLine}); break;
            case '#': tokens.push_back({TokenType::HASH,      "#", startLine}); break;
            case '.': tokens.push_back({TokenType::DOT,       ".", startLine}); break;
            default:  tokens.push_back({TokenType::UNKNOWN,   std::string(1,c), startLine}); break;
        }
    }
    tokens.push_back({TokenType::END_OF_FILE, "", line});
    return tokens;
}
