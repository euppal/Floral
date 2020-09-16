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
        void reset(void);
        
    private:
        char current(void);
        char peek(void);
        void advance(void);
        void next(int c);
        bool eof(size_t inset = 0);
        TokenLoc loc(void) const;
        
        bool isSpaceChar(void);
        bool isIdentifierStart(void);
        bool isIdentifierChar(void);
        bool isDigitChar(void);
        bool isHexDigitChar(void);
        bool isNumTermin(void);
        bool isQuoteChar(void);
        bool isDotChar(void);
        
        Token multichar(void);
        Token simpleStr(void);
        Token number(void);
        void comments(void);
        
        Token _tkn(TokenType type, const std::string &content);
        Token drive(void);
        
    public:
        std::vector<Token> lex();
    };
}

#endif /* Lexer_h */
