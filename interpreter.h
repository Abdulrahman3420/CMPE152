#pragma once
#include "parser.h"
#include <unordered_map>
#include <variant>
#include <string>

using Value = std::variant<double, bool, std::string>;

std::string valueToString(const Value& v);

class Interpreter {
public:
    void execute(const ProgramNode& prog);
    const std::unordered_map<std::string, Value>& symbolTable() const { return symtab; }

private:
    std::unordered_map<std::string, Value> symtab;

    Value evalExpr(const ASTNode* node);
    void  execStmt(const ASTNode* node);
    bool  isTruthy(const Value& v);
};
