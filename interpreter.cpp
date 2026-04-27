#include "interpreter.h"
#include <iostream>
#include <stdexcept>
#include <cmath>

std::string valueToString(const Value& v) {
    return std::visit([](auto&& x) -> std::string {
        using T = std::decay_t<decltype(x)>;
        if constexpr (std::is_same_v<T, double>) {
            if (x == (long long)x) return std::to_string((long long)x);
            return std::to_string(x);
        } else if constexpr (std::is_same_v<T, bool>) {
            return x ? "true" : "false";
        } else {
            return x;
        }
    }, v);
}

bool Interpreter::isTruthy(const Value& v) {
    if (std::holds_alternative<double>(v))      return std::get<double>(v) != 0.0;
    if (std::holds_alternative<bool>(v))        return std::get<bool>(v);
    return !std::get<std::string>(v).empty();
}

void Interpreter::execute(const ProgramNode& prog) {
    for (auto& stmt : prog.statements) execStmt(stmt.get());
}

void Interpreter::execStmt(const ASTNode* node) {
    if (!node) return;

    if (auto* d = dynamic_cast<const VarDeclNode*>(node)) {
        Value v = d->init ? evalExpr(d->init.get()) : Value{0.0};
        symtab[d->name] = v;
    }
    else if (auto* a = dynamic_cast<const VarAssignNode*>(node)) {
        if (symtab.find(a->name) == symtab.end())
            throw std::runtime_error("Undeclared variable: " + a->name);
        symtab[a->name] = evalExpr(a->value.get());
    }
    else if (auto* inc = dynamic_cast<const IncrementNode*>(node)) {
        if (symtab.find(inc->name) == symtab.end())
            throw std::runtime_error("Undeclared variable: " + inc->name);
        double cur = std::get<double>(symtab[inc->name]);
        symtab[inc->name] = inc->op == "++" ? cur + 1.0 : cur - 1.0;
    }
    else if (auto* p = dynamic_cast<const PrintNode*>(node)) {
        for (auto& item : p->items) {
            Value v = evalExpr(item.get());
            std::string s = valueToString(v);
            if (s == "\n") std::cout << "\n";
            else std::cout << s;
        }
    }
    else if (auto* ifn = dynamic_cast<const IfNode*>(node)) {
        bool cond = isTruthy(evalExpr(ifn->condition.get()));
        auto& body = cond ? ifn->ifBody : ifn->elseBody;
        for (auto& s : body) execStmt(s.get());
    }
    else if (auto* ret = dynamic_cast<const ReturnNode*>(node)) {
        // For main(), just ignore return value
    }
}

Value Interpreter::evalExpr(const ASTNode* node) {
    if (!node) return 0.0;

    if (auto* n = dynamic_cast<const NumNode*>(node))
        return n->value;
    if (auto* b = dynamic_cast<const BoolNode*>(node))
        return b->value;
    if (auto* s = dynamic_cast<const StringNode*>(node))
        return s->value;
    if (auto* v = dynamic_cast<const VarAccessNode*>(node)) {
        if (symtab.find(v->name) == symtab.end())
            throw std::runtime_error("Undeclared variable: " + v->name);
        return symtab[v->name];
    }

    if (auto* bin = dynamic_cast<const BinOpNode*>(node)) {
        // unary
        if (bin->op == "unary-") {
            Value r = evalExpr(bin->left.get());
            return -std::get<double>(r);
        }
        if (bin->op == "!") {
            return !isTruthy(evalExpr(bin->left.get()));
        }
        Value lv = evalExpr(bin->left.get());
        Value rv = evalExpr(bin->right.get());
        // string concat
        if (std::holds_alternative<std::string>(lv) || std::holds_alternative<std::string>(rv)) {
            return valueToString(lv) + valueToString(rv);
        }
        double l = std::get<double>(lv), r = std::get<double>(rv);
        if (bin->op == "+") return l + r;
        if (bin->op == "-") return l - r;
        if (bin->op == "*") return l * r;
        if (bin->op == "/") {
            if (r == 0) throw std::runtime_error("Division by zero");
            return l / r;
        }
        if (bin->op == "%") return std::fmod(l, r);
    }

    if (auto* cmp = dynamic_cast<const ComparisonNode*>(node)) {
        Value lv = evalExpr(cmp->left.get());
        Value rv = evalExpr(cmp->right.get());
        if (cmp->op == "&&") return isTruthy(lv) && isTruthy(rv);
        if (cmp->op == "||") return isTruthy(lv) || isTruthy(rv);
        if (std::holds_alternative<std::string>(lv)) {
            std::string ls = std::get<std::string>(lv), rs = valueToString(rv);
            if (cmp->op == "==") return ls == rs;
            if (cmp->op == "!=") return ls != rs;
            return false;
        }
        double l = std::get<double>(lv), r = std::get<double>(rv);
        if (cmp->op == "==") return l == r;
        if (cmp->op == "!=") return l != r;
        if (cmp->op == "<")  return l < r;
        if (cmp->op == ">")  return l > r;
        if (cmp->op == "<=") return l <= r;
        if (cmp->op == ">=") return l >= r;
    }

    return 0.0;
}
