//
//  Token.cpp
//  floral
//
//  Created by Ethan Uppal on 6/28/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Token.hpp"
#include <string>

namespace Floral {
    Token::Token(TokenLoc loc, TokenType type, const std::string& contents): loc(loc), type(type), contents(contents) {}
    std::string tokenTypeDescription(TokenType type) {
        return tokenTypeStrings[static_cast<int>(type)];
    }
    void Token::print() const {
        std::cout << "{";
        loc.print();
        std::cout << ", "
                  << tokenTypeDescription(type)
                  << ", \"" << contents << "\""
                  << "}\n";
    }
    size_t& Token::pos() {
        return loc.pos;
    }
    const size_t& Token::pos() const {
        return loc.pos;
    }
    size_t Token::end() const {
        return loc.pos + contents.size();
    }
    size_t Token::line() const {
        return loc.line;
    }
    bool Token::isLiteral() const {
        return type == TokenType::boolTrue || type == TokenType::boolFalse || type == TokenType::simpleString || type == TokenType::numIntDec || type == TokenType::numIntHex || type == TokenType::numFloating;
    }
    bool Token::isOperator() const {
        auto intType { static_cast<int>(type) };
        return (intType >= static_cast<int>(TokenType::plus) && intType <= static_cast<int>(TokenType::unequal)) || type == TokenType::leftParenthesis;
    }
    bool Token::isDeclarator() const {
        return type == TokenType::func || type == TokenType::global || type == TokenType::let || type == TokenType::var;
    }
    bool Token::isType() const {
        auto intType { static_cast<int>(type) };
        return (intType >= static_cast<int>(TokenType::int64Type) && intType <= static_cast<int>(TokenType::voidType));
    }
    bool Token::isValid() const {
        return !isInvalid();
    }
    bool Token::isInvalid() const {
        return type == TokenType::invalid;
    }

    Token* Token::invalid;
}
