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
        stdlib, C
    };
    class Parser: public ErrorReporting {
        void report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "");
        void warn(const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "");

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
        Declaration* global();
        Statement* statement(bool checkSemicolon = true);
        LetStatement* let(bool checkSemicolon = true);
        VarStatement* var(bool checkSemicolon = true);
        CallStatement* callStm(bool checkSemicolon = true);
        EmptyStatment* emptyStm();
        ExpressionStatement* exprStm(bool checkSemicolon = true);
        ReturnStatement* returnStm(bool checkSemicolon = true);
        Statement* assignmentStm(bool checkSemicolon = true);
        IfStatement* ifStm();
        WhileStatement* whileStm();
        ForStatement* forStm();
        Block* block();
        Expression* expr();
        BinaryExpression* binaryexpr(Expression* lhs, OperatorComponentExpression* op);
        Expression* primaryexpr();
        OperatorComponentExpression* op();
        Call* callexpr();
        Literal* literalexpr();
        SymbolExpression* symbolexpr();
        UnsafeCast* unsafecastexpr();
        
        std::vector<Use> _use;
        std::string _path;
        int _synchr_count {};
        
        std::vector<std::pair<std::string, size_t>> similarTo(const std::string& str);
        
    public:
        const std::vector<Use>& use() const;
        void reset();
        File* parse();
        Parser(std::vector<Token> &tokens);
        bool hasErrors() const;
        const std::vector<Error>& errors() const;
        bool hasWarnings() const;
        const std::vector<Error>& warnings() const;
        void setPath(const std::string& path);
    };
}

#endif /* Parser_hpp */
