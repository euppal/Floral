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

namespace Floral {
    TextRegion::TextRegion(const Token& value) {
        pos = value.pos();
        length = value.contents.size();
        startLine = value.line();
        endLine = value.line();
    }
    TextRegion::TextRegion(const Token& first, const Token& last) {
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
            delete node;
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
        std::cout << "File Constituents:\n";
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
            delete stm;
        for (auto p: _parameters)
            delete p.type;
        delete _retType;
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
    void Function::insert(Node* node) {
        _body.push_back(node);
    }
    const std::vector<Node*>& Function::body() const {
        return _body;
    }
    const Token& Function::name() const {
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
    const Token& FunctionForwardDeclaration::name() const {
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
        delete type;
        delete init;
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
    const Initializer* GlobalDeclaration::initializer() const {
        return init;
    }
    LetDeclaration::LetDeclaration(TextRegion loc, const Token& name, Type* type, Initializer* init): Declaration(loc), _name(name), _type(type), init(init) {}
    LetDeclaration::~LetDeclaration() {
        delete _type;
        delete init;
    }
    void LetDeclaration::print() const {
        std::cout << "Let declaration of '" << _name.contents << "' at loc ";
        _loc.describe();
    }
    const Initializer* LetDeclaration::initializer() const {
        return init;
    }
    const Type* LetDeclaration::type() const {
        return _type;
    }
    void LetDeclaration::setType(Type* newType) {
        _type = newType;
    }
    const Token& LetDeclaration::name() const {
        return _name;
    }
    VarDeclaration::VarDeclaration(TextRegion loc, const Token& name, Type* type, Initializer* init): Declaration(loc), _name(name), _type(type), init(init) {}
    VarDeclaration::~VarDeclaration() {
        delete _type;
        delete init;
    }
    void VarDeclaration::print() const {
        std::cout << "Var declaration of '" << _name.contents << "' at loc ";
        _loc.describe();
    }
    const Initializer* VarDeclaration::initializer() const {
        return init;
    }
    const Type* VarDeclaration::type() const {
        return _type;
    }
    void VarDeclaration::setType(Type* newType) {
        _type = newType;
    }
    const Token& VarDeclaration::name() const {
        return _name;
    }
    GlobalForwardDeclaration::GlobalForwardDeclaration(TextRegion loc, const Token& name, Type* type): Declaration(loc), _name(name), _type(type) {}
    GlobalForwardDeclaration::~GlobalForwardDeclaration() {
        delete _type;
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

    Statement::Statement(TextRegion loc): Node(loc) {}
    void Statement::print() const {
        std::cout << "Statement at loc ";
        _loc.describe();
    }
    CallStatement::CallStatement(TextRegion loc, Call* call): Statement(loc), call(call) {}
    CallStatement::~CallStatement() {
        delete call;
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
        delete _value;
    }
    void ReturnStatement::print() const {
        std::cout << "Return Statement with value [";
        if(_value) _value->pretty();
        std::cout << "] at loc ";
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
    LiteralStatement::LiteralStatement(TextRegion loc, Literal* lit): Statement(loc), _lit(lit) {}
    LiteralStatement::~LiteralStatement() {
        delete _lit;
    }
    void LiteralStatement::print() const {
        std::cout << "Literal Statement at loc ";
        _loc.describe();
    }

    Expression::Expression(TextRegion loc): Node(loc) {}
    void Expression::print() const {
        std::cout << "Expression at loc ";
        _loc.describe();
    }
    Call::Call(TextRegion loc, const Token& name, const std::vector<Expression*>& args): Expression(loc), name(name), args(args) {}
    Call::~Call() {
        for (auto arg: args) {
            delete arg;
        }
    }
    void Call::print() const {
        std::cout << "Call to " << name.contents << " with " << args.size() << " arg(s) at loc ";
        _loc.describe();
    }
    void Call::pretty() const {
        std::cout << name.contents << '(';
        for (size_t i = 0; i < args.size(); i++) {
            args[i]->pretty();
            if (i + 1 != args.size()) std::cout << ", ";
        }
        std::cout << ')';
    }
    Literal::Literal(TextRegion loc, LType type, const Token& value): Expression(loc), _type(type), _value(value) {}
    void Literal::print() const {
        std::cout << "Literal expression at loc ";
        _loc.describe();
    }
    void Literal::pretty() const {
        if (_type == LType::hexadecimalInteger) {
            std::cout << "0x" << _value.contents;
        } else if (_type == LType::simpleString) {
            std::cout << '\"' << _value.contents << '\"';
        }
        else std::cout << _value.contents;
    }
    const std::string Literal::description() const {
        if (_type == LType::hexadecimalInteger) {
            return "0x" + _value.contents;
        } else if (_type == LType::simpleString) {
            return TYPE_STRING_INDICATOR;
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
    size_t OperatorComponentExpression::precedence() const {
        switch (_op.type) {
            case TokenType::plus:
            case TokenType::minus: return 20;
            case TokenType::multiply:
            case TokenType::divide: return 30;
            default:
                return 0;
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
    const Token& SymbolExpression::value() const {
        return _val;
    }
    BinaryExpression::BinaryExpression(TextRegion loc, Expression* left, OperatorComponentExpression* op, Expression* right): Expression(loc), _left(left), _op(op), _right(right) {}
    BinaryExpression::~BinaryExpression() {
        delete _left;
        delete _right;
        delete type;
    }
    void BinaryExpression::print() const {
        std::cout << "Binary Expression at loc ";
        _loc.describe();
    }
    void BinaryExpression::pretty() const {
        if (_left) _left->pretty();
        if (_op) {
            if (_left) fputc(' ', stdout);
            _op->pretty();
            if (!_right || _left) fputc(' ', stdout);
        }
        if (_right) _right->pretty();
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
}
