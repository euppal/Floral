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
#include <map>
#include "Token.hpp"
#include "Error.hpp"

namespace Floral {
    class Lexer: public ErrorReporting {
    public:
        std::string code;
    private:
        size_t pos {};
        size_t line {1};
        
        std::map<const std::string, const std::string> _defines;
        
        void report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "") override;
        void warn(const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "") override;

    public:
        bool hasErrors() const override;
        const std::vector<Error>& errors() const override;
        bool hasWarnings() const override;
        const std::vector<Error>& warnings() const override;
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
        bool isNumSuffix(void);
        bool isQuoteChar(void);
        bool isDotChar(void);
        
        std::pair<const std::string, const TokenType> _analyze(const std::string& str);
        
        Token multichar(bool substitute = true);
        Token simpleStr(void);
        Token number(void);
        void comments(void);
        
        Token _tkn(TokenType type, const std::string &content);
        Token drive(void);
        
    public:
        bool _doanalyze {};
        std::vector<Token> lex();
    };
}

#endif /* Lexer_h */
