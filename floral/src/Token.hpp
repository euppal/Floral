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
        size_t column;
    
    public:
        TokenLoc(size_t pos, size_t line, size_t column, const std::string& filename): pos(pos), line(line), column(column), filename(filename) {}
        void print() const {
            std::cout << '{' << line << ':' << column;
            if (!filename.empty()) {
                std::cout << " (" << filename << ')';
            }
            std::cout << '}';
        }
        friend struct Token;
        
        const static TokenLoc zero;
        const std::string filename;
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
        plus, minus, inc, dec, multiply, divide, plusEqu, minusEq, mulEq, divEq, assign, bool_not, invert, bit_and, bit_or, modulus, bit_xor, bit_xorEq, bit_andEq, bit_orEq, modEq, bool_and, bool_or, bool_xor, less, greater, lessEqual, greaterEqual, equal, unequal, power, scopeResolve,
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
        "plus", "minus", "inc", "dec", "multiply", "divide", "plusEqu", "minusEqu", "mulEqu", "divEqu", "assign", "not", "invert", "and", "or", "modulus", "bit_xor", "bit_xorEq", "bit_andEq", "bit_orEq", "modEq", "bool_and", "bool_or", "bool_xor", "less", "greater", "lessEqual", "greaterEqual", "equal", "unequal", "power", "scopeResolve",
        "global", "let", "var", "if", "while", "for", "struct", "behavior", "predecl", "typealias",
        "return", "using", "const", "sizeof", "unsafe_cast", "static", "inline", "namespace"
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
