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
#include <vector>
#include "floral_cdef.h"

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
        invalid, macro,
        identifier,
        leftParenthesis, rightParenthesis,
        leftBrace, rightBrace,
        leftBracket, rightBracket,
        semicolon, colon, comma, dot, arrow, backarrow,
        func,
        asciiString, wideString, boolTrue, boolFalse, null, numIntDec, numUIntDec, numByteDec, numUByteDec, numShortDec, numUShortDec, numInt32Dec, numUInt32Dec, numIntHex, numWideChar, numWideUChar, numFloating,
        int64Type, uint64Type,
        charType, ucharType, wideCharType, wideUCharType,
        shortType, ushortType,
        int32Type, uint32Type,
        boolType,
        voidType,
        plus, minus, inc, dec, multiply, divide, plusEqu, minusEq, mulEq, divEq, assign, notOp, inv, andOp, orOp, modulus, xorOp, less, greater, lessEqual, greaterEqual, equal, unequal, power, scopeResolve,
        global, let, var, if_, while_, for_, struct_, behavior, predecl, typealias,
        return_, using_, const_, sizeof_, unsafe_cast, static_, inline_, namespace_
    };
    bool tokenTypeIsOperator(TokenType type);
    const std::string tokenTypeStrings[] {
        "invalid", "macro",
        "identifier",
        "leftParenthesis", "rightParenthesis",
        "leftBrace", "rightBrace",
        "leftBracket", "rightBracket",
        "semicolon", "colon", "comma", "dot", "arrow", "backarrow",
        "func",
        "asciiString", "wideString", "boolTrue", "boolFalse", "null", "numIntDec", "numUIntDec", "numByteDec", "numUByteDec", "numShortDec", "numUShortDec", "numInt32Dec", "numUInt32Dec", "numIntHex", "numWideChar", "numWideUChar", "numFloating",
        "int64Type", "uint64Type",
        "charType", "ucharType", "wideCharType", "wideUCharType",
        "shortType", "ushortType",
        "int32Type", "uint32Type",
        "boolType",
        "voidType",
        "plus", "minus", "inc", "dec", "multiply", "divide", "plusEqu", "minusEqu", "mulEqu", "divEqu", "assign", "not", "inv", "and", "or", "modulus", "xor", "less", "greater", "lessEqual", "greaterEqual", "equal", "unequal", "power", "scopeResolve",
        "global", "let", "var", "if", "while", "for", "struct", "behavior", "predecl", "typealias",
        "return", "using", "const", "sizeof", "unsafe_cast", "static", "inline", "namespace"
    };

    const std::unordered_map<std::string, TokenType> keywordMap = {
        { "func", TokenType::func },
        { "true", TokenType::boolTrue },
        { "false", TokenType::boolFalse },
        { "global", TokenType::global },
        { "null", TokenType::null },
        { "let", TokenType::let },
        { "var", TokenType::var },
        { "if", TokenType::if_ },
        { "while", TokenType::while_ },
        { "for", TokenType::for_ },
        { "struct", TokenType::struct_ },
        { "behavior", TokenType::behavior },
        { "predecl", TokenType::predecl },
        { "type", TokenType::typealias },
        { "static", TokenType::static_ },
        { "inline", TokenType::inline_ },
        { "namespace", TokenType::namespace_ },
        { "Int", TokenType::int64Type },
        { "Int64", TokenType::int64Type },
        { "QWord", TokenType::int64Type },
        { "UInt", TokenType::uint64Type },
        { "UInt64", TokenType::uint64Type },
        { "UnsignedQWord", TokenType::uint64Type },
        { "Char", TokenType::charType },
        { "Int8", TokenType::charType },
        { "UChar", TokenType::ucharType },
        { "UInt8", TokenType::ucharType },
        { "Byte", TokenType::ucharType },
        { "WideChar", TokenType::wideCharType },
        { "WideUChar", TokenType::wideUCharType },
        { "Short", TokenType::shortType },
        { "Int16", TokenType::shortType },
        { "Word", TokenType::shortType },
        { "UShort", TokenType::ushortType },
        { "UInt16", TokenType::ushortType },
        { "UnsignedWord", TokenType::ushortType },
        { "Int32", TokenType::int32Type },
        { "DWord", TokenType::uint32Type },
        { "UInt32", TokenType::uint32Type },
        { "UnsignedDWord", TokenType::uint32Type },
        { "Bool", TokenType::boolType },
        { "Void", TokenType::voidType },
        { "return", TokenType::return_ },
        { "using", TokenType::using_ },
        { "const", TokenType::const_ },
        { "sizeof", TokenType::sizeof_ },
        { "unsafe_cast", TokenType::unsafe_cast }
    };

    std::string tokenTypeDescription(TokenType type);
    bool tokenTypeIsDeclarator(TokenType type); 

    struct Token {

        TokenLoc loc;
        TokenType type;
        std::string contents;
           
    public:
        Token(TokenLoc loc, TokenType type, const std::string& contents);
        Token(TokenLoc loc, TokenType type, const std::string& contents, const std::vector<FloralWideChar>& _wstr);
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
        
        std::vector<FloralWideChar> _wstr;
    };
}


#endif /* Token_h */
