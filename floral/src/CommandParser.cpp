//
//  CommandParser.cpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 8/30/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "CommandParser.hpp"

namespace Floral {

    bool ends_with(const std::string& fullString, const std::string& ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }

    CommandParser::CommandParser(Command command) {
        int index = 0;
        std::vector<std::string> files;

        while (index++, --command.argc) {
            const char* arg = command.argv[index];
            if (arg[0] == '-') {
                if (strncmp(arg + 1, "O", 2) == 0) {
                    std::cout << "Arg: Optimize\n";
                    _optimization = 1;
                } else if (strncmp(arg + 1, "-use-C", 2) == 0) {
                    std::cout << "Arg: Include the C functions header\n";
                    _useC = true;
                } else if (strncmp(arg + 1, "-no-stdlib-header", 2) == 0) {
                    std::cout << "Arg: Do not include the stdlib header\n";
                    _noStdlibHeader = true;
                } else if (false) {
                    
                }
            } else {
                std::string str { arg };
                if (!(ends_with(str, ".floral") || ends_with(str, ".fh") || ends_with(str, ".s") || ends_with(str, ".asm"))) {
                    std::cout << "The Floral compiler only accepts .floral/.fh files and .s/.asm files for assembly output.\n";
                    return;
                }
                files.push_back(str);
            }
        }
        if (files.size() == 0) {
            std::cout << "Missing infile and/or outfile";
            return;
        } else {
            _infiles.reserve(files.size() - 1);
            for (size_t index{}; index < files.size() - 1; index++) {
                _infiles.push_back(files[index]);
            }
            if (files.size() == 0) {
                std::string last {files.back()};
                last.erase(last.end() - 6, last.end());
                _outfile = last.c_str();
            } else {
                _outfile = files.back();
            }
        }
    }

    void CommandParser::report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {domain, text, loc, errloc};
        err.fix = fix;
        _errors.push_back(err);
    }
        
    bool CommandParser::hasErrors() const {
        return !_errors.empty();
    }
    const std::vector<Error>& CommandParser::errors() const {
        return _errors;
    }

    const std::vector<std::string>& CommandParser::infiles() const {
        return _infiles;
    }
    const std::string& CommandParser::outfile() const {
        return _outfile;
    }
    const int CommandParser::optimization() const {
        return _optimization;
    }
    const int CommandParser::usingCFunctions() const {
        return _useC;
    }
    const int CommandParser::notUsingStdlibHeader() const {
        return _noStdlibHeader;
    }
}
