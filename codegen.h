#pragma once
#include "parser.h"
#include <string>
#include <vector>
#include <sstream>

class CodeGen {
public:
    void generate(const ProgramNode& prog);
    const std::string& intermediate() const { return irCode; }
    const std::string& assembly()     const { return asmCode; }

private:
    std::ostringstream ir, asm_;
    std::string irCode, asmCode;
    int tempCount = 0;
    int labelCount = 0;

    std::string newTemp();
    std::string newLabel();

    std::string genExpr(const ASTNode* node);
    void genStmt(const ASTNode* node);
};
