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
    struct ErrorLoc {
        size_t pos;
        size_t len;
    };
    struct Error {
        enum Domain {
            parseDomain,
            generalRejectionDomain,
            compileDomain,
            resolutionDomain,
            typeDomain
        };
        Domain domain;
        std::string text;
        TextRegion location;
        ErrorLoc errloc;
        
        // optional
        std::string fix;
        std::string path;
        
        Error(Domain domain, const std::string& text, TextRegion location, ErrorLoc errloc);
        
        void print(const std::string& optionalSource = "", size_t pathext = 3) const;
        
    private:
        std::string _domainStrings[5] { "Parsing Error", "General Rejection Error", "Compiliation Error", "Resolution Error", "Type Error" };
    };

    class ErrorReporting {
        std::vector<Error> _errors;
        virtual void report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "") = 0;
        
    public:
        virtual bool hasErrors() const = 0;
        virtual const std::vector<Error>& errors() const = 0;
    };

    const std::string extractLine(const std::string& src, size_t pos, size_t* start);
}

#endif /* Error_hpp */
