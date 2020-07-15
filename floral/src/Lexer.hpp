//
//  Lexer.hpp
//  floral
//
//  Created by Ethan Uppal on 6/29/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Lexer_h
#define Lexer_h

#include <string>
#include "Token.hpp"

namespace Floral {
    class Lexer {
        std::string code;
        size_t pos {};
        size_t line {1};
        
        public:
        Lexer(const std::string &code);
        void reset();
        
        private:
        char current();
        char peek();
        void advance();
        void next(int c);
        bool eof();
        TokenLoc loc() const;
        
        bool isSpaceChar();
        bool isIdentifierStart();
        bool isIdentifierChar();
        bool isDigitChar();
        bool isHexDigitChar();
        bool isNumTermin();
        bool isQuoteChar();
        bool isDotChar();
        
        Token multichar();
        Token simpleStr();
        Token number();
        
        Token _tkn(TokenType type, const std::string &content);
        Token drive();
        
        public:
        std::vector<Token> lex();
    };
}

#endif /* Lexer_h */
