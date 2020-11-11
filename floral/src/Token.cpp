//
//  Token.cpp
//  floral
//
//  Created by Ethan Uppal on 6/28/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Token.hpp"
#include "Type.hpp"
#include "AST.hpp"
#include <string>

namespace Floral {
    Token::Token(TokenLoc loc, TokenType type, const std::string& contents): loc(loc), type(type), contents(contents) {}
    Token::Token(TokenLoc loc, TokenType type, const std::string& contents, const std::vector<FloralWideChar>& _wstr): loc(loc), type(type), contents(contents), _wstr(_wstr) {}
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
        return
            type == TokenType::boolTrue || type == TokenType::boolFalse ||
            type == TokenType::asciiString || type == TokenType::wideString ||
            type == TokenType::numIntDec || type == TokenType::numUIntDec ||
            type == TokenType::numByteDec || type == TokenType::numUByteDec ||
            type == TokenType::numWideChar || type == TokenType::numWideUChar ||
            type == TokenType::numShortDec || type == TokenType::numUShortDec ||
            type == TokenType::numInt32Dec || type == TokenType::numUInt32Dec ||
            type == TokenType::numIntHex || type == TokenType::numFloating;
    }
    bool Token::isOperator() const {
        return tokenTypeIsOperator(type);
    }
    bool tokenTypeIsOperator(TokenType type) {
        auto intType { static_cast<int>(type) };
        return ((intType >= static_cast<int>(TokenType::plus) && intType <= static_cast<int>(TokenType::unequal)) && type != TokenType::assign) ||
                type == TokenType::leftBracket || type == TokenType::rightBracket ||
                type == TokenType::dot || type == TokenType::arrow;
    }
    bool Token::isDeclarator() const {
        return tokenTypeIsDeclarator(type);
    }
    inline bool tokenTypeIsDeclarator(TokenType type) {
        return type == TokenType::func || type == TokenType::global || type == TokenType::struct_ || type == TokenType::typealias || type == TokenType::namespace_;
    }
    bool Token::isType() const {
        auto intType { static_cast<int>(type) };
        return (intType >= static_cast<int>(TokenType::int64Type) && intType <= static_cast<int>(TokenType::voidType)) || (std::find_if(Type::structs.begin(), Type::structs.end(), [this](StructDeclaration* struct_) -> bool {
                return struct_->name().contents == contents;
            }) != Type::structs.end());
    }
    bool Token::isValid() const {
        return !isInvalid();
    }
    bool Token::isInvalid() const {
        return type == TokenType::invalid;
    }
    bool Token::isId() const {
        return type == TokenType::identifier;
    }

    Token* Token::invalid;

    bool operator ==(const Token& lhs, const Token& rhs) {
        return lhs.contents == rhs.contents;
    }
}
