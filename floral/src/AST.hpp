//
//  AST.hpp
//  floral
//
//  Created by Ethan Uppal on 6/29/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef AST_hpp
#define AST_hpp

#include <cstddef>
#include "Token.hpp"
#include <vector>
#include "Type.hpp"
#include "Scope.hpp"

namespace Floral {
    struct TextRegion {
        size_t pos;
        size_t length;
        size_t startLine;
        size_t endLine;
        
        TextRegion(const Token& token);
        TextRegion(const Token& first, const Token& last);
        TextRegion(size_t pos, size_t length, size_t startLine, size_t endLine);
        
        void describe(char term) const;
    };
    struct Node {
        TextRegion _loc;
        Scope scope;
        
    protected:
        Node(TextRegion loc);
        
    public:
        virtual void print();
    };
    class Function;
    class File: public Node {
        std::string _path;
        std::vector<Node*> _nodes;
        Function *_main;
        
    public:
        File(TextRegion loc, const std::string &path, const std::vector<Node*> &nodes);
        ~File();
        
        virtual void print() override;
        void insert(Node *node);
        void setPath(const std::string &path);
        const std::vector<Node*>& nodes() const;
        Function* main() const;
        void dump() const;
    };
    struct Declaration: public Node {
        Declaration(TextRegion loc);
        
        virtual void print() override;
    };
    struct Statement: public Node {
        Statement(TextRegion loc);
        
        virtual void print() override;
    };
    struct Expression: public Node {
        Expression(TextRegion loc);
        
        virtual void print() override;
    };
    class Function: public Declaration {
    public:
        struct Parameter {
            Token name;
            Type* type;
        };
        typedef std::vector<Parameter> Parameters;
        
    private:
        Token _name;
        Parameters _parameters;
        std::vector<Statement*> _body;
        Type* _retType;

    public:
        Function(TextRegion loc, const Token& name, const Parameters& parameters, Type* returnType);
        ~Function();
        
        virtual void print() override;
        void setBody(const std::vector<Statement*> &body);
        const std::vector<Statement*>& body() const;
        const Token& name() const;
        size_t arity() const;
        const Type* returnType() const;
    };
    class Call: public Expression {
    public:
        Token name;
        std::vector<Expression*> args;
        Call(TextRegion loc, const Token& name, const std::vector<Expression*>& args);
        ~Call();
        
        virtual void print() override;
    };
    class CallStatement: public Statement {
    public:
        Call *call;
        CallStatement(TextRegion loc, Call* call);
        ~CallStatement();
        
        virtual void print() override;
        std::string& name() const;
    };
    class ReturnStatement: public Statement {
        Expression* _value;
        
    public:
        ReturnStatement(TextRegion loc, Expression* value);
        ~ReturnStatement();
        
        virtual void print() override;
        Expression* value() const;
    };
    class EmptyStatment: public Statement {
    public:
        EmptyStatment(TextRegion loc);
        
        virtual void print() override;
    };
    class Literal: public Expression {
    public:
        enum class LType {
            boolean, decimalInteger, hexadecimalInteger, floatingPointNumber, simpleString
        };
        
    private:
        LType _type;
        Token _value;
        
    public:
        Literal(TextRegion loc, LType type, const Token& value);
        
        virtual void print() override;
        LType type() const;
        const Token& value() const;
    };
    class LiteralStatement: public Statement {
        Literal* _lit;
        
    public:
        LiteralStatement(TextRegion loc, Literal* lit);
        ~LiteralStatement();
        
        virtual void print() override;
    };
    class FlatExpression: public Expression {
        std::vector<Expression*> _components;
        
    public:
        FlatExpression(TextRegion loc, std::vector<Expression*> components);
        ~FlatExpression();
        
        virtual void print() override;
        const std::vector<Expression*>& components() const;
    };
    class OperatorComponentExpression: public Expression {
        Token _op;
    public:
        OperatorComponentExpression(const Token& op);
        
        virtual void print() override;
    };
    class GlobalDeclaration: public Declaration {
        Token name;
        Type type;
    };
    class LetDeclaration: public Declaration {
        
    };
    class VarDeclaration: public Declaration {
        
    };
}

#endif /* AST_hpp */
