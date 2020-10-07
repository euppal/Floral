//
//  Scope.hpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Scope_hpp
#define Scope_hpp

#include <unordered_map>
#include "Type.hpp"
#include "AST.hpp"

namespace Floral {
    class Expression;
    class Scope {
        std::unordered_map<std::string, Type*> _types;
        std::unordered_map<std::string, Expression*> _locals;
        
    public:
        Scope();
        
        bool exists(const std::string& name) const;
        void insert(const std::string& name, Type* type, Expression* expr);
        Type* typeOf(const std::string& name) const;
        Expression* lookup(const std::string& name) const;
        struct Function* func;
    };
}

#endif /* Scope_hpp */
