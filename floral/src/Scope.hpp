//
//  Scope.hpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include <vector>
#include "Type.hpp"
#include "AST.hpp"

#ifndef Scope_hpp
#define Scope_hpp

namespace Floral {
    class Scope {
        std::vector<std::pair<std::string, Type*>> _items;
        
    public:
        Scope();
        
        void insert(const std::string& name, Type* type);
        Type* typeOf(const std::string& name);
        struct Function* func;
    };
}

#endif /* Scope_hpp */
