//
//  Scope.cpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Scope.hpp"

namespace Floral {
    void Scope::enter(const char scope) {
        if (size < MAX_NESTING) {
            *++stack = scope;
            ++size;
        }
    }
    void Scope::exit() {
        --size;
    }
    char Scope::current() const {
        return stack[size-1];
    }
    bool Scope::isWithin(const Scope& otherScope) const {
        if (size > otherScope.size)
            return false;
        for (int i {}; i < size; ++i) {
            if (stack[i] != otherScope.stack[i])
                return false;
        }
        return true;
    }
}
