//
//  CommandParser.cpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 8/30/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "CommandParser.hpp"

namespace Floral {
    std::unordered_map<std::string, std::string> libmap {
        { "stl", FLORAL_OBJ(std) },
        { "C", FLORAL_OBJ(cbridge) }
    };
    std::string liblocFromName(const std::string& name) {
        return libmap.find(name)->second;
    }
    void setlibloc(const std::string& libloc, const std::string& name) {
        libmap.insert({ name, libloc });
    }

    bool ends_with(const std::string& fullString, const std::string& ending) {
        if (fullString.length() >= ending.length()) {
            return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
        } else {
            return false;
        }
    }

    CommandParser::CommandParser(Command command) {
        if (command.argc == 1) {
            report(Error::parseDomain, "No input files", { 0, 0, 0, 0}, { 0, 0 });
            return;
        }
        
        if (strncmp(command.argv[1], "help", 5) == 0) {
            packed_options_0 |= (1 << _showHelp);
            return;
        }
        
        int index = 0;
        
        _outfile = {
            "",
            CmdFileExt::unknown
        };

        while (index++, --command.argc) {
            const char* arg = command.argv[index];
            if (arg[0] == '-') {
                if (strncmp(arg + 1, "O", 1) == 0) {
                    if ((arg + 1)[1]) {
                        std::string opt { arg + 1 };
                        std::string_view view { opt };
                        view.remove_prefix(1);
                        opt = view;
                        _optimization = atoi(opt.c_str());
                        if (_optimization > 3) {
                            report(Error::parseDomain, "Unknown optimization level", { 0, 0, 0, 0} , { 0, 0 });
                        }
                    } else {
                        _optimization = 1;
                    }
                } else if (strncmp(arg + 1, "use", 4) == 0) {
                    index++; command.argc--;
                    const char* lib = command.argv[index];
                    libs.insert(lib);
                } else if (strncmp(arg + 1, "U", 2) == 0) {
                    libs.insert(arg + 2);
                } else if (strncmp(arg + 1, "o", 2) == 0) {
                    if (!_outfile.first.empty()) {
                        report(Error::parseDomain, "Cannot specify two target locations", { 0, 0, 0, 0 }, { 0, 0 });
                    }
                    index++; command.argc--;
                    _outfile = {
                        command.argv[index],
                        CmdFileExt::exec
                    };
                } else if (strncmp(arg + 1, "print-ast", 9) == 0 || strncmp(arg + 1, "a", 2) == 0) {
                    packed_options_0 |= (1 << _logDebugInfo);
                } else if (strncmp(arg + 1, "cat-src", 9) == 0 || strncmp(arg + 1, "s", 2) == 0) {
                    packed_options_0 |= (1 << _catSrc);
                } else if (strncmp(arg + 1, "dump-type-trace", 12) == 0 || strncmp(arg + 1, "t", 2) == 0) {
                    packed_options_0 |= (1 << _typeTrace);
                } else if (strncmp(arg + 1, "c", 2) == 0) {
                    packed_options_0 |= (1 << _justCompile);
                } else if (strncmp(arg + 1, "S", 2) == 0) {
                    packed_options_0 |= (1 << _stopAtASM);
                } else if (strncmp(arg + 1, "open-asm", 9) == 0) {
                    packed_options_0 |= (1 << _openASM);
                } else if (strncmp(arg + 1, "verbose", 8) == 0 || strncmp(arg + 1, "v", 2) == 0) {
                    packed_options_0 |= (1 << _verbose);
                } else if (strncmp(arg + 1, "###", 4) == 0) {
                    packed_options_0 |= (1 << _printNotRunCmds);
                } else if (strncmp(arg + 1, "stack-guard", 12) == 0 || strncmp(arg + 1, "g", 2) == 0) {
                    packed_options_0 |= (1 << _stackGuard);
                }
            } else {
                const std::string str { arg };
                _infiles.push_back({
                    str,
                    ends_with(str, ".floral") ? CmdFileExt::floral : ends_with(str, ".fh") ? CmdFileExt::fh : ends_with(str, ".c") ? CmdFileExt::c : ends_with(str, ".nasm") ? CmdFileExt::nasm : CmdFileExt::unknown
                });
            }
            if (_outfile.first.empty() && !justCompile()) {
                _outfile = {
                    "floral.out",
                    CmdFileExt::exec
                };
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

    std::vector<CmdFile>& CommandParser::infiles() {
        return _infiles;
    }
    const CmdFile& CommandParser::outfile() const {
        return _outfile;
    }
    const uint32_t CommandParser::showHelp() const {
        return packed_options_0 & (1 << _showHelp);
    }
    const int CommandParser::optimization() const {
        return _optimization;
    }
    const int CommandParser::usingCBridge() const {
        return libs.find("C") != libs.end();
    }
    const int CommandParser::usingSTL() const {
        return libs.find("stl") != libs.end();
    }
    void CommandParser::initLibs(std::set<std::string>& libs) const {
        for (auto &lib: this->libs) {
            libs.insert(lib);
        }
    }
    const uint32_t CommandParser::logDebugInfo() const {
        return packed_options_0 & (1 << _logDebugInfo);
    }
    const uint32_t CommandParser::catSource() const {
        return packed_options_0 & (1 << _catSrc);
    }
    const uint32_t CommandParser::typeTrace() const {
        return packed_options_0 & (1 << _typeTrace);
    }
    const uint32_t CommandParser::justCompile() const {
        return packed_options_0 & (1 << _justCompile);
    }
    const uint32_t CommandParser::stopAtASM() const {
        return packed_options_0 & (1 << _stopAtASM);
    }
    const uint32_t CommandParser::openASM() const {
        return packed_options_0 & (1 << _openASM);
    }
    const uint32_t CommandParser::isVerbose() const {
        return packed_options_0 & (1 << _verbose);
    }
    const uint32_t CommandParser::printNotRunCmds() const {
        return packed_options_0 & (1 << _printNotRunCmds);
    }
    const uint32_t CommandParser::stackGuard() const {
        return packed_options_0 & (1 << _stackGuard);
    }
}
