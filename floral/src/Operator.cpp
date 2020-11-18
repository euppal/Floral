//
//  Operator.cpp
//  floral
//
//  Created by Ethan Uppal on 7/6/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Operator.hpp"
#include <cassert>
#define MOST_CONST(l, r) ((l)->isConst() ? (l) : (r))
#define CONSTEST(l, r) ((l)->isConst() || (r)->isConst())

namespace Floral {
    Operator::Operator(TokenType type): _type(type) {
        assert(tokenTypeIsOperator(type));
    }
    Type* Operator::overload(Type* left, Type* right) const {
        switch (_type) {
            case TokenType::plus: {
                if (!left && right) return right->isNumber() ? right : nullptr; // +Number (+1, +3.14)
                if (left && right && left->isPointer() && right->isNumber()) return left;
                if (left && right) return (left->isNumber() && right->isNumber())/* || (left->isString() && right->isString()) */? MOST_CONST(left, right) : nullptr; // Number+Number or String+String (2+3, "h" + "i")
                return nullptr;
            }
            case TokenType::minus: {
                if (!left && right) return(right->isNumber() && right->isSigned()) ? right : nullptr; // -Number (-1, -3.14)
                if (left && right) return (left->isPointer() && right->isNumber() ? left : (left->isNumber() && right->isNumber() ? MOST_CONST(left, right) : nullptr)); // Number-Number (2-3, 5.8-3.2)
                return nullptr;
            }
            case TokenType::multiply: {
                if (!left && right) return right->isPointer() ? GET_PTRTYYPE(right) : nullptr; // *Pointer (*ptr, *null)
                if (left && right) return (left->isNumber() && right->isNumber()) ? MOST_CONST(left, right) : nullptr; // Number*Number (3*7, 4*0.5)
                return nullptr;
            }
            case TokenType::divide: {
                if (left && right) return (left->isNumber() && right->isNumber()) ? MOST_CONST(left, right) : nullptr; // Number/Number (3/7, 4/0.5)
                return nullptr;
            }
            case TokenType::plusEqu:
            case TokenType::minusEq:
            case TokenType::mulEq:
            case TokenType::divEq: {
                if (left && right && left->isPointer() && right->isNumber()) return left;
                if (left && right) return (left->isNumber() && right->isNumber()) ? MOST_CONST(left, right) : nullptr;
                return nullptr;
            }
            case TokenType::bit_and: {
                if (left && right) {
                    if (left->isNumber() && right->isNumber()) return MOST_CONST(left, right);
                    else if (left->isBool() && right->isBool()) return new Type(new Token(TokenLoc::zero, TokenType::boolType, "Bool"), CONSTEST(left, right));
                    else return nullptr;
                }
                if (!left && right) return new Type(right, true, right->isConst());
            }
            case TokenType::equal:
            case TokenType::unequal:
            case TokenType::less:
            case TokenType::greater:
            case TokenType::lessEqual:
            case TokenType::greaterEqual: {
                if (left && right && *left == *right) return new Type(new Token(TokenLoc::zero, TokenType::boolType, "Bool"), CONSTEST(left, right));
                else return nullptr;
            }
            case TokenType::bit_or:
            case TokenType::bit_xor: {
                if (left && right) (
                    (left->isNumber() && right->isNumber()) ||
                    (left->isBool() && right->isBool())
                ) ? new Type(new Token(TokenLoc::zero, TokenType::boolType, "Bool"), CONSTEST(left, right)) : nullptr;
            }
            case TokenType::bool_not:
            case TokenType::invert: {
                if (!left && right) return right->isBool() ? new Type(new Token(TokenLoc::zero, TokenType::boolType, "Bool"), right->isConst()) : (right->isNumber() ? right : nullptr);
            }
            case TokenType::leftBracket: {
                if (left && right) return (
                    left->isPointer() && right->isNumber()
                ) ? GET_PTRTYYPE(left) : nullptr;
            }
            default:
                break;
        }
        return nullptr;
    }
}
