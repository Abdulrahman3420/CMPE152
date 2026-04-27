#include "codegen.h"
#include <sstream>
#include <stdexcept>

std::string CodeGen::newTemp()  { return "t" + std::to_string(tempCount++); }
std::string CodeGen::newLabel() { return "L" + std::to_string(labelCount++); }

void CodeGen::generate(const ProgramNode& prog) {
    ir.str(""); asm_.str("");
    ir  << "; === Intermediate Representation (3-Address Code) ===\n";
    asm_ << "; === x86-like Assembly Code ===\n";
    asm_ << "section .data\n";
    asm_ << "section .text\n";
    asm_ << "global main\n";
    asm_ << "main:\n";
    asm_ << "  push rbp\n  mov rbp, rsp\n";

    for (auto& stmt : prog.statements) genStmt(stmt.get());

    asm_ << "  mov eax, 0\n";
    asm_ << "  pop rbp\n";
    asm_ << "  ret\n";

    irCode  = ir.str();
    asmCode = asm_.str();
}

std::string CodeGen::genExpr(const ASTNode* node) {
    if (!node) return "0";

    if (auto* n = dynamic_cast<const NumNode*>(node)) {
        std::string t = newTemp();
        ir   << "  " << t << " = " << n->value << "\n";
        asm_ << "  mov rax, " << (long long)n->value << "\n";
        asm_ << "  mov [" << t << "], rax\n";
        return t;
    }
    if (auto* b = dynamic_cast<const BoolNode*>(node)) {
        std::string t = newTemp();
        ir   << "  " << t << " = " << (b->value ? 1 : 0) << "\n";
        asm_ << "  mov rax, " << (b->value ? 1 : 0) << "\n";
        asm_ << "  mov [" << t << "], rax\n";
        return t;
    }
    if (auto* s = dynamic_cast<const StringNode*>(node)) {
        std::string t = newTemp();
        ir   << "  " << t << " = \"" << s->value << "\"\n";
        asm_ << "  lea rax, [str_" << t << "]\n";
        return t;
    }
    if (auto* v = dynamic_cast<const VarAccessNode*>(node)) {
        std::string t = newTemp();
        ir   << "  " << t << " = " << v->name << "\n";
        asm_ << "  mov rax, [" << v->name << "]\n";
        asm_ << "  mov [" << t << "], rax\n";
        return t;
    }
    if (auto* bin = dynamic_cast<const BinOpNode*>(node)) {
        if (bin->op == "unary-") {
            std::string l = genExpr(bin->left.get());
            std::string t = newTemp();
            ir   << "  " << t << " = -" << l << "\n";
            asm_ << "  mov rax, [" << l << "]\n  neg rax\n";
            asm_ << "  mov [" << t << "], rax\n";
            return t;
        }
        std::string l = genExpr(bin->left.get());
        std::string r = genExpr(bin->right.get());
        std::string t = newTemp();
        ir   << "  " << t << " = " << l << " " << bin->op << " " << r << "\n";
        asm_ << "  mov rax, [" << l << "]\n";
        if (bin->op == "+") {
            asm_ << "  add rax, [" << r << "]\n";
        } else if (bin->op == "-") {
            asm_ << "  sub rax, [" << r << "]\n";
        } else if (bin->op == "*") {
            asm_ << "  imul rax, [" << r << "]\n";
        } else if (bin->op == "/") {
            asm_ << "  cqo\n  idiv qword [" << r << "]\n";
        } else if (bin->op == "%") {
            asm_ << "  cqo\n  idiv qword [" << r << "]\n  mov rax, rdx\n";
        }
        asm_ << "  mov [" << t << "], rax\n";
        return t;
    }
    if (auto* cmp = dynamic_cast<const ComparisonNode*>(node)) {
        std::string l = genExpr(cmp->left.get());
        std::string r = genExpr(cmp->right.get());
        std::string t = newTemp();
        ir   << "  " << t << " = " << l << " " << cmp->op << " " << r << "\n";
        asm_ << "  mov rax, [" << l << "]\n";
        asm_ << "  cmp rax, [" << r << "]\n";
        if (cmp->op == "==")      asm_ << "  sete al\n";
        else if (cmp->op == "!=") asm_ << "  setne al\n";
        else if (cmp->op == "<")  asm_ << "  setl al\n";
        else if (cmp->op == ">")  asm_ << "  setg al\n";
        else if (cmp->op == "<=") asm_ << "  setle al\n";
        else if (cmp->op == ">=") asm_ << "  setge al\n";
        asm_ << "  movzx rax, al\n";
        asm_ << "  mov [" << t << "], rax\n";
        return t;
    }
    return "0";
}

void CodeGen::genStmt(const ASTNode* node) {
    if (!node) return;

    if (auto* d = dynamic_cast<const VarDeclNode*>(node)) {
        ir << "; declare " << d->type << " " << d->name << "\n";
        asm_ << "  ; declare " << d->type << " " << d->name << "\n";
        std::string val = d->init ? genExpr(d->init.get()) : "0";
        ir   << "  " << d->name << " = " << val << "\n";
        asm_ << "  mov rax, [" << val << "]\n";
        asm_ << "  mov [" << d->name << "], rax\n";
    }
    else if (auto* a = dynamic_cast<const VarAssignNode*>(node)) {
        std::string val = genExpr(a->value.get());
        ir   << "  " << a->name << " = " << val << "\n";
        asm_ << "  mov rax, [" << val << "]\n";
        asm_ << "  mov [" << a->name << "], rax\n";
    }
    else if (auto* inc = dynamic_cast<const IncrementNode*>(node)) {
        std::string op = inc->op == "++" ? "add" : "sub";
        ir   << "  " << inc->name << " = " << inc->name << " " << (inc->op == "++" ? "+" : "-") << " 1\n";
        asm_ << "  " << op << " qword [" << inc->name << "], 1\n";
    }
    else if (auto* p = dynamic_cast<const PrintNode*>(node)) {
        ir << "; cout <<\n";
        asm_ << "  ; print\n";
        for (auto& item : p->items) {
            std::string val = genExpr(item.get());
            ir   << "  print " << val << "\n";
            asm_ << "  mov rdi, [" << val << "]\n";
            asm_ << "  call print_value\n";
        }
    }
    else if (auto* ifn = dynamic_cast<const IfNode*>(node)) {
        std::string cond = genExpr(ifn->condition.get());
        std::string lElse = newLabel(), lEnd = newLabel();

        ir   << "  if " << cond << " == 0 goto " << lElse << "\n";
        asm_ << "  mov rax, [" << cond << "]\n";
        asm_ << "  cmp rax, 0\n";
        asm_ << "  jz " << lElse << "\n";

        for (auto& s : ifn->ifBody) genStmt(s.get());

        ir   << "  goto " << lEnd << "\n";
        asm_ << "  jmp " << lEnd << "\n";
        ir   << lElse << ":\n";
        asm_ << lElse << ":\n";

        for (auto& s : ifn->elseBody) genStmt(s.get());

        ir   << lEnd << ":\n";
        asm_ << lEnd << ":\n";
    }
    else if (auto* ret = dynamic_cast<const ReturnNode*>(node)) {
        if (ret->value) {
            std::string val = genExpr(ret->value.get());
            ir   << "  return " << val << "\n";
            asm_ << "  mov eax, [" << val << "]\n";
        } else {
            ir   << "  return 0\n";
            asm_ << "  mov eax, 0\n";
        }
        asm_ << "  pop rbp\n  ret\n";
    }
}
