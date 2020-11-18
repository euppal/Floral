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
#include <optional>

namespace Floral {
    class Compiler;

    struct TextRegion {
        friend struct TokenLoc;
        
        size_t pos;
        size_t length;
        size_t startLine;
        size_t endLine;
        const std::string path;
        
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
        virtual const std::string prettystr() const = 0;
        
        Type* type;
    };
    struct Function: public Declaration {
        friend class Compiler;

        struct Parameter {
            Parameter(const Token& name, Type* type);
            
            Token name;
            Type* type;
        };
        typedef std::vector<Parameter> Parameters;
        
        size_t staticAllocationSize{};
        void setRType(Type* newType);
        
        enum Attributes {
            static_ = 0,
            inline_,
            onlyReg
        };
        
        #define __cat(x, y) x ## y
        #define _cat(x, y) __cat(x, y)
        #define BITSET(n) _cat(_cat(uint, n), _t)
        typedef BITSET(8) AttributeStorage;
        #undef __cat
        #undef _cat
        #undef BITSET

    private:
        Token _name;
        Parameters _parameters;
        std::vector<Statement*> _body;
        Type* _retType;
        AttributeStorage _attributes {};
        std::optional<std::string> _depr;

    public:
        Function(TextRegion loc, const Token& name, const Parameters& parameters, Type* returnType);
        ~Function();
        
        virtual void print() const override;
        void insert(Statement* stm);
        const std::vector<Statement*>& body() const;
        Token& name();
        size_t arity() const;
        const Type* returnType() const;
        const Parameters& parameters() const;
        const bool isLeaf() const;
        
        const AttributeStorage isStatic() const;
        void setStatic(AttributeStorage bit);
        const AttributeStorage isInline() const;
        void setInline(AttributeStorage bit);
        const AttributeStorage useRegAllocOnly() const;
        void setUseRegAllocOnly(AttributeStorage bit);

        const std::optional<std::string>& deprecationWarning();
    };
    struct FunctionForwardDeclaration: public Declaration {
        friend class Compiler;
        void setRType(Type* newType);

        FunctionForwardDeclaration(TextRegion loc, const Token& name, const Function::Parameters& parameters, Type* returnType);
        ~FunctionForwardDeclaration();
        
        virtual void print() const override;
        Token& name();
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
        const std::string prettystr() const override;
        const std::string generateTypeDescription() const;
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
        enum OperatorMode {
            infix, prefix, suffix
        };
        
        OperatorComponentExpression(const Token& op);
        
        virtual void print() const override;
        virtual void pretty() const override;
        const std::string prettystr() const override;

        size_t precedence(const OperatorMode mode) const;
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
        const std::string prettystr() const override;

        Expression* left() const;
        OperatorComponentExpression* op() const;
        Expression* right() const;
        bool isPlainExpression() const;
    };
    #define TYPE_STRING_INDICATOR "FLORAL_TYPE_ASCII_STRING_INDICATOR"
    #define TYPE_WSTRING_INDICATOR "FLORAL_TYPE_WIDE_STRING_INDICATOR"
    class Literal: public Expression {
    public:
        enum class LType {
            boolean,
            decimalInteger, decimalUInteger,
            decimalByte, decimalUByte,
            decimalWideChar, decimalWideUChar,
            decimalShort, decimalUShort,
            decimalInt32, decimalUInt32,
            hexadecimalInteger, floatingPointNumber,
            cString, wideString
        };
        
    private:
        LType _type;
        Token _value;
        
    public:
        Literal(TextRegion loc, LType type, const Token& value);
        
        virtual void print() const override;
        virtual void pretty() const override;
        const std::string prettystr() const override;
        const std::string description() const;
        LType type() const;
        const Token& value() const;
    };
    class ExpressionStatement: public Statement {
        Expression* _expr;
        
    public:
        ExpressionStatement(TextRegion loc, Expression* expr);
        ~ExpressionStatement();
        
        virtual void print() const override;
        Expression* expr() const;
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
        Token name;
        Type* type;

        GlobalDeclaration(TextRegion loc, const Token& name, Type* type, Initializer* init);
        ~GlobalDeclaration();
        
        virtual void print() const override;
        bool isZeroInitialized() const;
        Initializer* initializer() const;
    };
    class LetStatement: public Statement {
        Token _name;
        Type* _type;
        Initializer* init;

    public:
        LetStatement(TextRegion loc, const Token& name, Type* type, Initializer* init);
        ~LetStatement();
        
        virtual void print() const override;
        const Initializer* initializer() const;
        const Type* type() const;
        void setType(Type* newType);
        const Token& name() const;
    };
    class VarStatement: public Statement {
        Token _name;
        Type* _type;
        Initializer* init;

    public:
        VarStatement(TextRegion loc, const Token& name, Type* type, Initializer* init);
        ~VarStatement();
        
        virtual void print() const override;
        const Initializer* initializer() const;
        Type* type();
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
        const std::string prettystr() const override;

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
    class PointerAssignment: public Statement {
        Expression* _ptrExpr;
        Expression* _newValue;
        
    public:
        PointerAssignment(TextRegion loc, Expression* ptrExpr, Expression* newValue);
        ~PointerAssignment();
        virtual void print() const override;
        void makenull();
        Expression* ptrExpr() const;
        Expression* newValue() const;
    };
    class SizeOfType: public Expression {
        Type* _type;
            
    public:
        SizeOfType(TextRegion loc, Type* type);
        ~SizeOfType();
        
        virtual void print() const override;
        virtual void pretty() const override;
        const std::string prettystr() const override;
        size_t size() const;
        Type* type() const;
    };
    class UnsafeCast: public Expression {
        Type* _type;
        Expression* _expr;
    public:
        UnsafeCast(TextRegion loc, Type* type, Expression* expr);
        ~UnsafeCast();
        
        virtual void print() const override;
        virtual void pretty() const override;
        const std::string prettystr() const override;
        Type* type() const;
        Expression* expr() const;
    };
    class Assignment: public Statement {
        Expression* _lval;
        Expression* _rval;
        
    public:
        Assignment(TextRegion loc, Expression* lval, Expression* rval);
        ~Assignment();
        virtual void print() const override;
        Expression* lval() const;
        Expression* rval() const;
    };
    class Block: public Statement {
        std::vector<Node*> _body;

    public:
        Block(TextRegion loc, const std::vector<Node*>& body);
        ~Block();
        
        virtual void print() const override;
        void insert(Node* node);
        const std::vector<Node*>& body() const;
        size_t size() const;
        const bool isLeaf() const;
    };
    class IfStatement: public Statement {
        Expression* _condition;
        Block* _body;
        
    public:
        IfStatement(TextRegion loc, Expression* condition, Block* body);
        ~IfStatement();
        
        virtual void print() const override;
        Expression* condition() const;
        Block* body() const;
    };
    class WhileStatement: public Statement {
        Expression* _condition;
        Block* _body;
        
    public:
        WhileStatement(TextRegion loc, Expression* condition, Block* body);
        ~WhileStatement();
        
        virtual void print() const override;
        Expression* condition() const;
        Block* body() const;
    };
    class ForStatement: public Statement {
        Statement* _init;
        Expression* _check;
        Statement* _modify;
        Block* _body;
        
    public:
        ForStatement(TextRegion loc, Statement* init, Expression* condition, Statement* modify, Block* body);
        ~ForStatement();
        
        virtual void print() const override;
        Statement* init() const;
        Expression* check() const;
        Statement* modify() const;
        Block* body() const;
        size_t size() const;
    };
    struct StructConstructor {
        StructConstructor(const Function::Parameters& params, const std::vector<std::pair<Token, Expression*>>& inits, Statement* after);
        ~StructConstructor();
        
        Function::Parameters params;
        std::vector<std::pair<Token, Expression*>> inits;
        Statement* after;
    };
    class StructDeclaration: public Declaration {
        Token _name;
        std::vector<Statement*> _dataMembers;
        std::vector<Function*> _functionMembers;
        std::vector<StructConstructor*> _constructors;
        
    public:
        StructDeclaration(TextRegion loc,const Token& name, const std::vector<Statement*>& dataMembers, const std::vector<Function*>& functionMembers, const std::vector<StructConstructor*>& constructors);
        ~StructDeclaration();
        
        virtual void print() const override;
        const Token& name() const;
        long offsetOf(const std::string& memberName) const;
        std::vector<Statement*>& dataMembers();
        std::vector<Function*>& functionMembers();
        std::vector<StructConstructor*>& constructors();
    };
    class TypeAliasDeclaration: public Declaration {
        Token _alias;
        Type* _aliased;
        
    public:
        TypeAliasDeclaration(TextRegion loc, const Token& alias, Type* aliased);
        ~TypeAliasDeclaration();
        
        virtual void print() const override;
        Token& alias();
        Type* aliased() const;
    };
    struct ConstructExpression: public Expression {
        enum class Mode {
            stack, heap
        };
        
    private:
        Token _name;
        std::vector<Expression*> _args;
        Mode _mode;
        Type* _struct;

    public:
        ConstructExpression(TextRegion loc, const Token& name, const std::vector<Expression*>& args, Mode mode);
        ~ConstructExpression();
        
        virtual void print() const override;
        virtual void pretty() const override;
        const std::string prettystr() const override;
        const Token& name() const;
        const std::vector<Expression*>& args() const;
        const Mode mode() const;
        Type* type() const;
        
        StructConstructor* sc;
    };
    class ArrayLiteralExpression: public Expression {
        std::vector<Expression*> _values;
        
    public:
        ArrayLiteralExpression(TextRegion loc, const std::vector<Expression*>& values);
        ~ArrayLiteralExpression();
        
        virtual void print() const override;
        virtual void pretty() const override;
        const std::string prettystr() const override;
        const std::vector<Expression*>& values() const;
    };
    class NamespaceDeclaration: public Declaration {
        Token _name;
        std::vector<Node*> _nodes;
        
    public:
        NamespaceDeclaration(TextRegion loc, const Token& name, const std::vector<Node*>& nodes);
        ~NamespaceDeclaration();
        
        virtual void print() const override;
        const Token& name() const;
        const std::vector<Node*>& nodes() const;
    };

    // https://stackoverflow.com/a/33447587
    template <typename I> std::string n2hexstr(I w, size_t hex_len = sizeof(I)<<1) {
        static const char* digits = "0123456789ABCDEF";
        std::string rc(hex_len,'0');
        for (size_t i=0, j=(hex_len-1)*4 ; i<hex_len; ++i,j-=4)
            rc[i] = digits[(w>>j) & 0x0f];
        return rc;
    }
}

#endif /* AST_hpp */
