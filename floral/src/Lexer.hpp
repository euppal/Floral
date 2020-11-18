//
//  Lexer.hpp
//  floral
//
//  Created by Ethan Uppal on 6/29/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Lexer_h
#define Lexer_h

#include <string>
#include <map>
#include "Token.hpp"
#include "Error.hpp"
#include "CommandParser.hpp"
#include "LexerKeywords.h"

namespace Floral {
//    inline namespace v1 {
//        class Lexer: public ErrorReporting {
//        public:
//            std::string code;
//        private:
//            size_t pos {};
//            size_t line {1};
//
//            std::map<const std::string, const std::string> _defines;
//
//            void report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "") override;
//            void warn(const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "") override;
//
//        public:
//            bool hasErrors() const override;
//            const std::vector<Error>& errors() const override;
//            bool hasWarnings() const override;
//            const std::vector<Error>& warnings() const override;
//            Lexer(const std::string &code);
//            void reset(void);
//
//        private:
//            char current(void);
//            char peek(void);
//            void advance(void);
//            void next(int c);
//            bool eof(size_t inset = 0);
//            TokenLoc loc(void) const;
//
//            bool isSpaceChar(void);
//            bool isIdentifierStart(void);
//            bool isIdentifierChar(void);
//            bool isDigitChar(void);
//            bool isHexDigitChar(void);
//            bool isNumTermin(void);
//            bool isNumSuffix(void);
//            bool isQuoteChar(void);
//            bool isDotChar(void);
//
//            Token _analyze(const std::string& str);
//
//            Token multichar(bool substitute = true);
//            Token simpleStr(void);
//            Token number(void);
//            void comments(void);
//
//            Token _tkn(TokenType type, const std::string &content);
//            Token drive(void);
//
//        public:
//            bool _doanalyze {true};
//            std::vector<Token> lex();
//        };
//    }

    namespace v2 {
        struct FileRegion {
            size_t startPos;
            size_t endPos;
            const std::string file;
            
            const bool contains(size_t pos) const {
                return pos >= startPos && pos <= endPos;
            }
        };
        struct FileLocation {
            size_t pos;
            const std::string file;
        };
        struct Macro {
            std::string arg;
            std::string value;
        };
        class Preprocessor: public ErrorReporting {
            size_t line; size_t col;
            size_t index;
            size_t temp;
            std::vector<bool> accepts;
            bool _wrapQuotes;
            
            std::string _source;
            std::string _preprocessedSource;
            std::vector<std::string> _fileStack;
            std::vector<FileRegion> _fileResolutionMap;
            std::unordered_map<std::string, Macro> _defines;
            
            const bool match(const std::string& nextString);
            const bool processPotentialExpansion();
            void define(const std::string& macro, const std::string& arg, const std::string& value = "");
            void undef(const std::string& macro);
            bool isdef(const std::string& macro) const;
            const std::string stringTill(const char* terminators);
            const std::string& currentFile() const;
            void push(const char current);
            void reset();
            
        public:
            Preprocessor(const std::string& source, const std::string& file);
            void preprocess();
            
            const std::string& source() const;
            const std::string& preprocessedSource() const;
            const FileLocation resolveLocation(size_t pos) const;
            const bool lookupMacro(const std::string& macro, Macro& value) const;
            
            virtual bool hasErrors() const override;
            virtual const std::vector<Error>& errors() const override;
            virtual bool hasWarnings() const override;
            virtual const std::vector<Error>& warnings() const override;
        };
        class Lexer: public ErrorReporting {            
            std::vector<Token> _tokens;
            Preprocessor _preprocessor;
            const CommandParser& _commandParser;
            
            void reset();
            
        public:
            Lexer(const std::string& source, const std::string& filename, const CommandParser& commandParser);
            
            virtual bool hasErrors() const override;
            virtual const std::vector<Error>& errors() const override;
            virtual bool hasWarnings() const override;
            virtual const std::vector<Error>& warnings() const override;
            
            const Preprocessor& preprocessor() const;
            const std::vector<Token>& lex();
        };
    }
}

#endif /* Lexer_h */
