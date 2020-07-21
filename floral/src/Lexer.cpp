//
//  Lexer.cpp
//  floral
//
//  Created by Ethan Uppal on 6/28/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Lexer.hpp"
#include "Token.hpp"
#include <string>
#include <cctype>
#include <vector>

namespace Floral {
    Lexer::Lexer(const std::string &code):
    code(code) {};

    void Lexer::reset() {
        // Restores lexer to original state
        pos = 0;
        line = 1;
    }
    char Lexer::current() {
        // Returns the current character
        if (eof()) return '\0';
        return code.at(pos);
    }
    char Lexer::peek() {
        // Peeks at the next character
        return code.at(pos + 1);
    }
    void Lexer::advance() {
        // Advances and discards newlines
        ++pos;
        while (current() == '\n') {
            ++line; ++pos;
        }
    }
    void Lexer::next(int c = 1) {
        for (int i {}; i < c; ++i) {
            ++pos;
            if (current() == '\n') {
                ++line;
            }
        }
    }

    bool Lexer::eof() {
        // Checks whether pos is one past the last valid string index
        return pos == code.size();
    }
    
    Token Lexer::_tkn(TokenType type, const std::string &content) {
        return { {pos, line}, type, content };
    }
    TokenLoc Lexer::loc() const {
        return { pos, line };
    }

    bool Lexer::isSpaceChar() {
        return isspace(current());
    }
    bool Lexer::isIdentifierStart() {
        return current() == '_' || isalpha(current());
    }
    bool Lexer::isIdentifierChar() {
        return current() == '_' || isalnum(current());
    }
    bool Lexer::isDigitChar() {
        return isdigit(current());
    }
    bool Lexer::isHexDigitChar() {
        return ishexnumber(current());
    }
    bool Lexer::isNumTermin() {
        return eof() || isSpaceChar() || current() == '\n';
    }
    bool Lexer::isQuoteChar() {
        return current() == '\"';
    }
    bool Lexer::isDotChar() {
        return current() == '.';
    }

    Token Lexer::multichar() {
        std::string id {};
        TokenLoc cachedLoc { loc() };
        do {
            id.push_back(current());
            next();
        } while (isIdentifierChar());
        if (Floral::keywords.find(id) != Floral::keywords.end())
            return { cachedLoc, Floral::keywords.at(id), id };
        return { cachedLoc, TokenType::identifier, id };
    }

    Token Lexer::simpleStr() {
        TokenLoc cachedLoc { loc() };
        advance();
        std::string str {};
        while (!eof() && !isQuoteChar()) {
            str.push_back(current());
            advance();
        }
        if (isQuoteChar())
            advance();
        else
            return _tkn(TokenType::invalid, "");
        return { cachedLoc, TokenType::simpleString, str };
    }
    Token Lexer::number() {
        TokenLoc cachedLoc { loc() };
        std::string num {};
        if (current() == '0' && peek() == 'x') {
            next(2);
            while (!isNumTermin() && isHexDigitChar()) {
                num.push_back(current());
                advance();
            }
            return { cachedLoc, TokenType::numIntHex, num };
        }
        bool dot { false };
        while (!isNumTermin() && isDigitChar()) {
            num.push_back(current());
            next();
            if (isDotChar()) {
                if (dot)
                    return { cachedLoc, TokenType::numFloating, num };
                dot = true;
                num.push_back('.');
                next();
            }
        }
        return { cachedLoc, dot ? TokenType::numFloating : TokenType::numIntDec, num };
    }
    
    Token Lexer::drive() {
        while (isSpaceChar()) {
            advance();
        }
        
        if (eof())
            return _tkn(TokenType::invalid, "");
        
        if (isDigitChar())
            return number();
        
        if (isIdentifierStart())
            return multichar();
        
        if (isQuoteChar())
            return simpleStr();
        
        // Tries to lex a token
        Token tmp { _tkn(TokenType::invalid, "") };
        switch (current()) {
            case '(':
                tmp = _tkn(TokenType::leftParenthesis, "(");
                break;
            case ')':
                tmp = _tkn(TokenType::rightParenthesis, ")");
                break;
            case '{':
                tmp = _tkn(TokenType::leftBrace, "{");
                break;
            case '}':
                tmp = _tkn(TokenType::rightBrace, "}");
                break;
            case '[':
                tmp = _tkn(TokenType::leftBracket, "[");
                break;
            case ']':
                tmp = _tkn(TokenType::rightBracket, "]");
                break;
            case ';':
                tmp = _tkn(TokenType::semicolon, ";");
                break;
            case ',':
                tmp = _tkn(TokenType::comma, ",");
                break;
            case '+':
                tmp = _tkn(TokenType::plus, "+");
                break;
            case '-':
                tmp = _tkn(TokenType::minus, "-");
                break;
            case '*':
                tmp = _tkn(TokenType::multiply, "*");
                break;
            case '/':
                tmp = _tkn(TokenType::divide, "/");
                break;
            case '=':
                if (peek() == '=') {
                    tmp = _tkn(TokenType::equal, "==");
                    next();
                } else
                    tmp = _tkn(TokenType::assign, "=");
                break;
            case '!':
                if (peek() == '=') {
                    tmp = _tkn(TokenType::unequal, "!=");
                    next();
                } else
                    tmp = _tkn(TokenType::notOp, "!");
                break;
            case '&':
                tmp = _tkn(TokenType::andOp, "&");
                break;
            case '|':
                tmp = _tkn(TokenType::orOp, "|");
                break;
            case '%':
                tmp = _tkn(TokenType::modulus, "%");
                break;
            case '^':
                tmp = _tkn(TokenType::xorOp, "^");
                break;
            case ':':
                tmp = _tkn(TokenType::colon, ":");
                break;
            case '>':
                if (peek() == '=') {
                    tmp = _tkn(TokenType::lessEqual, "<=");
                    next();
                } else
                    tmp = _tkn(TokenType::less, "<");
                break;
            case '<':
                if (peek() == '=') {
                    tmp = _tkn(TokenType::greaterEqual, ">=");
                    next();
                } else
                    tmp = _tkn(TokenType::greater, ">");
                break;
            default:
                break;
        }
        advance();
        return tmp;
    }

    std::vector<Token> Lexer::lex() {
        std::vector<Token> tkns {};
        Token tkn { drive() };
        while (tkn.type != TokenType::invalid) {
            tkns.push_back(tkn);
            tkn = drive();
            if (eof()) {
                if (tkn.type != TokenType::invalid)
                    tkns.push_back(tkn);
                break;
            }
        }
        return tkns;
    }
}
