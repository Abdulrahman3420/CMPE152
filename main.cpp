#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"
#include "codegen.h"

std::string tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::NUMBER:     return "NUMBER";
        case TokenType::FLOAT_NUM:  return "FLOAT";
        case TokenType::STRING_LIT: return "STRING";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::KW_INT:     return "KEYWORD(int)";
        case TokenType::KW_FLOAT:   return "KEYWORD(float)";
        case TokenType::KW_BOOL:    return "KEYWORD(bool)";
        case TokenType::KW_TRUE:    return "KEYWORD(true)";
        case TokenType::KW_FALSE:   return "KEYWORD(false)";
        case TokenType::KW_IF:      return "KEYWORD(if)";
        case TokenType::KW_ELSE:    return "KEYWORD(else)";
        case TokenType::KW_COUT:    return "KEYWORD(cout)";
        case TokenType::KW_ENDL:    return "KEYWORD(endl)";
        case TokenType::KW_INCLUDE: return "KEYWORD(include)";
        case TokenType::KW_MAIN:    return "KEYWORD(main)";
        case TokenType::KW_RETURN:  return "KEYWORD(return)";
        case TokenType::PLUS:       return "OP(+)";
        case TokenType::MINUS:      return "OP(-)";
        case TokenType::STAR:       return "OP(*)";
        case TokenType::SLASH:      return "OP(/)";
        case TokenType::PERCENT:    return "OP(%)";
        case TokenType::ASSIGN:     return "OP(=)";
        case TokenType::EQ:         return "OP(==)";
        case TokenType::NEQ:        return "OP(!=)";
        case TokenType::LT:         return "OP(<)";
        case TokenType::GT:         return "OP(>)";
        case TokenType::LEQ:        return "OP(<=)";
        case TokenType::GEQ:        return "OP(>=)";
        case TokenType::AND:        return "OP(&&)";
        case TokenType::OR:         return "OP(||)";
        case TokenType::NOT:        return "OP(!)";
        case TokenType::INC:        return "OP(++)";
        case TokenType::DEC:        return "OP(--)";
        case TokenType::LPAREN:     return "PUNCT(()";
        case TokenType::RPAREN:     return "PUNCT())";
        case TokenType::LBRACE:     return "PUNCT({)";
        case TokenType::RBRACE:     return "PUNCT(})";
        case TokenType::SEMICOLON:  return "PUNCT(;)";
        case TokenType::COMMA:      return "PUNCT(,)";
        case TokenType::HASH:       return "PUNCT(#)";
        case TokenType::LSHIFT:     return "OP(<<)";
        case TokenType::END_OF_FILE:return "EOF";
        default:                    return "UNKNOWN";
    }
}

void banner(const std::string& title) {
    std::cout << "\n" << title << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: compiler <source_file.cpp>\n";
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) { std::cerr << "Error: cannot open '" << argv[1] << "'\n"; return 1; }
    std::ostringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();

    std::cout << "Simple C++ Compiler - CMPE 152\n";
    std::cout << "Compiling: " << argv[1] << "\n";

    banner("PHASE 1: LEXICAL ANALYSIS — Token Generation");
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    std::cout << "\n  Token Classification:\n";
    printf("  %-8s %-20s %s\n", "Line", "Type", "Value");

    int keywords=0, identifiers=0, operators=0, punctuation=0, literals=0;
    for (auto& tok : tokens) {
        if (tok.type == TokenType::END_OF_FILE) break;
        std::string tn = tokenTypeName(tok.type);
        printf("  %-8d %-22s %s\n", tok.line, tn.c_str(), tok.value.c_str());
        if (tn.find("KEYWORD") != std::string::npos) keywords++;
        else if (tn == "IDENTIFIER") identifiers++;
        else if (tn.find("OP") != std::string::npos) operators++;
        else if (tn.find("PUNCT") != std::string::npos) punctuation++;
        else if (tn == "NUMBER" || tn == "FLOAT" || tn == "STRING") literals++;
    }
    std::cout << "\n  Token Summary:\n";
    printf("    Keywords:    %d\n", keywords);
    printf("    Identifiers: %d\n", identifiers);
    printf("    Operators:   %d\n", operators);
    printf("    Punctuation: %d\n", punctuation);
    printf("    Literals:    %d\n", literals);
    printf("    Total:       %d\n", keywords+identifiers+operators+punctuation+literals);

    banner("PHASE 2: PARSING — Abstract Syntax Tree");
    Parser parser(tokens);
    auto ast = parser.parse();
    const auto& parseErrors = parser.errors();

    if (!parseErrors.empty()) {
        std::cout << "\n  *** SYNTAX ERRORS FOUND — Compilation Failed ***\n\n";
        for (auto& e : parseErrors) {
            printf("  [ERROR] Line %d: %s\n", e.line, e.message.c_str());
        }
        std::cout << "\n  Compiler will still display partial AST below.\n";
    }

    std::cout << "\n  Abstract Syntax Tree:\n";
    ast->print(1);

    if (!parseErrors.empty()) {
        std::cout << "\n  Total errors: " << parseErrors.size() << "\n";
        std::cout << "  Target code generation skipped due to errors.\n";
        return 1;
    }

    banner("PHASE 3: INTERMEDIATE CODE GENERATION (3-Address IR)");
    CodeGen cg;
    cg.generate(*ast);
    std::cout << "\n" << cg.intermediate();

    banner("PHASE 4: ASSEMBLY CODE GENERATION");
    std::cout << "\n" << cg.assembly();

    banner("PHASE 5: INTERPRETER EXECUTION — Program Output");
    std::cout << "\n";
    try {
        Interpreter interp;
        interp.execute(*ast);

        std::cout << "\n\n  Symbol Table (final state):\n";
        for (auto& [name, val] : interp.symbolTable()) {
            printf("  %-12s = %s\n", name.c_str(), valueToString(val).c_str());
        }
        std::cout << "\n  Compilation successful. No errors.\n";
    } catch (std::exception& e) {
        std::cerr << "\n  [RUNTIME ERROR] " << e.what() << "\n";
        return 1;
    }

    return 0;
}
