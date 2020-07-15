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
    void Node::print() {
        std::cout << "Node at loc";
        _loc.describe();
    };

    File::File(TextRegion loc, const std::string &path, const std::vector<Node*> &nodes): Node(loc), _path(path), _nodes(nodes) {}
    File::~File() {
        for (auto node: _nodes) {
            delete node;
        }
    }
    void File::print() {
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
        std::cout << "- ";
        _main->print();
        for (auto stm: _main->body()) {
            std::cout << "  - ";
            stm->print();
        }
    }

    Declaration::Declaration(TextRegion loc): Node(loc) {}
    void Declaration::print() {
        std::cout << "Declaration at loc ";
        _loc.describe();
    }
    Function::Function(TextRegion loc, const Token& name, const Parameters& parameters, Type* returnType):
    Declaration(loc), _name(name), _parameters(parameters), _retType(returnType) {}
    Function::~Function() {
        for (auto stm: _body)
            delete stm;
        for (auto p: _parameters)
            delete p.type;
        delete _retType;
    }
    void Function::print() {
        std::cout << "Function at loc ";
        _loc.describe(' ');
        std::cout << "named " << _name.contents << "(";
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
        std::cout << '\n';
    }
    void Function::setBody(const std::vector<Statement*> &body) {
        _body = body;
    }
    const std::vector<Statement*>& Function::body() const {
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


    Statement::Statement(TextRegion loc): Node(loc) {}
    void Statement::print() {
        std::cout << "Statement at loc ";
        _loc.describe();
    }
    CallStatement::CallStatement(TextRegion loc, Call* call): Statement(loc), call(call) {}
    CallStatement::~CallStatement() {
        delete call;
    }
    void CallStatement::print() {
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
    void ReturnStatement::print() {
        std::cout << "Return Statement at loc ";
        _loc.describe();
    }
    Expression* ReturnStatement::value() const {
        return _value;
    }
    EmptyStatment::EmptyStatment(TextRegion loc): Statement(loc) {}
    void EmptyStatment::print() {
        std::cout << "Empty statement at loc ";
        _loc.describe();
    }
    LiteralStatement::LiteralStatement(TextRegion loc, Literal* lit): Statement(loc), _lit(lit) {}
    LiteralStatement::~LiteralStatement() {
        delete _lit;
    }
    void LiteralStatement::print() {
        std::cout << "Literal Statement at loc ";
        _loc.describe();
    }

    Expression::Expression(TextRegion loc): Node(loc) {}
    void Expression::print() {
        std::cout << "Expression at loc ";
        _loc.describe();
    }
    Call::Call(TextRegion loc, const Token& name, const std::vector<Expression*>& args): Expression(loc), name(name), args(args) {}
    Call::~Call() {
        for (auto arg: args) {
            delete arg;
        }
    }
    void Call::print() {
        std::cout << "Call to " << name.contents << " with " << args.size() << " arg(s) at loc ";
        _loc.describe();
    }
    Literal::Literal(TextRegion loc, LType type, const Token& value): Expression(loc), _type(type), _value(value) {}
    void Literal::print() {
        std::cout << "Literal " + _value.contents + " at loc ";
        _loc.describe();
    }
    Literal::LType Literal::type() const {
        return _type;
    }
    const Token& Literal::value() const {
        return _value;
    }
    FlatExpression::FlatExpression(TextRegion loc, std::vector<Expression*> components): Expression(loc), _components(components) {}
    FlatExpression::~FlatExpression() {
        for (auto component: _components)
            delete component;
    }
    void FlatExpression::print() {
        std::cout << "Flat Expression [" << _components.size() << " components] at loc ";
        _loc.describe();
    }
    const std::vector<Expression*>& FlatExpression::components() const {
        return _components;
    }
    OperatorComponentExpression::OperatorComponentExpression(const Token& op): Expression({ op, op }), _op(op) {}
    void OperatorComponentExpression::print() {
        std::cout << "Operator Component Expression (" << _op.contents << ") at loc ";
        _loc.describe();
    }
}
