//
//  CommandParser.hpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 8/30/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef CommandParser_hpp
#define CommandParser_hpp

#include <string>
#include <vector>
#include <set>
#include "Error.hpp"

#define FLORAL_OBJS "/usr/local"
#define FLORAL_SRCS "/usr/local/src"
#define FLORAL_HDR(name) "~/Programming/floral-src/include/" #name ".fh"
#define FLORAL_OBJ(name) "~/Programming/floral-src/stdlib/obj/" #name ".o"
#define FLORAL_RUNTIME(name) "~/Programming/floral-src/runtime/" #name ".o"

namespace Floral {
    enum class Use {
        stl, libc
    };
    std::string liblocFromName(const std::string& name);
    void setlibloc(const std::string& libloc, const std::string& name);

    enum class CmdFileExt {
        floral, fh, c, nasm, exec, object, unknown
    };
    typedef std::pair<std::string, CmdFileExt> CmdFile;

    struct Command {
        int argc;
        const char* *argv;
    };

    bool ends_with(const std::string& fullString, const std::string& ending);

    class CommandParser: public ErrorReporting {        
        std::vector<CmdFile> _infiles;
        CmdFile _outfile;
        
        int _optimization = 0;
        std::set<std::string> libs;
        enum Options {
            _showHelp = 0,
            _logDebugInfo,
            _catSrc,
            _typeTrace,
            _justCompile,
            _stopAtASM,
            _openASM,
            _verbose,
            _printNotRunCmds,
            _stackGuard
        };
        uint32_t packed_options_0 {};
        
        std::vector<Error> _errors;
        void report(Error::Domain domain, const std::string& text, const std::string& path, TextRegion loc, ErrorLoc errloc, const std::string& fix = "");
        void warn(const std::string& text, const std::string& path, TextRegion loc, ErrorLoc errloc, const std::string& fix = "");

    public:
        CommandParser(Command command);
        
        bool hasErrors() const;
        const std::vector<Error>& errors() const;
        bool hasWarnings() const;
        const std::vector<Error>& warnings() const;
        
        std::vector<CmdFile>& infiles();
        const CmdFile& outfile() const;
        const uint32_t showHelp() const;
        const int optimization() const;
        const int usingCBridge() const;
        const int usingSTL() const;
        void initLibs(std::set<std::string>& libs) const;
        const uint32_t logDebugInfo() const;
        const uint32_t catSource() const;
        const uint32_t typeTrace() const;
        const uint32_t justCompile() const;
        const uint32_t stopAtASM() const;
        const uint32_t openASM() const;
        const uint32_t isVerbose() const;
        const uint32_t printNotRunCmds() const;
        const uint32_t stackGuard() const;
    };
}

#endif /* CommandParser_hpp */
