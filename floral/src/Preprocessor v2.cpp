//
//  Preprocessor v2.cpp
//  floralc
//
//  Created by Ethan Uppal on 11/16/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Lexer.hpp"
#include "File IO.hpp"

namespace Floral { namespace v2 {
    #define ERROR(msg) report(Error::prepError, #msg, currentFile(), TextRegion(index, 0, line, line), ErrorLoc(index, 0))
    Preprocessor::Preprocessor(const std::string& source, const std::string& file): _source(source), _fileStack({ file }) {}
    bool Preprocessor::hasErrors() const {
        return !_errors.empty();
    }
    const std::vector<Error>& Preprocessor::errors() const {
        return _errors;
    }
    bool Preprocessor::hasWarnings() const {
        return !_warnings.empty();
    }
    const std::vector<Error>& Preprocessor::warnings() const {
        return _warnings;
    }
    const std::string& Preprocessor::source() const {
        return _source;
    }
    const std::string& Preprocessor::preprocessedSource() const {
        return _preprocessedSource;
    }
    const FileLocation Preprocessor::resolveLocation(size_t pos) const {
        for (auto range: _fileResolutionMap) {
            if (range.contains(pos)) {
                const size_t localizedPos = pos - range.startPos;
                return { localizedPos, range.file };
            }
        }
        return { 0, "" };
    }
    const bool Preprocessor::lookupMacro(const std::string& macro, Macro& value) const {
        auto iter = _defines.find(macro);
        if (iter == _defines.end()) {
            return false;
        } else {
            value = { iter->second.arg, iter->second.value };
            return true;
        }
    }
    const bool Preprocessor::match(const std::string& nextString) {
        const size_t save = index;
        size_t strpos = 0;
        while (strpos < nextString.size() && _source[index] == nextString[strpos]) {
            col++;
            if (nextString[strpos] == '\n') {
                line++; col = 0;
            }
            ++index;
            ++strpos;
        }
        if (strpos == nextString.size()) {
            return true;
        } else {
            index = save;
            return false;
        }
    }
    const bool Preprocessor::processPotentialExpansion() {
        const size_t save = index;
        std::string acc;
        acc.push_back(_source[index]);
        index++;
        while (isalnum(_source[index]) || _source[index] == '_') {
            acc.push_back(_source[index]);
            index++;
            col++;
        }
        std::string arg;
        if (_source[index] == '(') {
            index++;
            while (_source[index] != ')') {
                arg.push_back(_source[index]);
                index++;
                col++;
            }
            index++;
        }
        Macro r;
        if (lookupMacro(acc, r)) {
            _defines.insert({ r.arg, { "", arg } });
            _source.insert(index, r.value);
            return true;
        } else {
            index = save;
            return false;
        }
    }
    void Preprocessor::define(const std::string& macro, const std::string& arg, const std::string& value) {
        _defines.insert({ macro, {arg, value} });
    }
    void Preprocessor::undef(const std::string &macro) {
        _defines.erase(macro);
    }
    bool Preprocessor::isdef(const std::string& macro) const {
        return _defines.find(macro) != _defines.end();
    }
    const std::string Preprocessor::stringTill(const char* terminators) {
        std::string accumulator;
        while ((strchr(terminators, _source[index]) == nullptr) && index < _source.size()) {
            if (_source[index] == '\n') {
                line++;
                col = 1;
            }
            accumulator.push_back(_source[index++]);
        }
        return accumulator;
    }
    const std::string& Preprocessor::currentFile() const {
        return _fileStack.back();
    }
    void Preprocessor::push(const char current) {
        if (!accepts.back()) {
            index++;
            return;
        }
        if (current == '\n') {
            line++; col = 1;
        }
        _preprocessedSource.push_back(current);
        index++;
        col++;
    }
    void Preprocessor::reset() {
        index = 0;
        line = 1;
        col = 1;
        temp = 0;
        accepts = { true };
        _wrapQuotes = false;
    }
    void Preprocessor::preprocess() {
        reset();
        while (index < _source.size() && _source[index]) {
            if (hasErrors()) {
                index++;
            }
            const char current = _source[index];
            if (isalpha(current) || current == '_') {
                processPotentialExpansion();
            }
            if (current == ')' && _wrapQuotes) {
                _preprocessedSource.push_back('\"');
                _wrapQuotes = false;
                index++;
                continue;
            }
            if (current == '#') {
                index++;
                if (match("str")) {
                    _wrapQuotes = true;
                    _preprocessedSource.push_back('\"');
                    if (_source[index] == '(') {
                        index++;
                    } else {
                        ERROR(Expected '(' after #str macro);
                    }
                } if (match("define")) {
                    if (!accepts.back()) {
                        continue;
                    }
                    if (_source[index] == ' ') index++;
                    const std::string macro = stringTill(" \n(");
                    std::string arg;
                    if (_source[index] == ' ') index++;
                    else if (_source[index] == '(') {
                        index++;
                        arg = stringTill(")");
                        index++;
                    }
                    if (_source[index] == ' ') index++;
                    const std::string value = stringTill("\n");
                    if (macro.empty()) {
                        ERROR(Expected identifier after #define macro);
                    }
                    const char current = _source[index];
                    if (current == '\n') {
                        line++; col = 1; index++;
                        define(macro, arg, value);
                    } else {
                        ERROR(Expected newline after #define macro);
                    }
                } else if (match("undef")) {
                    if (!accepts.back()) {
                        continue;
                    }
                    if (_source[index] == ' ') index++;
                    const std::string macro = stringTill("\n");
                    if (macro.empty()) {
                        ERROR(Expected identifier after #undef macro);
                    }
                    const char current = _source[index];
                    if (current == '\n') {
                        line++; col = 1; index++;
                        undef(macro);
                    } else {
                        ERROR(Expected newline after #undef macro);
                    }
                } else if (match("ifdef")) {
                    if (!accepts.back()) {
                        continue;
                    }
                    if (_source[index] == ' ') index++;
                    const std::string macro = stringTill("\n");
                    const char current = _source[index];
                    if (macro.empty()) {
                        ERROR(Expected identifier after #ifdef macro);
                    }
                    if (current == '\n') {
                        line++; col = 1; index++;
                        if (isdef(macro)) {
                            accepts.push_back(true);
                        } else {
                            accepts.push_back(false);
                        }
                    } else {
                        ERROR(Expected newline after #ifdef macro);
                    }
                } else if (match("ifndef")) {
                    if (!accepts.back()) {
                        continue;
                    }
                    if (_source[index] == ' ') index++;
                    const std::string macro = stringTill("\n");
                    if (macro.empty()) {
                        ERROR(Expected identifier after #ifndef macro);
                    }
                    const char current = _source[index];
                    if (current == '\n') {
                        line++; col = 1; index++;
                        if (isdef(macro)) {
                            accepts.push_back(false);
                        } else {
                            accepts.push_back(true);
                        }
                    } else {
                        ERROR(Expected newline after #ifndef macro);
                    }
                } else if (match("endif")) {
                    const char current = _source[index];
                    if (current == '\n') {
                        line++; col = 1; index++;
                        accepts.pop_back();
                    } else {
                        ERROR(Expected newline after #endif macro);
                    }
                } else if (match("include")) {
                    if (!accepts.back()) {
                        continue;
                    }
                    if (_source[index] == ' ') index++;
                    std::string path;
                    switch (_source[index]) {
                        case '<': {
                            index++;
                            path = FLORAL_HDR(stringTill(">\n"));
                            if (_source[++index] == '\n') {
                                line++; col = 1; index++;
                            } else {
                                ERROR(Expected newline after #include macro);
                            }
                            break;
                        }
                        case '\"': {
                            index++;
                            path = stringTill("\"\n");
                            if (_source[++index] == '\n') {
                                line++; col = 1; index++;
                            } else {
                                ERROR(Expected newline after #include macro);
                            }
                            break;
                        }
                        default:
                            ERROR(Unexpected character after #include macro);
                            break;
                    }
                    _fileResolutionMap.push_back({ temp, index, currentFile() });
                    temp = index;
                    _fileStack.push_back(path);
                    std::string buffer;
                    read(path, buffer);
                    Preprocessor preprocessor(buffer, path);
                    for (auto pair: _defines) {
                        preprocessor._defines.insert({ pair.first, pair.second });
                    }
                    preprocessor.preprocess();
                    for (auto error: preprocessor._errors) {
                        _errors.push_back(error);
                    }
                    for (auto warning: preprocessor._warnings) {
                        _warnings.push_back(warning);
                    }
                    _preprocessedSource += preprocessor.preprocessedSource();
                    _fileStack.pop_back();
                    _fileResolutionMap.push_back({ temp, index, path });
                    temp = index;
                } else if (match("line")) {
                    if (!accepts.back()) {
                        continue;
                    }
                    _preprocessedSource += std::to_string(line);
                } else if (match("column")) {
                    if (!accepts.back()) {
                        continue;
                    }
                    _preprocessedSource += std::to_string(col - 6);
                } else if (match("file")) {
                    if (!accepts.back()) {
                        continue;
                    }
                    _preprocessedSource.push_back('\"');
                    _preprocessedSource += currentFile();
                    _preprocessedSource.push_back('\"');
                }
            } else  {
                push(current);
            }
        }
    }
    #undef ERROR
}}
