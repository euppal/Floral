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
    class Parser {
        std::vector<Error> _errors;
        void report(Error::Domain domain, const std::string& text, TextRegion loc);

        public:
        std::string path;
        
        private:
        std::vector<Token> tokens;
        size_t index;
        
        Token current();
        Token peek();
        void advance();
        bool eof();
        Token match(TokenType type);
        void synchronize();
        
        Type* type();
        Function* function();
        Statement* statement();
        CallStatement* callStm();
        EmptyStatment* emptyStm();
        LiteralStatement* literalStm();
        FlatExpression* expr();
        Call* call();
        Literal* literal();
        
        public:
        void reset();
        File* parse();
        Parser(std::vector<Token> &tokens);
        bool hasErrors() const;
        const std::vector<Error>& errors() const;
    };
}

#endif /* Parser_hpp */
