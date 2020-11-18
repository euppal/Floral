//
//  Lexer v2.cpp
//  floralc
//
//  Created by Ethan Uppal on 11/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Lexer.hpp"
#include "File IO.hpp"
#include "LexerKeywords.h"
#include "LexerOperators.h"

namespace Floral { namespace v2 {
    Lexer::Lexer(const std::string& source, const std::string& filename, const CommandParser& commandParser): _preprocessor(source, filename), _commandParser(commandParser) {}
    bool Lexer::hasErrors() const {
        return !_errors.empty();
    }
    const std::vector<Error>& Lexer::errors() const {
        return _errors;
    }
    bool Lexer::hasWarnings() const {
        return !_warnings.empty();
    }
    const std::vector<Error>& Lexer::warnings() const {
        return _warnings;
    }
    const std::vector<Token>& Lexer::lex() {
        return _tokens;
    }
    const Preprocessor& Lexer::preprocessor() const {
        return _preprocessor;
    }
}}
