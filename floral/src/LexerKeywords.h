//
//  Keywords.h
//  floral
//
//  Created by Ethan Uppal on 11/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Keywords_h
#define Keywords_h

#include "Token.hpp"
#include <unordered_map>

namespace Floral {
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
}
#endif /* Keywords_h */
