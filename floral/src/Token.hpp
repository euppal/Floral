//
//  Token.hpp
//  floral
//
//  Created by Ethan Uppal on 6/29/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Token_h
#define Token_h

#include <string>
#include <iostream>
#include <unordered_map>
#include <memory>

namespace Floral {
    class TokenLoc {
        size_t pos;
        size_t line;
        
        public:
        TokenLoc(size_t pos, size_t line): pos(pos), line(line) {}
        void print() const {
            std::cout << "{" << pos << ", " << line << "}";
        }
        friend struct Token;
    };
    enum class TokenType {
        invalid,
        identifier,
        leftParenthesis, rightParenthesis,
        leftBrace, rightBrace,
        leftBracket, rightBracket,
        semicolon, colon, comma, arrow,
        func,
        simpleString, boolTrue, boolFalse, numIntDec, numIntHex, numFloating,
        int64Type, uint64Type,
        charType, ucharType,
        shortType, ushortType,
        int32Type, uint32Type,
        boolType,
        stringType, cStringType,
        voidType,
        plus, minus, multiply, divide, assign, notOp, andOp, orOp, modulus, xorOp, less, greater, lessEqual, greaterEqual, equal, unequal, power,
        global, let, var,
        return_, using_, const_
    };
    bool tokenTypeIsOperator(TokenType type);
    const std::string tokenTypeStrings[] {
        "invalid", "identifier", "leftParenthesis", "rightParenthesis", "leftBrace", "rightBrace", "leftBracket", "rightBracket", "semicolon", "colon", "comma", "arrow", "func",
        "simpleString", "boolTrue", "boolFalse", "decimalInteger", "hexadecimalInteger", "floatingPointNumber",
        "int64Type", "uint64Type",
        "charType", "ucharType",
        "shortType", "ushortType",
        "int32Type", "uint32type",
        "boolType",
        "stringType", "cStringType",
        "voidType",
        "plus", "minus", "multiply", "divide", "assign", "not", "and", "or", "modulus", "xor", "less", "greater", "lessEqual", "greaterEqual", "equal", "unequal", "power"
        "global", "let", "var",
        "return", "using", "const"
    };

    const std::unordered_map<std::string, TokenType> keywords = {
        { "func", TokenType::func },
        { "true", TokenType::boolTrue },
        { "false", TokenType::boolFalse },
        { "global", TokenType::global },
        { "let", TokenType::let },
        { "var", TokenType::var },
        { "Int", TokenType::int64Type },
        { "Int64", TokenType::int64Type },
        { "UInt", TokenType::uint64Type },
        { "UInt64", TokenType::uint64Type },
        { "Char", TokenType::charType },
        { "Int8", TokenType::charType },
        { "UChar", TokenType::ucharType },
        { "UInt8", TokenType::ucharType },
        { "Short", TokenType::shortType },
        { "Int16", TokenType::shortType },
        { "UShort", TokenType::ushortType },
        { "UInt16", TokenType::ushortType },
        { "Int32", TokenType::int32Type },
        { "UInt32", TokenType::uint32Type },
        { "Bool", TokenType::boolType },
        { "String", TokenType::stringType },
        { "CString", TokenType::cStringType },
        { "Void", TokenType::voidType },
        { "return", TokenType::return_ },
        { "using", TokenType::using_ },
        { "const", TokenType::const_ }
    };

    std::string tokenTypeDescription(TokenType type);
    struct Token {

        TokenLoc loc;
        TokenType type;
        std::string contents;
           
    public:
        Token(TokenLoc loc, TokenType type, const std::string& contents);
        friend bool operator ==(const Token& lhs, const Token& rhs);
        
        void print() const;
        size_t& pos();
        const size_t& pos() const;
        size_t end() const;
        size_t line() const;
        
        bool isLiteral() const;
        bool isOperator() const;
        bool isDeclarator() const;
        bool isType() const;
        bool isValid() const;
        bool isInvalid() const;
        bool isId() const;
        
        static Token* invalid;
    };
}


#endif /* Token_h */
