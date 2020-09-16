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
    namespace v2 {
        class Compiler;
    }

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
    struct StaticAnalysisResult {
        bool isStaticEval {};
    };
    struct Node {
        TextRegion _loc;
        Scope scope;
        
        virtual ~Node();
        
    protected:
        Node(TextRegion loc);
        
    public:
        virtual void print() const = 0;
        StaticAnalysisResult info;
    };
    class Function;
    class File: public Node {
        std::string _path;
        std::vector<Node*> _nodes;
        Function* _main;
        
    public:
        File(TextRegion loc, const std::string &path, const std::vector<Node*> &nodes);
        ~File();
        
        virtual void print() const override;
        const std::string& path() const;
        void insert(Node *node);
        void setPath(const std::string &path);
        const std::vector<Node*>& nodes() const;
        Function* main() const;
        void dump() const;
    };
    struct Declaration: public Node {
        Declaration(TextRegion loc);
        
        virtual void print() const override;
    };
    struct Statement: public Node {
        Statement(TextRegion loc);
        
        virtual void print() const override;
    };
    struct Expression: public Node {
        Expression(TextRegion loc);
        
        virtual void print() const override;
        virtual void pretty() const = 0;
        
        Type* type;
    };
    struct Function: public Declaration {
        friend class v2::Compiler;

        struct Parameter {
            Parameter(const Token& name, Type* type);
            
            Token name;
            Type* type;
        };
        typedef std::vector<Parameter> Parameters;
        
        size_t staticAllocationSize{};
        void setRType(Type* newType);
        
    private:
        Token _name;
        Parameters _parameters;
        std::vector<Node*> _body;
        Type* _retType;

    public:
        Function(TextRegion loc, const Token& name, const Parameters& parameters, Type* returnType);
        ~Function();
        
        virtual void print() const override;
        void insert(Node* node);
        const std::vector<Node*>& body() const;
        const Token& name() const;
        size_t arity() const;
        const Type* returnType() const;
        const Parameters& parameters() const;
    };
    struct FunctionForwardDeclaration: public Declaration {
        friend class v2::Compiler;
        void setRType(Type* newType);

        FunctionForwardDeclaration(TextRegion loc, const Token& name, const Function::Parameters& parameters, Type* returnType);
        ~FunctionForwardDeclaration();
        
        virtual void print() const override;
        const Token& name() const;
        size_t arity() const;
        const Type* returnType() const;
        const Function::Parameters& parameters() const;
        
    private:
        Token _name;
        Function::Parameters _parameters;
        Type* _retType;

    };
    class Call: public Expression {
    public:
        Token name;
        std::vector<Expression*> args;
        Function::Parameters _spa_params;
        Call(TextRegion loc, const Token& name, const std::vector<Expression*>& args);
        ~Call();
        
        virtual void print() const override;
        virtual void pretty() const override;
    };
    class CallStatement: public Statement {
    public:
        Call *call;
        CallStatement(TextRegion loc, Call* call);
        ~CallStatement();
        
        virtual void print() const override;
        std::string& name() const;
    };
    class BinaryExpression;
    class ReturnStatement: public Statement {
        Expression* _value;
        
    public:
        ReturnStatement(TextRegion loc, Expression* value);
        ~ReturnStatement();
        
        virtual void print() const override;
        Expression* value() const;
    };
    class EmptyStatment: public Statement {
    public:
        EmptyStatment(TextRegion loc);
        
        virtual void print() const override;
    };
    class OperatorComponentExpression: public Expression {
        Token _op;
    public:
        OperatorComponentExpression(const Token& op);
        
        virtual void print() const override;
        virtual void pretty() const override;
        size_t precedence() const;
        const Token& tkn() const;
        const TokenType tkntype() const;
    };
    class BinaryExpression: public Expression {
        Expression* _left;
        OperatorComponentExpression* _op;
        Expression* _right;
        
    public:
        BinaryExpression(TextRegion loc, Expression* left, OperatorComponentExpression* op, Expression* right);
        ~BinaryExpression();
        
        virtual void print() const override;
        virtual void pretty() const override;
        Expression* left() const;
        OperatorComponentExpression* op() const;
        Expression* right() const;
        bool isPlainExpression() const;
    };
    #define TYPE_STRING_INDICATOR "FLORAL_TYPE_STRING_INDICATOR"
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
        
        virtual void print() const override;
        virtual void pretty() const override;
        const std::string description() const;
        LType type() const;
        const Token& value() const;
    };
    class LiteralStatement: public Statement {
        Literal* _lit;
        
    public:
        LiteralStatement(TextRegion loc, Literal* lit);
        ~LiteralStatement();
        
        virtual void print() const override;
    };
    struct Initializer {
        enum InitializerType {
            zero, direct, copy
        };
        const InitializerType type;
        
        virtual ~Initializer();
        
    protected:
        Initializer(const InitializerType type);
    };
    class ZeroInitializer: public Initializer {
    public:
        ZeroInitializer();
    };
    class DirectInitializer: public Initializer {
        Expression* _expr;
        
    public:
        DirectInitializer(Expression* expr);
        Expression* expr() const;
    };
    class CopyInitializer: public Initializer {
        Expression* _expr;
        
    public:
        CopyInitializer(Expression* expr);
        Expression* expr() const;
    };
    class GlobalDeclaration: public Declaration {
        Initializer* init;

    public:
        const Token name;
        Type* type;

        GlobalDeclaration(TextRegion loc, const Token& name, Type* type, Initializer* init);
        ~GlobalDeclaration();
        
        virtual void print() const override;
        bool isZeroInitialized() const;
        const Initializer* initializer() const;
    };
    class LetDeclaration: public Declaration {
        Token _name;
        Type* _type;
        Initializer* init;

    public:
        LetDeclaration(TextRegion loc, const Token& name, Type* type, Initializer* init);
        ~LetDeclaration();
        
        virtual void print() const override;
        const Initializer* initializer() const;
        const Type* type() const;
        void setType(Type* newType);
        const Token& name() const;
    };
    class VarDeclaration: public Declaration {
        Token _name;
        Type* _type;
        Initializer* init;

    public:
        VarDeclaration(TextRegion loc, const Token& name, Type* type, Initializer* init);
        ~VarDeclaration();
        
        virtual void print() const override;
        const Initializer* initializer() const;
        const Type* type() const;
        void setType(Type* newType);
        const Token& name() const;
    };
    class SymbolExpression: public Expression {
        Token _val;
            
    public:
        SymbolExpression(TextRegion loc, const Token& val);
        ~SymbolExpression();
            
        virtual void print() const override;
        virtual void pretty() const override;
        const Token& value() const;
    };
    class GlobalForwardDeclaration: public Declaration {
        Token _name;
        Type* _type;
        
    public:
        GlobalForwardDeclaration(TextRegion loc, const Token& name, Type* type);
        ~GlobalForwardDeclaration();
            
        virtual void print() const override;
        const Token& name() const;
        const Type* type() const;
    };
}

#endif /* AST_hpp */
