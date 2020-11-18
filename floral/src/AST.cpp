//
//  AST.cpp
//  floral
//
//  Created by Ethan Uppal on 6/29/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "AST.hpp"
#include "Token.hpp"
#include "FilePath.hpp"
#include <vector>
#include <iostream>
#include "Colors.hpp"

namespace Floral {
    TextRegion::TextRegion(const Token& value): path(value.loc.filename) {
        pos = value.pos();
        length = value.contents.size();
        startLine = value.line();
        endLine = value.line();
        
    }
    TextRegion::TextRegion(const Token& first, const Token& last): path(first.loc.filename) {
        pos = first.pos();
        length = last.end() - pos;
        startLine = first.line();
        endLine = last.line();
    }
    TextRegion::TextRegion(size_t pos, size_t length, size_t startLine, size_t endLine):
    pos(pos), length(length), startLine(startLine), endLine(endLine) {}
    void TextRegion::describe(char term = '\n') const {
        std::cout << "{" << pos << "..." << pos + length;
        if (startLine == endLine)
            std::cout << ", line " << startLine;
        else
            std::cout << ", lines " << startLine << "-" << endLine;
        std::cout << "}" << term;
    }
    Node::Node(TextRegion loc): _loc(loc) {}
    Node::~Node() {}
    void Node::print() const {
        std::cout << "Node at loc";
        _loc.describe();
    };

    File::File(TextRegion loc, const std::string &path, const std::vector<Node*> &nodes): Node(loc), _path(path), _nodes(nodes), _main(nullptr) {}
    File::~File() {
        for (auto node: _nodes) {
            dealloc(node);
        }
    }
    void File::print() const {
        FilePath filePath { _path };
        std::cout << "File at " << filePath.last() << " with " << _loc.endLine << " lines.\n";
    }
    void File::insert(Node *node) {
        if (auto func = dynamic_cast<Function*>(node)) {
            if (func->name().contents == "main" && func->returnType()->isInt()) {
                _main = func;
                return;
            }
        }
        _nodes.push_back(node);
    }
    void File::setPath(const std::string &path) {
        _path = path;
    }
    const std::vector<Node*>& File::nodes() const {
        return _nodes;
    }
    Function* File::main() const {
        return _main;
    }
    void File::dump() const {
        ColoredStream colout(std::cout);
        colout << Color::reset << Color::cyan << "File Constituents:\n" << Color::reset;
        for (auto node: _nodes) {
            std::cout << "- ";
            node->print();
            if (auto fn = dynamic_cast<Floral::Function*>(node)) {
                auto body { fn->body() };
                for (auto stm: body) {
                    std::cout << "  - ";
                    stm->print();
                }
            }
        }
        if (_main) {
            std::cout << "- ";
            _main->print();
            for (auto stm: _main->body()) {
                std::cout << "  - ";
                stm->print();
            }
        }
    }
    const std::string& File::path() const {
        return _path;
    }


    Declaration::Declaration(TextRegion loc): Node(loc) {}
    void Declaration::print() const {
        std::cout << "Declaration at loc ";
        _loc.describe();
    }
    Function::Parameter::Parameter(const Token& name, Type* type): name(name), type(type) {}
    Function::Function(TextRegion loc, const Token& name, const Parameters& parameters, Type* returnType): Declaration(loc), _name(name), _parameters(parameters), _retType(returnType) {}
    Function::~Function() {
        for (auto stm: _body)
            dealloc(stm);
        for (auto p: _parameters)
            ;//dealloc(p.type);
        dealloc(_retType);
    }
    void Function::print() const {
        std::cout << "Function at loc ";
        _loc.describe(' ');
        std::cout << "named '" << _name.contents << "(";
        for (size_t i {}; i < _parameters.size(); ++i) {
            std::cout << _parameters[i].name.contents << ": ";
            _parameters[i].type->print();
            if (i + 1 != _parameters.size())
                std::cout << ", ";
        }
        std::cout << "): ";
        if (_retType->isVoid() || _retType->isIncomplete())
            std::cout << "Void";
        else
            _retType->print();
        std::cout << "'\n";
    }
    void Function::insert(Statement* stm) {
        _body.push_back(stm);
    }
    const std::vector<Statement*>& Function::body() const {
        return _body;
    }
    Token& Function::name() {
        return _name;
    }
    size_t Function::arity() const {
        return _parameters.size();
    }
    const Type* Function::returnType() const {
        return _retType;
    }
    const Function::Parameters& Function::parameters() const {
        return _parameters;
    }
    void Function::setRType(Type* newType) {
        _retType = newType;
    }
    const bool Function::isLeaf() const {
        for (auto node: _body) {
            if (dynamic_cast<CallStatement*>(node)) {
                return false;
            } else if (auto block = dynamic_cast<Block*>(node)) {
                if (!block->isLeaf()) return false;
            }
        }
        return true;
    }
    #define REPLACE_BIT(v, b, i)\
        (v) |= (1 << (i));\
        (v) &= ((b) << (i))
    const Function::AttributeStorage Function::isStatic() const {
        return _attributes & (1 << static_);
    }
    void Function::setStatic(Function::AttributeStorage bit) {
        REPLACE_BIT(_attributes, bit, static_);
    }
    const Function::AttributeStorage Function::isInline() const {
        return _attributes & (1 << inline_);
    }
    void Function::setInline(AttributeStorage bit) {
        REPLACE_BIT(_attributes, bit, inline_);
    }
    const Function::AttributeStorage Function::useRegAllocOnly() const {
        return _attributes & (1 << inline_);
    }
    void Function::setUseRegAllocOnly(AttributeStorage bit) {
        REPLACE_BIT(_attributes, bit, onlyReg);
    }

const std::optional<std::string>& deprecationWarning();
    FunctionForwardDeclaration::FunctionForwardDeclaration(TextRegion loc, const Token& name, const Function::Parameters& parameters, Type* returnType): Declaration(loc), _name(name), _parameters(parameters), _retType(returnType) {}
    FunctionForwardDeclaration::~FunctionForwardDeclaration() {}
    void FunctionForwardDeclaration::print() const {
        std::cout << "Function forward declaration at loc ";
        _loc.describe(' ');
        std::cout << "named '" << _name.contents << "(";
        for (size_t i {}; i < _parameters.size(); ++i) {
            std::cout << _parameters[i].name.contents << ": ";
            _parameters[i].type->print();
            if (i + 1 != _parameters.size())
                std::cout << ", ";
        }
        std::cout << "): ";
        if (_retType->isVoid() || _retType->isIncomplete())
            std::cout << "Void";
        else
            _retType->print();
        std::cout << "'\n";
    }
    Token& FunctionForwardDeclaration::name() {
        return _name;
    }
    size_t FunctionForwardDeclaration::arity() const {
        return _parameters.size();
    }
    const Type* FunctionForwardDeclaration::returnType() const {
        return _retType;
    }
    const Function::Parameters& FunctionForwardDeclaration::parameters() const {
        return _parameters;
    }
    void FunctionForwardDeclaration::setRType(Type* newType) {
        _retType = newType;
    }
    Initializer::Initializer(InitializerType type): type(type) {}
    Initializer::~Initializer() {}
    ZeroInitializer::ZeroInitializer(): Initializer(InitializerType::zero) {}
    DirectInitializer::DirectInitializer(Expression* expr): Initializer(InitializerType::direct), _expr(expr) {}
    Expression* DirectInitializer::expr() const {
        return _expr;
    }
    CopyInitializer::CopyInitializer(Expression* expr): Initializer(InitializerType::copy), _expr(expr) {}
    Expression* CopyInitializer::expr() const {
        return _expr;
    }
    GlobalDeclaration::GlobalDeclaration(TextRegion loc, const Token& name, Type* type, Initializer* init): Declaration(loc), name(name), type(type), init(init) {}
    GlobalDeclaration::~GlobalDeclaration() {
        //dealloc(type);
        dealloc(init);
    }
    void GlobalDeclaration::print() const {
        std::cout << "Global declaration of '" << name.contents << "' at loc ";
        _loc.describe(' ');
        if (init->type == Initializer::zero) {
            std::cout << "zero-initialized\n";
        } else if (init->type == Initializer::direct) {
            std::cout << "direct-initialized\n";
        } else if (init->type == Initializer::copy) {
            std::cout << "copy-initialized\n";
        }
    }
    bool GlobalDeclaration::isZeroInitialized() const {
        return init->type == Initializer::zero;
    }
    Initializer* GlobalDeclaration::initializer() const {
        return init;
    }
    LetStatement::LetStatement(TextRegion loc, const Token& name, Type* type, Initializer* init): Statement(loc), _name(name), _type(type), init(init) {}
    LetStatement::~LetStatement() {
        dealloc(_type);
        dealloc(init);
    }
    void LetStatement::print() const {
        std::cout << "Let statement of '" << _name.contents << "' at loc ";
        _loc.describe();
    }
    const Initializer* LetStatement::initializer() const {
        return init;
    }
    const Type* LetStatement::type() const {
        return _type;
    }
    void LetStatement::setType(Type* newType) {
        _type = newType;
    }
    const Token& LetStatement::name() const {
        return _name;
    }
    VarStatement::VarStatement(TextRegion loc, const Token& name, Type* type, Initializer* init): Statement(loc), _name(name), _type(type), init(init) {}
    VarStatement::~VarStatement() {
        dealloc(_type);
        dealloc(init);
    }
    void VarStatement::print() const {
        std::cout << "Var statement of '" << _name.contents << "' at loc ";
        _loc.describe();
    }
    const Initializer* VarStatement::initializer() const {
        return init;
    }
    Type* VarStatement::type() {
        return _type;
    }
    void VarStatement::setType(Type* newType) {
        _type = newType;
    }
    const Token& VarStatement::name() const {
        return _name;
    }
    GlobalForwardDeclaration::GlobalForwardDeclaration(TextRegion loc, const Token& name, Type* type): Declaration(loc), _name(name), _type(type) {}
    GlobalForwardDeclaration::~GlobalForwardDeclaration() {
        dealloc(_type);
    }
    const Token& GlobalForwardDeclaration::name() const {
        return _name;
    }
    const Type* GlobalForwardDeclaration::type() const {
        return _type;
    }
    void GlobalForwardDeclaration::print() const {
        std::cout << "Global forward declaration of '" << _name.contents << "' at loc ";
        _loc.describe(' ');
    }

    StructConstructor::StructConstructor(const Function::Parameters& params, const std::vector<std::pair<Token, Expression*>>& inits, Statement* after): params(params), inits(inits), after(after) {}
    StructConstructor::~StructConstructor() {
        for (auto param: params) dealloc(param.type);
        for (auto init: inits) dealloc(init.second);
        dealloc(after);
    }
    StructDeclaration::StructDeclaration(TextRegion loc, const Token& name, const std::vector<Statement*>& dataMembers, const std::vector<Function*>& functionMembers, const std::vector<StructConstructor*>& constructors): Declaration(loc), _name(name), _dataMembers(dataMembers), _functionMembers(functionMembers), _constructors(constructors) {}
    StructDeclaration::~StructDeclaration() {
        for (auto m_data: _dataMembers) dealloc(m_data);
        for (auto m_func: _functionMembers) dealloc(m_func);
        for (auto c: _constructors) dealloc(c);
    }
        
    void StructDeclaration::print() const {
        std::cout << "Struct Declaration at loc ";
        _loc.describe();
    }
    const Token& StructDeclaration::name() const {
        return _name;
    }
    long StructDeclaration::offsetOf(const std::string& memberName) const {
        long offset = 0;
        size_t index = 0;
        while (index < _dataMembers.size()) {
            if (auto var = dynamic_cast<VarStatement*>(_dataMembers[index])) {
                if (var->name().contents == memberName) {
                    break;
                }
                offset -= var->type()->size();
            }
            index++;
        }
        return offset;
    }
    std::vector<Statement*>& StructDeclaration::dataMembers() {
        return _dataMembers;
    }
    std::vector<Function*>& StructDeclaration::functionMembers() {
        return _functionMembers;
    }
    std::vector<StructConstructor*>& StructDeclaration::constructors() {
        return _constructors;
    }
    TypeAliasDeclaration::TypeAliasDeclaration(TextRegion loc, const Token& alias, Type* aliased): Declaration(loc), _alias(alias), _aliased(aliased) {}
    TypeAliasDeclaration::~TypeAliasDeclaration() {
        dealloc(_aliased);
    }
    void TypeAliasDeclaration::print() const {
        std::cout << "Type Alias Declaration at loc ";
        _loc.describe();
    }
    Token& TypeAliasDeclaration::alias() {
        return _alias;
    }
    Type* TypeAliasDeclaration::aliased() const {
        return _aliased;
    }
    NamespaceDeclaration::NamespaceDeclaration(TextRegion loc, const Token& name, const std::vector<Node*>& nodes): Declaration(loc), _name(name), _nodes(nodes) {}
    NamespaceDeclaration::~NamespaceDeclaration() {
        for (auto node: _nodes) dealloc(node);
    }
    void NamespaceDeclaration::print() const {
        std::cout << "Namespace " << _name.contents << " at loc ";
        _loc.describe();
    }
    const Token& NamespaceDeclaration::name() const {
        return _name;
    }
    const std::vector<Node*>& NamespaceDeclaration::nodes() const {
        return _nodes;
    }

    Statement::Statement(TextRegion loc): Node(loc) {}
    void Statement::print() const {
        std::cout << "Statement at loc ";
        _loc.describe();
    }
    CallStatement::CallStatement(TextRegion loc, Call* call): Statement(loc), call(call) {}
    CallStatement::~CallStatement() {
        dealloc(call);
    }
    void CallStatement::print() const {
        std::cout << "Call Statement to " << call->name.contents << " at loc ";
        _loc.describe();
    }
    std::string& CallStatement::name() const {
        return call->name.contents;
    }
    ReturnStatement::ReturnStatement(TextRegion loc, Expression* value): Statement(loc), _value(value) {}
    ReturnStatement::~ReturnStatement() {
        dealloc(_value);
    }
    void ReturnStatement::print() const {
        std::cout << "Return Statement";
        if (_value) {
            std::cout << " with value [" << _value->prettystr() << ']';
        }
        std::cout << " at loc ";
        _loc.describe();
    }
    Expression* ReturnStatement::value() const {
        return _value;
    }
    EmptyStatment::EmptyStatment(TextRegion loc): Statement(loc) {}
    void EmptyStatment::print() const {
        std::cout << "Empty statement at loc ";
        _loc.describe();
    }
    ExpressionStatement::ExpressionStatement(TextRegion loc, Expression* expr): Statement(loc), _expr(expr) {}
    ExpressionStatement::~ExpressionStatement() {
        dealloc(_expr);
    }
    Expression* ExpressionStatement::expr() const {
        return _expr;
    }
    void ExpressionStatement::print() const {
        std::cout << "Expression Statement with value [" << expr()->prettystr() << "] at loc ";
        _loc.describe();
    }
    PointerAssignment::PointerAssignment(TextRegion loc, Expression* ptrExpr, Expression* newValue): Statement(loc), _ptrExpr(ptrExpr), _newValue(newValue) {}
    PointerAssignment::~PointerAssignment() {
        dealloc(_ptrExpr);
        dealloc(_newValue);
    }
    void PointerAssignment::print() const {
        std::cout << "Pointer Assignment Statement assigning [" << _newValue->prettystr() << "] to the value at [" << _ptrExpr->prettystr() << "] at loc ";
        _loc.describe();
    }
    Expression* PointerAssignment::ptrExpr() const {
        return _ptrExpr;
    }
    Expression* PointerAssignment::newValue() const {
        return _newValue;
    }
    void PointerAssignment::makenull() {
        _ptrExpr = nullptr;
        _newValue = nullptr;
    }
    Assignment::Assignment(TextRegion loc, Expression* lval, Expression* rval): Statement(loc), _lval(lval), _rval(rval) {}
    Assignment::~Assignment() {
        dealloc(_lval);
        dealloc(_rval);
    }
    void Assignment::print() const {
        std::cout << "Assignment Statement assigning [" << _rval->prettystr() << "] to [" << _lval->prettystr() << "] at loc ";
        _loc.describe();
    }
    Expression* Assignment::lval() const {
        return _lval;
    }
    Expression* Assignment::rval() const {
        return _rval;
    }
    IfStatement::IfStatement(TextRegion loc, Expression* condition, Block* body): Statement(loc), _condition(condition), _body(body) {}
    IfStatement::~IfStatement() {
        dealloc(_condition);
        dealloc(_body);
    }
    void IfStatement::print() const {
        std::cout << "If Statement at loc ";
        _loc.describe();
    }
    Expression* IfStatement::condition() const {
        return _condition;
    }
    Block* IfStatement::body() const {
        return _body;
    }
    Block::Block(TextRegion loc, const std::vector<Node*>& body): Statement(loc), _body(body) {}
    Block::~Block() {
        for (auto node: _body) dealloc(node);
    }

    void Block::print() const {
        std::cout << "Block at loc ";
        _loc.describe();
    }
    void Block::insert(Node* node) {
        _body.push_back(node);
    }
    const std::vector<Node*>& Block::body() const {
        return _body;
    }
    size_t Block::size() const {
        size_t acc {};
        for (auto node: _body) {
            if (auto let = dynamic_cast<LetStatement*>(node)) {
                acc += let->type()->alignment();
            } else if (auto var = dynamic_cast<VarStatement*>(node)) {
                acc += var->type()->alignment();
            } else if (auto block = dynamic_cast<Block*>(node)) {
                acc += block->size();
            }
        }
        return acc;
    }
    const bool Block::isLeaf() const {
        for (auto node: _body) {
            if (dynamic_cast<CallStatement*>(node)) {
                return false;
            } else if (auto block = dynamic_cast<Block*>(node)) {
                if (!block->isLeaf()) return false;
            }
        }
        return true;
    }
    WhileStatement::WhileStatement(TextRegion loc, Expression* condition, Block* body): Statement(loc), _condition(condition), _body(body) {}
    WhileStatement::~WhileStatement() {
        dealloc(_condition);
        dealloc(_body);
    }
    void WhileStatement::print() const {
        std::cout << "While Statement at loc ";
        _loc.describe();
    }
    Expression* WhileStatement::condition() const {
        return _condition;
    }
    Block* WhileStatement::body() const {
        return _body;
    }
    ForStatement::ForStatement(TextRegion loc, Statement* init, Expression* check, Statement* modify, Block* body): Statement(loc), _init(init), _check(check), _modify(modify), _body(body) {}
    ForStatement::~ForStatement() {
        dealloc(_init);
        dealloc(_check);
        dealloc(_modify);
        dealloc(_body);
    }
    void ForStatement::print() const {
        std::cout << "For Statement at loc ";
        _loc.describe();
    }
    Statement* ForStatement::init() const {
        return _init;
    }
    Expression* ForStatement::check() const {
        return _check;
    }
    Statement* ForStatement::modify() const {
        return _modify;
    }
    Block* ForStatement::body() const {
        return _body;
    }
    size_t ForStatement::size() const {
        size_t acc {};
        if (auto let = dynamic_cast<LetStatement*>(_init)) {
            acc += let->type()->alignment();
        } else if (auto var = dynamic_cast<VarStatement*>(_init)) {
            acc += var->type()->alignment();
        } else if (auto block = dynamic_cast<Block*>(_init)) {
            acc += block->size();
        }
        if (auto let = dynamic_cast<LetStatement*>(_modify)) {
            acc += let->type()->alignment();
        } else if (auto var = dynamic_cast<VarStatement*>(_modify)) {
            acc += var->type()->alignment();
        } else if (auto block = dynamic_cast<Block*>(_modify)) {
            acc += block->size();
        }
        for (auto node: _body->body()) {
            if (auto let = dynamic_cast<LetStatement*>(node)) {
                acc += let->type()->alignment();
            } else if (auto var = dynamic_cast<VarStatement*>(node)) {
                acc += var->type()->alignment();
            } else if (auto block = dynamic_cast<Block*>(node)) {
                acc += block->size();
            }
        }
        return acc;
    }
    Expression::Expression(TextRegion loc): Node(loc), type(nullptr) {}
    void Expression::print() const {
        std::cout << "Expression at loc ";
        _loc.describe();
    }
    Call::Call(TextRegion loc, const Token& name, const std::vector<Expression*>& args): Expression(loc), name(name), args(args) {}
    Call::~Call() {
        for (auto arg: args) {
            dealloc(arg);
        }
    }
    void Call::print() const {
        std::cout << "Call to " << name.contents << " with " << args.size() << " arg(s) at loc ";
        _loc.describe();
    }
    void Call::pretty() const {
        std::cout << prettystr();
    }
    const std::string Call::prettystr() const {
        std::string r = name.contents + '(';
        for (size_t i = 0; i < args.size(); i++) {
            r += args[i]->prettystr();
            if (i + 1 != args.size()) r += ", ";
        }
        r.push_back(')');
        return r;
    }
    const std::string Call::generateTypeDescription() const {
        if (args.empty()) return name.contents + "(Void)";
        std::string result { name.contents + '(' };
        bool nullArg = false;
        for (Expression* expr: args) {
            if (expr) result += expr->type->des() + ", ";
            else nullArg = true;
        }
        if (!nullArg || args.size() > 1) {
            result.pop_back();
            result.pop_back();
        }
        result.push_back(')');
        return result;
    }

    Literal::Literal(TextRegion loc, LType type, const Token& value): Expression(loc), _type(type), _value(value) {}
    void Literal::print() const {
        std::cout << "Literal expression at loc ";
        _loc.describe();
    }
    void Literal::pretty() const {
        std::cout << prettystr();
    }
    const std::string Literal::prettystr() const {
        if (_type == LType::hexadecimalInteger) {
            return "0x" + _value.contents;
        } else if (_type == LType::cString) {
            return '\"' + _value.contents + '\"';
        } else if (_type == LType::wideString) {
            std::string acc = "W\"";
            for (FloralWideChar wchar: _value._wstr) {
                acc += "\\x";
                acc += n2hexstr(wchar);
            }
            acc.push_back('\"');
            return acc;
        }
        else return _value.contents;
    }
    const std::string Literal::description() const {
        if (_type == LType::hexadecimalInteger) {
            return "0x" + _value.contents;
        } else if (_type == LType::cString) {
            return TYPE_STRING_INDICATOR;
        } else if (_type == LType::cString) {
            return TYPE_WSTRING_INDICATOR;
        } else if (_type == LType::boolean) {
            return _value.type == TokenType::boolTrue ? "1" : "0";
        }
        else return _value.contents;
    }
    Literal::LType Literal::type() const {
        return _type;
    }
    const Token& Literal::value() const {
        return _value;
    }
    OperatorComponentExpression::OperatorComponentExpression(const Token& op): Expression({ op, op }), _op(op) {}
    void OperatorComponentExpression::print() const {
        std::cout << _op.contents;
    }
    void OperatorComponentExpression::pretty() const {
        std::cout << _op.contents;
    }
    const std::string OperatorComponentExpression::prettystr() const {
        return _op.contents;
    }
    size_t OperatorComponentExpression::precedence(const OperatorMode mode) const {
        switch (_op.type) {
            case TokenType::equal:
            case TokenType::unequal: return 10;
            case TokenType::less:
            case TokenType::lessEqual:
            case TokenType::greater:
            case TokenType::greaterEqual: return 20;
            case TokenType::plus:
            case TokenType::minus: return mode != infix ? 60 : 40;
            case TokenType::multiply: if (mode == prefix) return 70;
            case TokenType::divide: return 50;
            case TokenType::leftBracket:
            case TokenType::inc:
            case TokenType::dec: return 60;
            case TokenType::dot:
            case TokenType::arrow: return 80;
            default: return 0;
        }
    }
    const TokenType OperatorComponentExpression::tkntype() const {
        return _op.type;
    }
    const Token& OperatorComponentExpression::tkn() const {
        return _op;
    }
    SymbolExpression::SymbolExpression(TextRegion loc, const Token& val): Expression(loc), _val(val) {}
    SymbolExpression::~SymbolExpression() {}
    void SymbolExpression::print() const {
        std::cout << "Symbol expression at loc ", _loc.describe();
    }
    void SymbolExpression::pretty() const {
        std::cout << _val.contents;
    }
    const std::string SymbolExpression::prettystr() const {
        return _val.contents;
    }
    const Token& SymbolExpression::value() const {
        return _val;
    }
    BinaryExpression::BinaryExpression(TextRegion loc, Expression* left, OperatorComponentExpression* op, Expression* right): Expression(loc), _left(left), _op(op), _right(right) {}
    BinaryExpression::~BinaryExpression() {
        dealloc(_left);
        dealloc(_right);
        //dealloc(type); // MARK: Please fix this somehow
    }
    void BinaryExpression::print() const {
        std::cout << "Binary Expression at loc ";
        _loc.describe();
    }
    void BinaryExpression::pretty() const {
        std::cout << prettystr();
    }
    const std::string BinaryExpression::prettystr() const {
        std::string r;
        if (_left) r += _left->prettystr();
        if (_op) {
            if (_left) r.push_back(' ');
            r += _op->prettystr();
            if (!_right || _left) r.push_back(' ');
        }
        if (_right) r += _right->prettystr();
        return r;
    }
    Expression* BinaryExpression::left() const {
        return _left;
    }
    OperatorComponentExpression* BinaryExpression::op() const {
        return _op;
    }
    Expression* BinaryExpression::right() const {
        return _right;
    }
    SizeOfType::SizeOfType(TextRegion loc, Type* type): Expression(loc), _type(type) {}
    SizeOfType::~SizeOfType() {
        dealloc(_type);
    }
    void SizeOfType::print() const {
        std::cout << "SizeOf Expression at loc ";
        _loc.describe();
    }
    void SizeOfType::pretty() const {
        std::cout << prettystr();
    }
    const std::string SizeOfType::prettystr() const {
        return "sizeof(" + _type->des() + ')';
    }
    size_t SizeOfType::size() const {
        return _type->size();
    }
    Type* SizeOfType::type() const {
        return _type;
    }
    UnsafeCast::UnsafeCast(TextRegion loc, Type* type, Expression* expr): Expression(loc), _type(type), _expr(expr) {}
    UnsafeCast::~UnsafeCast() {
        dealloc(_type);
        dealloc(_expr);
    }
    void UnsafeCast::print() const {
        std::cout << "Unsafe Cast at loc ";
        _loc.describe();
    }
    void UnsafeCast::pretty() const {
        std::cout << prettystr();
    }
    const std::string UnsafeCast::prettystr() const {
        return "unsafe_cast<" + _type->des() + ">(" + _expr->prettystr() + ')';
    }
    Type* UnsafeCast::type() const {
        return _type;
    }
    Expression* UnsafeCast::expr() const {
        return _expr;
    }
    ConstructExpression::ConstructExpression(TextRegion loc, const Token& name, const std::vector<Expression*>& args, Mode mode): Expression(loc), _name(name), _args(args), _mode(mode) {
        _struct = new Type(0, name.contents);
    }
    ConstructExpression::~ConstructExpression() {
        for (auto arg: _args) delete arg;
        delete _struct;
    }
    void ConstructExpression::print() const {
        std::cout << "Unsafe Cast at loc ";
        _loc.describe();
    }
    void ConstructExpression::pretty() const {
        std::cout << prettystr();
    }
    const std::string ConstructExpression::prettystr() const {
        switch (_mode) {
            case Mode::stack:
                return _name.contents;
            case Mode::heap: {
                return "new " + _name.contents;
            }
        }
    }
    const Token& ConstructExpression::name() const {
        return _name;
    }
    const std::vector<Expression*>& ConstructExpression::args() const {
        return _args;
    }
    const ConstructExpression::Mode ConstructExpression::mode() const {
        return _mode;
    }
    Type* ConstructExpression::type() const {
        return _struct;
    }
    ArrayLiteralExpression::ArrayLiteralExpression(TextRegion loc, const std::vector<Expression*>& values): Expression(loc), _values(values) {}
    ArrayLiteralExpression::~ArrayLiteralExpression() {
        for (auto expr: _values) dealloc(expr);
    }

    void ArrayLiteralExpression::print() const {
        std::cout << "Array Literal Expression at loc ";
        _loc.describe();
    }
void ArrayLiteralExpression::pretty() const {
    std::cout << prettystr();
}
const std::string ArrayLiteralExpression::prettystr() const {
    std::string acc;
    acc.push_back('[');
    for (auto expr: _values) {
        acc += expr->prettystr();
        acc += ", ";
    }
    if (!_values.empty()) {
        acc.pop_back();
        acc.pop_back();
    }
    acc.push_back(']');
    return acc;
}
    const std::vector<Expression*>& ArrayLiteralExpression::values() const {
        return _values;
    }
}
