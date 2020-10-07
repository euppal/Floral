//
//  Scope.cpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Scope.hpp"
#include <algorithm>

namespace Floral {
    Scope::Scope(): func(nullptr) {}

    void Scope::insert(const std::string& name, Type* type, Expression* expr) {
        if (type) _types[name] = type;
        if (expr) _locals[name] = expr;
    }
    Type* Scope::typeOf(const std::string& name) const {
        auto iter = _types.find(name);
        if (iter == _types.end()) return nullptr;
        else return (*iter).second;
    }
    Expression* Scope::lookup(const std::string& name) const {
        auto iter = _locals.find(name);
        if (iter == _locals.end()) return nullptr;
        else return (*iter).second;
    }
    bool Scope::exists(const std::string& name) const {
        return _types.find(name) != _types.end() || _locals.find(name) != _locals.end();
    }
}
