//
//  Parser.hpp
//  floral
//
//  Created by Ethan Uppal on 6/29/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Parser_hpp
#define Parser_hpp

#include <cstddef>
#include <vector>
#include <string>
#include "Token.hpp"
#include "AST.hpp"
#include "Error.hpp"

namespace Floral {
    enum class Use {
        syscalls, C
    };
    class Parser: public ErrorReporting {
        std::vector<Error> _errors;
        void report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "");
        
        private:
        std::vector<Token> tokens;
        size_t index;
        
        Token current();
        Token peek();
        void advance();
        bool eof();
        Token match(TokenType type, const std::string& withinCtx = "", const std::string& fix = "");
        void synchronize();
        
        Type* type();
        Initializer* initializer();
        Declaration* function();
        Declaration* bodydecl();
        Declaration* global();
        LetDeclaration* let();
        VarDeclaration* var();
        Statement* statement();
        CallStatement* callStm();
        EmptyStatment* emptyStm();
        LiteralStatement* literalStm();
        ReturnStatement* returnStm();
        Expression* expr();
        BinaryExpression* binaryexpr(Expression* lhs, OperatorComponentExpression* op);
        Expression* primaryexpr();
        OperatorComponentExpression* op();
        Call* callexpr();
        Literal* literalexpr();
        SymbolExpression* symbolexpr();
        
        std::vector<Use> _use;
        
        std::string _path;
        
        std::vector<std::pair<std::string, size_t>> similarTo(const std::string& str);
        
    public:
        const std::vector<Use>& use() const;
        void reset();
        File* parse();
        Parser(std::vector<Token> &tokens);
        bool hasErrors() const;
        const std::vector<Error>& errors() const;
        void setPath(const std::string& path);
    };
}

#endif /* Parser_hpp */
