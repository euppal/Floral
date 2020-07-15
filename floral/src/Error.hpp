//
//  Error.hpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Error_hpp
#define Error_hpp

#include <string>
#include "AST.hpp"

namespace Floral {
    struct Error {
        enum Domain {
            parseDomain
        };
        Domain domain;
        std::string text;
        TextRegion location;
        
        Error(Domain domain, const std::string& text, TextRegion location);
        
        void print() const;
        
    private:
        std::string _domainStrings[1] { "Parsing Error" };
    };
}

#endif /* Error_hpp */
