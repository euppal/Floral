//
//  Operators.h
//  floral
//
//  Created by Ethan Uppal on 11/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Operators_h
#define Operators_h

#include "Token.hpp"
#include <unordered_map>

namespace Floral {
    const std::unordered_map<const char*, TokenType> operatorMap = {
        { "+", TokenType::plus },
        { "-", TokenType::minus },
        { "*", TokenType::multiply },
        { "/", TokenType::divide },
        { "~", TokenType::invert },
        { "&", TokenType::bit_and },
        { "!", TokenType::bool_not },
        { "%", TokenType::modulus },
        { "^", TokenType::bit_xor },
        { "|", TokenType::bit_or },
        { ".", TokenType::dot },
        { ",", TokenType::comma },
        { "<", TokenType::less },
        { ">", TokenType::greater },
        { "==", TokenType::equal },
        { "!=", TokenType::unequal },
        { "+=", TokenType::plusEqu },
        { "-=", TokenType::minusEq },
        { "*=", TokenType::mulEq },
        { "/=", TokenType::divEq },
        { "^/", TokenType::bit_xorEq },
        { "&=", TokenType::bit_andEq },
        { "|=", TokenType::bit_orEq },
        { "%=", TokenType::modEq },
        { "&&", TokenType::bool_and },
        { "||", TokenType::bool_or },
        { "^^", TokenType::bool_xor },
        { "<=", TokenType::lessEqual },
        { ">=", TokenType::greaterEqual }
    };
}

#endif /* Operators_h */
