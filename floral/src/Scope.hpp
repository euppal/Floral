//
//  Scope.hpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include <cstddef>

#ifndef Scope_hpp
#define Scope_hpp

#define MAX_NESTING 256

namespace Floral {
    class Scope {
        char* stack;
        size_t size;
        
    public:
        void enter(const char scope);
        void exit();
        char current() const;
        
        bool isWithin(const Scope& otherScope) const;
    };
}

#endif /* Scope_hpp */
