//
//  Operator.hpp
//  floral
//
//  Created by Ethan Uppal on 7/6/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Operator_hpp
#define Operator_hpp

#include "Token.hpp"
#include "Type.hpp"

namespace Floral {
    class Operator {
        const TokenType _type;
        
    public:
        Operator(TokenType type);
        Type* overloadExists(Type* left, Type* right) const;
    };
}
#endif /* Operator_hpp */
