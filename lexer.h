#pragma once
#include <string>
#include <vector>

enum class TokenType {
    NUMBER, FLOAT_NUM, STRING_LIT, IDENTIFIER,
    KW_INT, KW_FLOAT, KW_BOOL, KW_TRUE, KW_FALSE,
    KW_IF, KW_ELSE, KW_COUT, KW_ENDL,
    KW_INCLUDE, KW_MAIN, KW_RETURN,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN,
    EQ, NEQ, LT, GT, LEQ, GEQ,
    AND, OR, NOT,
    INC, DEC,
    LPAREN, RPAREN, LBRACE, RBRACE,
    SEMICOLON, COMMA, HASH, DOT,
    LSHIFT,
    END_OF_FILE, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

class Lexer {
public:
    Lexer(const std::string& src);
    std::vector<Token> tokenize();

private:
    std::string source;
    int pos;
    int line;

    char current();
    char peek(int offset = 1);
    char advance();
    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();
    Token readNumber();
    Token readString();
    Token readIdentifierOrKeyword();
    TokenType classifyKeyword(const std::string& word);
};
