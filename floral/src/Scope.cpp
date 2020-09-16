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

    void Scope::insert(const std::string& name, Type* type) {
        _items.push_back({name, type});
    }
    Type* Scope::typeOf(const std::string& name) {
        const auto iter = std::find_if(_items.begin(), _items.end(), [name](std::pair<std::string, Type*> item) -> bool {
            return item.first == name;
        });
        if (iter == _items.end()) return nullptr;
        else return iter->second;
    }
}
