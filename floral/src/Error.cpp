//
//  Error.cpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Error.hpp"
#include "AST.hpp"
#include <iostream>

namespace Floral {
    void Error::print() const {
        std::cout << _domainStrings[domain] << ": " << text << " (";
        location.describe(')');
        std::cout << '\n';
    }
    Error::Error(Domain domain, const std::string& text, TextRegion location): domain(domain), text(text), location(location) {}
}
