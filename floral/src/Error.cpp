//
//  Error.cpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Error.hpp"
#include "AST.hpp"
#include "FilePath.hpp"
#include <iostream>

namespace Floral {
    void Error::print(const std::string& optionalSource, size_t pathext) const {
        std::string pathitem;
        if (!path.empty()) {
            FilePath fpath {path};
            fpath.keepLast(pathext);
            pathitem = fpath.path();
        }
        std::cout << _domainStrings[domain] << ": " << pathitem;
        if (location.startLine == location.endLine) {
            std::cout << '(' << location.startLine << ')';
        } else {
            std::cout << '('<< location.startLine << '-' << location.endLine << ')';
        }
        std::cout << ": " << text << '\n';
        
        if (!optionalSource.empty()) {
            size_t start;
            const std::string line = extractLine(optionalSource, errloc.pos, &start);
            const long shift = errloc.pos - start - 1;
            
            std::cout << line << '\n';
            for (long i = 0; i < shift; i++) fputc(' ', stdout);
            fputc('^', stdout);
            if (errloc.len) {
                for (size_t i = 1; i < errloc.len; i++) fputc('^', stdout);
            }
            if (fix.empty()) fputc('\n', stdout);
            else std::cout << '\n' << fix << '\n';
        }
        fputc('\n', stdout);
    }
    Error::Error(Domain domain, const std::string& text, TextRegion location, ErrorLoc errloc): domain(domain), text(text), location(location), errloc(errloc) {}

    const std::string extractLine(const std::string& src, size_t pos, size_t* start) {
        if (src.size() < pos) return "";
        *start = pos;
        size_t end = pos;
        while (*start > 0 && src[*start] != '\n') (*start)--;
        while (end < src.size() && src[end] != '\n') end++;
        return src.substr(*start, end - *start);
    }
}
