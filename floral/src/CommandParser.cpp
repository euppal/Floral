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
                if (strncmp(arg + 1, "O", 1) == 0) {
//                    std::cout << "Arg: Optimize\n";
                    if ((arg + 1)[1]) {
                        std::string opt { arg + 1 };
                        std::string_view view { opt };
                        view.remove_prefix(1);
                        opt = view;
                        _optimization = atoi(opt.c_str());
                        if (_optimization > 3) {
                            report(Error::parseDomain, "Unknown optimization level", { 0, 0, 0, 0}, { 0, 0 });
                        }
                    } else {
                        _optimization = 1;
                    }
                } else if (strncmp(arg + 1, "-use-C", 6) == 0) {
//                    std::cout << "Arg: Include the C functions header\n";
                    _useC = true;
                } else if (strncmp(arg + 1, "-no-stdlib-header", 17) == 0) {
//                    std::cout << "Arg: Do not include the stdlib header\n";
                    _noStdlibHeader = true;
                } else if (strncmp(arg + 1, "o", 1) == 0) {
                    index++; command.argc--;
                    _outfile = {
                        command.argv[index],
                        CmdFileExt::exec
                    };
                } else if (strncmp(arg + 1, "-verbose", 9)) {
                    _logDebugInfo = true;
                } else if (strncmp(arg + 1, "-cat-src", 9)) {
                    _catSrc = true;
                } else if (strncmp(arg + 1, "-type-trace", 12)) {
                    _typeTrace = true;
                }
                else if (false) {
                    
                }
            } else {
                const std::string str { arg };
                _infiles.push_back({
                    str,
                    ends_with(str, ".floral") ? CmdFileExt::floral : ends_with(str, ".fh") ? CmdFileExt::fh : ends_with(str, ".c") ? CmdFileExt::c : ends_with(str, ".nasm") ? CmdFileExt::nasm : CmdFileExt::unknown
                });
            }
        }
    }

    void CommandParser::report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {domain, text, loc, errloc};
        err.fix = fix;
        _errors.push_back(err);
    }
    void CommandParser::warn(const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {Error::warning, text, loc, errloc};
        err.fix = fix;
        err.isWarning = true;
        _errors.push_back(err);
    }
    bool CommandParser::hasErrors() const {
        return !_errors.empty();
    }
    const std::vector<Error>& CommandParser::errors() const {
        return _errors;
    }
    bool CommandParser::hasWarnings() const {
        return !_warnings.empty();
    }
    const std::vector<Error>& CommandParser::warnings() const {
        return _warnings;
    }


    const std::vector<CmdFile>& CommandParser::infiles() const {
        return _infiles;
    }
    const CmdFile& CommandParser::outfile() const {
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
    const int CommandParser::logDebugInfo() const {
        return _logDebugInfo;
    }
    const int CommandParser::catSource() const {
        return _catSrc;
    }
    const int CommandParser::typeTrace() const {
        return _typeTrace;
    }
}
