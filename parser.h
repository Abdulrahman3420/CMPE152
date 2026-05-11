#pragma once
#include "lexer.h"
#include <memory>
#include <vector>
#include <string>

struct ASTNode {
    virtual ~ASTNode() = default;
    virtual std::string nodeType() const = 0;
    virtual void print(int indent = 0) const = 0;
};
using NodePtr = std::unique_ptr<ASTNode>;

struct NumNode : ASTNode {
    double value;
    NumNode(double v) : value(v) {}
    std::string nodeType() const override { return "Number"; }
    void print(int i = 0) const override;
};

struct BoolNode : ASTNode {
    bool value;
    BoolNode(bool v) : value(v) {}
    std::string nodeType() const override { return "Bool"; }
    void print(int i = 0) const override;
};

struct StringNode : ASTNode {
    std::string value;
    StringNode(const std::string& v) : value(v) {}
    std::string nodeType() const override { return "String"; }
    void print(int i = 0) const override;
};

struct VarAccessNode : ASTNode {
    std::string name;
    VarAccessNode(const std::string& n) : name(n) {}
    std::string nodeType() const override { return "VarAccess"; }
    void print(int i = 0) const override;
};

struct BinOpNode : ASTNode {
    std::string op;
    NodePtr left, right;
    BinOpNode(std::string o, NodePtr l, NodePtr r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
    std::string nodeType() const override { return "BinOp"; }
    void print(int i = 0) const override;
};

struct ComparisonNode : ASTNode {
    std::string op;
    NodePtr left, right;
    ComparisonNode(std::string o, NodePtr l, NodePtr r)
        : op(std::move(o)), left(std::move(l)), right(std::move(r)) {}
    std::string nodeType() const override { return "Comparison"; }
    void print(int i = 0) const override;
};

struct VarDeclNode : ASTNode {
    std::string type, name;
    NodePtr init;
    VarDeclNode(std::string t, std::string n, NodePtr i)
        : type(std::move(t)), name(std::move(n)), init(std::move(i)) {}
    std::string nodeType() const override { return "VarDecl"; }
    void print(int i = 0) const override;
};

struct VarAssignNode : ASTNode {
    std::string name;
    NodePtr value;
    VarAssignNode(std::string n, NodePtr v) : name(std::move(n)), value(std::move(v)) {}
    std::string nodeType() const override { return "VarAssign"; }
    void print(int i = 0) const override;
};

struct IncrementNode : ASTNode {
    std::string name, op;
    IncrementNode(std::string n, std::string o) : name(std::move(n)), op(std::move(o)) {}
    std::string nodeType() const override { return "Increment"; }
    void print(int i = 0) const override;
};

struct IfNode : ASTNode {
    NodePtr condition;
    std::vector<NodePtr> ifBody, elseBody;
    IfNode(NodePtr c, std::vector<NodePtr> ib, std::vector<NodePtr> eb)
        : condition(std::move(c)), ifBody(std::move(ib)), elseBody(std::move(eb)) {}
    std::string nodeType() const override { return "If"; }
    void print(int i = 0) const override;
};

struct PrintNode : ASTNode {
    std::vector<NodePtr> items;
    PrintNode(std::vector<NodePtr> it) : items(std::move(it)) {}
    std::string nodeType() const override { return "Print"; }
    void print(int i = 0) const override;
};

struct ReturnNode : ASTNode {
    NodePtr value;
    ReturnNode(NodePtr v) : value(std::move(v)) {}
    std::string nodeType() const override { return "Return"; }
    void print(int i = 0) const override;
};

struct ProgramNode : ASTNode {
    std::vector<NodePtr> statements;
    ProgramNode(std::vector<NodePtr> s) : statements(std::move(s)) {}
    std::string nodeType() const override { return "Program"; }
    void print(int i = 0) const override;
};

struct ParseError {
    int line;
    std::string message;
};

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    std::unique_ptr<ProgramNode> parse();
    const std::vector<ParseError>& errors() const { return errs; }

private:
    std::vector<Token> toks;
    int pos;
    std::vector<ParseError> errs;

    Token& current();
    Token& peek(int offset = 1);
    Token eat(TokenType expected, const std::string& msg);
    bool match(TokenType t);
    bool check(TokenType t);
    void syncToNextStatement();

    std::unique_ptr<ProgramNode> parseProgram();
    NodePtr parseStatement();
    NodePtr parseVarDecl(const std::string& typeName);
    NodePtr parseAssignOrIncrement();
    NodePtr parsePrint();
    NodePtr parseIf();
    NodePtr parseReturn();
    std::vector<NodePtr> parseBlock();

    NodePtr parseExpr();
    NodePtr parseComparison();
    NodePtr parseAddSub();
    NodePtr parseMulDiv();
    NodePtr parseUnary();
    NodePtr parsePrimary();
};
