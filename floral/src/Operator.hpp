//
//  Operator.hpp
//  floral
//
//  Created by Ethan Uppal on 7/6/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Operator_hpp
#define Operator_hpp

#include <cstddef>

namespace Floral {
    struct Operator {
        size_t precedence;
        bool isLeftAssociative;
        Operator(const size_t p, const bool l): precedence(p), isLeftAssociative(l) {}
    };
}
#endif /* Operator_hpp */
