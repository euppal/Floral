//
//  Operator.cpp
//  floral
//
//  Created by Ethan Uppal on 7/6/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Operator.hpp"
#include <cassert>

namespace Floral {
    Operator::Operator(TokenType type): _type(type) {
        assert(tokenTypeIsOperator(type));
    }
    Type* Operator::overloadExists(Type *left, Type *right) const {
        switch (_type) {
            case TokenType::plus: {
                if (!left && right) return right->isNumber() ? right : nullptr; // +Number (+1, +3.14)
                if (left && right) return (left->isNumber() && right->isNumber()) || (left->isString() && right->isString()) ? left : nullptr; // Number+Number or String+String (2+3, "h" + "i")
                return nullptr;
            }
            case TokenType::minus: {
                if (!left && right) return right->isNumber() ? right : nullptr; // -Number (-1, -3.14)
                if (left && right) return (left->isNumber() && right->isNumber()) ? left : nullptr; // Number-Number (2-3, 5.8-3.2)
                return nullptr;
            }
            case TokenType::multiply: {
                // if (!left && right) return right->isPointer() ? right->_ptrType : nullptr; // *Pointer (*ptr, *null)
                if (left && right) return (left->isNumber() && right->isNumber()) ? left : nullptr; // Number(Number (3*7, 4*0.5)
                return nullptr;
            }
            case TokenType::andOp: {
                if (left && right) return right->isNumber() && right->isNumber() ? left : nullptr;
                if (!left && right) return new Type(right, true);
            }
            default:
                break;
        }
        return nullptr;
    }
}
