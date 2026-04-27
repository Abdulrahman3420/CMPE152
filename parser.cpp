#include "parser.h"
#include <iostream>
#include <string>

// ─── AST print helpers ───────────────────────────────────────────────────────
static std::string ind(int n) { return std::string(n * 2, ' '); }

void NumNode::print(int i) const {
    std::cout << ind(i) << "Number(" << value << ")\n";
}
void BoolNode::print(int i) const {
    std::cout << ind(i) << "Bool(" << (value ? "true" : "false") << ")\n";
}
void StringNode::print(int i) const {
    std::cout << ind(i) << "String(\"" << value << "\")\n";
}
void VarAccessNode::print(int i) const {
    std::cout << ind(i) << "VarAccess(" << name << ")\n";
}
void BinOpNode::print(int i) const {
    std::cout << ind(i) << "BinOp(" << op << ")\n";
    left->print(i + 1);
    right->print(i + 1);
}
void ComparisonNode::print(int i) const {
    std::cout << ind(i) << "Comparison(" << op << ")\n";
    left->print(i + 1);
    right->print(i + 1);
}
void VarDeclNode::print(int i) const {
    std::cout << ind(i) << "VarDecl(" << type << " " << name << ")\n";
    if (init) init->print(i + 1);
}
void VarAssignNode::print(int i) const {
    std::cout << ind(i) << "VarAssign(" << name << ")\n";
    value->print(i + 1);
}
void IncrementNode::print(int i) const {
    std::cout << ind(i) << "Increment(" << name << " " << op << ")\n";
}
void IfNode::print(int i) const {
    std::cout << ind(i) << "If\n";
    std::cout << ind(i+1) << "Condition:\n"; condition->print(i + 2);
    std::cout << ind(i+1) << "Then:\n";
    for (auto& s : ifBody)   s->print(i + 2);
    if (!elseBody.empty()) {
        std::cout << ind(i+1) << "Else:\n";
        for (auto& s : elseBody) s->print(i + 2);
    }
}
void PrintNode::print(int i) const {
    std::cout << ind(i) << "Print\n";
    for (auto& it : items) it->print(i + 1);
}
void ReturnNode::print(int i) const {
    std::cout << ind(i) << "Return\n";
    if (value) value->print(i + 1);
}
void ProgramNode::print(int i) const {
    std::cout << ind(i) << "Program\n";
    for (auto& s : statements) s->print(i + 1);
}

// ─── Parser ──────────────────────────────────────────────────────────────────
Parser::Parser(const std::vector<Token>& tokens) : toks(tokens), pos(0) {}

Token& Parser::current() { return toks[pos]; }
Token& Parser::peek(int offset) {
    int p = pos + offset;
    return (p < (int)toks.size()) ? toks[p] : toks.back();
}
bool Parser::check(TokenType t) { return current().type == t; }
bool Parser::match(TokenType t) {
    if (check(t)) { pos++; return true; }
    return false;
}
Token Parser::eat(TokenType expected, const std::string& msg) {
    if (check(expected)) { Token t = current(); pos++; return t; }
    errs.push_back({current().line, msg + " (got '" + current().value + "')"});
    return current(); // don't advance — let caller recover
}

void Parser::syncToNextStatement() {
    while (!check(TokenType::END_OF_FILE) &&
           !check(TokenType::SEMICOLON) &&
           !check(TokenType::RBRACE)) {
        pos++;
    }
    if (check(TokenType::SEMICOLON)) pos++;
}

std::unique_ptr<ProgramNode> Parser::parse() { return parseProgram(); }

std::unique_ptr<ProgramNode> Parser::parseProgram() {
    std::vector<NodePtr> stmts;
    // skip #include lines
    while (check(TokenType::HASH)) {
        pos++;
        while (!check(TokenType::END_OF_FILE) && current().line == toks[pos-1].line) pos++;
    }
    // skip "int main() {"
    while (!check(TokenType::LBRACE) && !check(TokenType::END_OF_FILE)) pos++;
    if (check(TokenType::LBRACE)) pos++;

    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        try {
            auto s = parseStatement();
            if (s) stmts.push_back(std::move(s));
        } catch (...) { syncToNextStatement(); }
    }
    if (check(TokenType::RBRACE)) pos++;
    return std::make_unique<ProgramNode>(std::move(stmts));
}

NodePtr Parser::parseStatement() {
    // Variable declarations
    if (check(TokenType::KW_INT) || check(TokenType::KW_FLOAT) || check(TokenType::KW_BOOL)) {
        std::string typeName = current().value; pos++;
        return parseVarDecl(typeName);
    }
    if (check(TokenType::KW_COUT))    return parsePrint();
    if (check(TokenType::KW_IF))      return parseIf();
    if (check(TokenType::KW_RETURN))  return parseReturn();
    if (check(TokenType::IDENTIFIER)) return parseAssignOrIncrement();
    // skip unknown tokens
    if (!check(TokenType::END_OF_FILE)) {
        errs.push_back({current().line, "Unexpected token '" + current().value + "'"});
        syncToNextStatement();
    }
    return nullptr;
}

NodePtr Parser::parseVarDecl(const std::string& typeName) {
    if (!check(TokenType::IDENTIFIER)) {
        errs.push_back({current().line, "Expected identifier after type '" + typeName + "'"});
        syncToNextStatement(); return nullptr;
    }
    std::string name = current().value; pos++;
    NodePtr init;
    if (match(TokenType::ASSIGN)) init = parseExpr();
    eat(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    return std::make_unique<VarDeclNode>(typeName, name, std::move(init));
}

NodePtr Parser::parseAssignOrIncrement() {
    std::string name = current().value; pos++;
    if (check(TokenType::INC)) {
        pos++;
        eat(TokenType::SEMICOLON, "Expected ';' after '++'");
        return std::make_unique<IncrementNode>(name, "++");
    }
    if (check(TokenType::DEC)) {
        pos++;
        eat(TokenType::SEMICOLON, "Expected ';' after '--'");
        return std::make_unique<IncrementNode>(name, "--");
    }
    eat(TokenType::ASSIGN, "Expected '=' in assignment");
    auto val = parseExpr();
    eat(TokenType::SEMICOLON, "Expected ';' after assignment");
    return std::make_unique<VarAssignNode>(name, std::move(val));
}

NodePtr Parser::parsePrint() {
    pos++; // consume cout
    eat(TokenType::LSHIFT, "Expected '<<' after 'cout'");
    std::vector<NodePtr> items;
    while (true) {
        if (check(TokenType::KW_ENDL)) {
            items.push_back(std::make_unique<StringNode>("\n")); pos++;
        } else {
            items.push_back(parseExpr());
        }
        if (!check(TokenType::LSHIFT)) break;
        pos++;
    }
    eat(TokenType::SEMICOLON, "Expected ';' after cout statement");
    return std::make_unique<PrintNode>(std::move(items));
}

NodePtr Parser::parseIf() {
    pos++; // consume if
    eat(TokenType::LPAREN, "Expected '(' after 'if'");
    auto cond = parseExpr();
    eat(TokenType::RPAREN, "Expected ')' after condition");
    auto ifBody = parseBlock();
    std::vector<NodePtr> elseBody;
    if (check(TokenType::KW_ELSE)) {
        pos++;
        elseBody = parseBlock();
    }
    return std::make_unique<IfNode>(std::move(cond), std::move(ifBody), std::move(elseBody));
}

NodePtr Parser::parseReturn() {
    pos++; // consume return
    NodePtr val;
    if (!check(TokenType::SEMICOLON)) val = parseExpr();
    eat(TokenType::SEMICOLON, "Expected ';' after return");
    return std::make_unique<ReturnNode>(std::move(val));
}

std::vector<NodePtr> Parser::parseBlock() {
    std::vector<NodePtr> stmts;
    if (check(TokenType::LBRACE)) {
        pos++;
        while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
            auto s = parseStatement();
            if (s) stmts.push_back(std::move(s));
        }
        eat(TokenType::RBRACE, "Expected '}'");
    } else {
        auto s = parseStatement();
        if (s) stmts.push_back(std::move(s));
    }
    return stmts;
}

// ─── Expression parsing (precedence climbing) ────────────────────────────────
NodePtr Parser::parseExpr()       { return parseComparison(); }

NodePtr Parser::parseComparison() {
    auto left = parseAddSub();
    while (check(TokenType::EQ) || check(TokenType::NEQ) ||
           check(TokenType::LT) || check(TokenType::GT) ||
           check(TokenType::LEQ) || check(TokenType::GEQ) ||
           check(TokenType::AND) || check(TokenType::OR)) {
        std::string op = current().value; pos++;
        auto right = parseAddSub();
        left = std::make_unique<ComparisonNode>(op, std::move(left), std::move(right));
    }
    return left;
}

NodePtr Parser::parseAddSub() {
    auto left = parseMulDiv();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = current().value; pos++;
        auto right = parseMulDiv();
        left = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
    }
    return left;
}

NodePtr Parser::parseMulDiv() {
    auto left = parseUnary();
    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT)) {
        std::string op = current().value; pos++;
        auto right = parseUnary();
        left = std::make_unique<BinOpNode>(op, std::move(left), std::move(right));
    }
    return left;
}

NodePtr Parser::parseUnary() {
    if (check(TokenType::MINUS)) { pos++; auto e = parsePrimary(); return std::make_unique<BinOpNode>("unary-", std::move(e), nullptr); }
    if (check(TokenType::NOT))   { pos++; auto e = parsePrimary(); return std::make_unique<BinOpNode>("!", std::move(e), nullptr); }
    return parsePrimary();
}

NodePtr Parser::parsePrimary() {
    if (check(TokenType::NUMBER) || check(TokenType::FLOAT_NUM)) {
        double v = std::stod(current().value); pos++;
        return std::make_unique<NumNode>(v);
    }
    if (check(TokenType::STRING_LIT)) {
        std::string s = current().value; pos++;
        return std::make_unique<StringNode>(s);
    }
    if (check(TokenType::KW_TRUE))  { pos++; return std::make_unique<BoolNode>(true); }
    if (check(TokenType::KW_FALSE)) { pos++; return std::make_unique<BoolNode>(false); }
    if (check(TokenType::IDENTIFIER)) {
        std::string name = current().value; pos++;
        return std::make_unique<VarAccessNode>(name);
    }
    if (check(TokenType::LPAREN)) {
        pos++;
        auto e = parseExpr();
        eat(TokenType::RPAREN, "Expected ')'");
        return e;
    }
    errs.push_back({current().line, "Unexpected token in expression: '" + current().value + "'"});
    if (!check(TokenType::END_OF_FILE)) pos++;
    return std::make_unique<NumNode>(0);
}
