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
#include "Colors.hpp"

namespace Floral {
    void Error::print(const std::string& optionalSource, size_t pathext) const {
        ColoredStream out(std::cerr);
        
        out.resetAutomatically = true;
        
        std::string pathitem;
        if (!path.empty()) {
            FilePath fpath {path};
            pathitem = fpath.last();
        }
        if (isWarning) out << Color::yellow;
        else out << Color::red;
        out << _domainStrings[domain] << ": " << pathitem << ' ';
        if (location.startLine == location.endLine) {
            std::cout << '(' << location.startLine << ')';
        } else {
            std::cout << '('<< location.startLine << '-' << location.endLine << ')';
        }
        out << ": " << text << '\n';
        
        if (!optionalSource.empty()) {
            size_t start;
            const std::string line = extractLine(optionalSource, errloc.pos, &start);
            const long shift = errloc.pos - start - 1;
            
            out << Color::white << line << '\n';
            for (long i = 0; i < shift; i++) fputc(' ', stdout);
            out << Color::white << '^';
            if (errloc.len) {
                for (size_t i = 1; i < errloc.len; i++) out << Color::white << '^';
            }
            if (fix.empty()) fputc('\n', stdout);
            else out << '\n' << fix << '\n';
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
    void ErrorReporting::report(Error::Domain domain, const std::string &text, const std::string &path, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {domain, text, loc, errloc};
        err.fix = fix;
        err.path = path;
        _errors.push_back(err);
    }
    void ErrorReporting::warn(const std::string &text, const std::string &path, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {Error::warning, text, loc, errloc};
        err.fix = fix;
        err.path = path;
        err.isWarning = true;
        _warnings.push_back(err);
    }
}
