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
    struct Command {
        int argc;
        const char* *argv;
    };

    bool ends_with(const std::string& fullString, const std::string& ending);

    class CommandParser: public ErrorReporting {        
        std::vector<std::string> _files;
        std::vector<std::string> _infiles;
        std::string _outfile;
        int _optimization = 0;
        int _useC = false;
        int _noStdlibHeader = false;
        
        std::vector<Error> _errors;
        void report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "");
        void warn(const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "");

    public:
        CommandParser(Command command);
        
        bool hasErrors() const;
        const std::vector<Error>& errors() const;
        bool hasWarnings() const;
        const std::vector<Error>& warnings() const;
        
        const std::vector<std::string>& infiles() const;
        const std::string& outfile() const;
        const int optimization() const;
        const int usingCFunctions() const;
        const int notUsingStdlibHeader() const;
    };
}

#endif /* CommandParser_hpp */
