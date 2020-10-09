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
#include "Error.hpp"

namespace Floral {
    enum class CmdFileExt {
        floral, fh, c, nasm, exec, unknown
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
        int _useC = false;
        int _noStdlibHeader = false;
        int _logDebugInfo = false;
        int _catSrc = false;
        int _typeTrace = false;
        
        std::vector<Error> _errors;
        void report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "");
        void warn(const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "");

    public:
        CommandParser(Command command);
        
        bool hasErrors() const;
        const std::vector<Error>& errors() const;
        bool hasWarnings() const;
        const std::vector<Error>& warnings() const;
        
        const std::vector<CmdFile>& infiles() const;
        const CmdFile& outfile() const;
        const int optimization() const;
        const int usingCFunctions() const;
        const int notUsingStdlibHeader() const;
        const int logDebugInfo() const;
        const int catSource() const;
        const int typeTrace() const;
    };
}

#endif /* CommandParser_hpp */
