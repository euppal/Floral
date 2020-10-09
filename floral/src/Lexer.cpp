//
//  Lexer.cpp
//  floral
//
//  Created by Ethan Uppal on 6/28/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Lexer.hpp"
#include "Token.hpp"
#include <string>
#include <cctype>
#include <vector>
#include "File IO.hpp"

namespace Floral {
    Lexer::Lexer(const std::string &code):
    code(code) {};
    
    void Lexer::report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {domain, text, loc, errloc};
        err.fix = fix;
        _errors.push_back(err);
    }
    void Lexer::warn(const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {Error::warning, text, loc, errloc};
        err.fix = fix;
        err.isWarning = true;
        _errors.push_back(err);
    }
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

    void Lexer::reset() {
        // Restores lexer to original state
        pos = 0;
        line = 1;
    }
    char Lexer::current() {
        // Returns the current character
        if (eof()) return '\0';
        return code[pos];
    }
    char Lexer::peek() {
        // Peeks at the next character
        if (eof(-1)) return '\0';
        return code[pos + 1];
    }
    void Lexer::advance() {
        // Advances and discards newlines
        ++pos;
        while (current() == '\n') {
            ++line; ++pos;
        }
    }
    void Lexer::next(int c = 1) {
        for (int i {}; i < c; ++i) {
            ++pos;
            if (current() == '\n') {
                ++line;
            }
        }
    }

    bool Lexer::eof(size_t inset) {
        // Checks whether pos is one past the last valid string index
        return pos >= code.size() + inset;
    }
    
    Token Lexer::_tkn(TokenType type, const std::string &content) {
        return { {pos, line}, type, content };
    }
    TokenLoc Lexer::loc() const {
        return { pos, line };
    }

    bool Lexer::isSpaceChar() {
        return isspace(current());
    }
    bool Lexer::isIdentifierStart() {
        return current() == '_' || isalpha(current());
    }
    bool Lexer::isIdentifierChar() {
        return current() == '_' || isalnum(current());
    }
    bool Lexer::isDigitChar() {
        return isdigit(current());
    }
    bool Lexer::isHexDigitChar() {
        return ishexnumber(current());
    }
    bool Lexer::isNumTermin() {
        return eof() || isSpaceChar() || current() == '\n';
    }
    bool Lexer::isNumSuffix() {
        return current() == 'u' || current() == 'b' || current() == 'w' || current() == 'd' || current() == 'c';
    }
    bool Lexer::isQuoteChar() {
        return current() == '\"';
    }
    bool Lexer::isDotChar() {
        return current() == '.';
    }

    // https://thispointer.com/find-and-replace-all-occurrences-of-a-sub-string-in-c/
    void findAndReplaceAll(std::string & data, std::string toSearch, std::string replaceStr)
    {
        // Get the first occurrence
        size_t pos = data.find(toSearch);
        // Repeat till end is reached
        while( pos != std::string::npos)
        {
            // Replace this occurrence of Sub String
            data.replace(pos, toSearch.size(), replaceStr);
            // Get the next occurrence from the current position
            pos =data.find(toSearch, pos + replaceStr.size());
        }
    }

    Token Lexer::multichar(bool substitute) {
        std::string id {};
        TokenLoc cachedLoc { loc() };
        do {
            id.push_back(current());
            next();
        } while (isIdentifierChar());
        if (substitute && _defines.find(id) != _defines.end()) {
            id = _defines[id];
        }
        if (Floral::keywordMap.find(id) != Floral::keywordMap.end())
            return { cachedLoc, Floral::keywordMap.at(id), id };
        if (_doanalyze) {
            const auto result = _analyze(id);
            return { cachedLoc, result.second, result.first };
        }
        std::pair<const std::string, const TokenType> result = { id, TokenType::identifier };
        return { cachedLoc, result.second, result.first };
    }

    Token Lexer::simpleStr() {
        TokenLoc cachedLoc { loc() };
        advance();
        std::string str {};
        while (!eof() && !isQuoteChar()) {
            str.push_back(current());
            advance();
            if (current() == '\\') {
                str.push_back(current());
                advance();
                str.push_back(current());
                advance();
            }
        }
        if (isQuoteChar())
            advance();
        else
            return _tkn(TokenType::invalid, "");
        return { cachedLoc, TokenType::simpleString, str };
    }
    Token Lexer::number() {
        TokenLoc cachedLoc { loc() };
        std::string num {};
        if (current() == '0' && peek() == 'x') {
            next(2);
            while ((!isNumTermin() && isHexDigitChar()) || current() == '_') {
                if (current() == '_') {
                    next();
                    continue;
                }
                num.push_back(current());
                advance();
            }
            return { cachedLoc, TokenType::numIntHex, num };
        }
        bool dot { false };
        while ((!isNumTermin() && !isNumSuffix() && isDigitChar()) || current() == '_') {
            if (current() == '_') {
                next();
                continue;
            }
            num.push_back(current());
            next();
            if (isDotChar()) {
                if (dot)
                    return { cachedLoc, TokenType::numFloating, num };
                dot = true;
                num.push_back('.');
                next();
            }
        }
        uint8_t flags {};
        while (!eof() && isNumSuffix()) {
            switch (current()) {
                case 'u':
                    flags |= 0b00001;
                    break;
                case 'b':
                    flags |= 0b00010;
                    break;
                case 'w':
                    flags |= 0b00100;
                    break;
                case 'd':
                    flags |= 0b01000;
                    break;
                case 'c':
                    flags |= 0b10000;
                    break;
            }
            pos++;
        }
        const TokenType num_type {
            flags == 0b00000 ? TokenType::numIntDec : (
            flags == 0b00001 ? TokenType::numUIntDec : (
            flags == 0b00010 ? TokenType::numByteDec : (
            flags == 0b00011 ? TokenType::numUByteDec : (
            flags == 0b00100 ? TokenType::numShortDec : (
            flags == 0b00101 ? TokenType::numUShortDec : (
            flags == 0b01000 ? TokenType::numInt32Dec : (
            flags == 0b01001 ? TokenType::numUInt32Dec : (
            flags == 0b10000 ? TokenType::numWideChar : (
            flags == 0b10001 ? TokenType::numWideUChar : TokenType::invalid)))))))))
        };
        if (num_type == TokenType::invalid) {
            report(Error::lexDomain, "Unknown integer type suffix", { pos, 1, line, line }, { pos, 1 });
        }
        return { cachedLoc, dot ? TokenType::numFloating : num_type, num };
    }

    std::pair<const std::string, const TokenType> Lexer::_analyze(const std::string& str) {
        Lexer lexer(str);
        lexer._doanalyze = false;
        return { str, lexer.drive().type };
    }
    
    uint32_t utf8_to_utf32(uint8_t* text);
    Token Lexer::drive() {
        while (isSpaceChar()) {
            advance();
        }
        
        if (eof())
            return _tkn(TokenType::invalid, "");

        comments();
        
        if (current() == 'W' && peek() == '\'') {
            pos += 2;
            uint8_t utf8[5] { 0, 0, 0, 0, 0 };
            int l {};
            while (!eof() && current() != '\'') {
                if (l >= 4) {
                    report(Error::lexDomain, "Wide character literal larger than 4 bytes", { pos, 1, line, line }, { pos, 1 });
                    return _tkn(TokenType::invalid, "");
                }
                utf8[l] = (unsigned char)current();
                pos++; l++;
            }
            const uint64_t c = utf8_to_utf32(utf8);
            if (current() != '\'') {
                report(Error::lexDomain, "Expected single quote at end of wide character literal", { pos, 1, line, line }, { pos, 1 });
                return _tkn(TokenType::invalid, "");
            }
            pos++;
            return _tkn(TokenType::numWideChar, std::to_string(c));
        }

        if (isDigitChar())
            return number();
        
        if (isIdentifierStart())
            return multichar();
        
        if (isQuoteChar())
            return simpleStr();
        
        // Tries to lex a token
        Token tmp { _tkn(TokenType::invalid, "") };
        switch (current()) {
            case '\'': {
                pos++;
                if (current() == '\n') {
                    report(Error::lexDomain, "Unexpected newline in character literal", { pos - 1, 1, line, line }, { pos - 1, 1 }, "Did you mean to use '\\n' instead?");
                    break;
                }
                char c = current();
                if (c == '\\') {
                    pos++;
                    switch (current()) {
                        case 'n':
                            c = '\n';
                            break;
                        case 'e':
                            c = '\e';
                            break;
                        case 't':
                            c = '\t';
                            break;
                        case '\"':
                            c = '\"';
                            break;
                        case '\'':
                            c = '\'';
                            break;
                        default:
                            break;
                    }
                }
                pos++;
                if (current() == '\'') {
                    tmp = _tkn(TokenType::numByteDec, std::to_string(c));
                } else {
                    report(Error::lexDomain, "Missing single quote in character literal", { pos, 1, line, line }, { pos, 1 }, "Replace this position with a single quote");
                }
                break;
            }
            case '#': {
                const size_t tpos = pos;
                pos++;
                std::string prepr_macro;
                while (!eof() && isalpha(current())) {
                    prepr_macro.push_back(current());
                    pos++;
                }
                pos--;
                if (prepr_macro == "line") {
                    tmp = _tkn(TokenType::numUIntDec, std::to_string(line));
                } else if (prepr_macro == "define") {
                    next();
                    if (current() != ' ') {
                        report(Error::lexDomain, "Expected space after define macro", { pos, 1, line, line }, { pos, line});
                        break;
                    }
                    next();
                    const std::string name = multichar(false).contents;
                    if (current() == '\n') {
                        tmp = _tkn(TokenType::macro, "#define " + name);
                        _defines.insert({ name, "" });
                        break;
                    } else if (current() == ' ') {
                        next();
                        std::string value;
                        while (!eof() && current() != '\n') {
                            value.push_back(current());
                            next();
                        }
                        tmp = _tkn(TokenType::macro, "#define " + name + ' ' + value);
                        _defines.insert({ name, value });
                        break;
                    }
                } else if (prepr_macro == "undef") {
                    next();
                    if (current() != ' ') {
                        report(Error::lexDomain, "Expected space after undef macro", { pos, 1, line, line }, { pos, line});
                        break;
                    }
                    next();
                    const std::string name = multichar(false).contents;
                    if (current() != '\n') {
                        report(Error::lexDomain, "Expected newline at end of undef macro", { pos, 1, line, line }, { pos, line});
                        break;
                    }
                    tmp = _tkn(TokenType::macro, "#undef " + name);
                    _defines.erase(name);
                }
                else if (prepr_macro == "ifdef") {
                    next();
                    if (current() != ' ') {
                        report(Error::lexDomain, "Expected space after ifdef macro", { pos, 1, line, line }, { pos, line});
                        break;
                    }
                    next();
                    const std::string name = multichar(false).contents;
                    if (current() != '\n') {
                        report(Error::lexDomain, "Expected newline at end of ifdef macro", { pos, 1, line, line }, { pos, line});
                        break;
                    }
                    tmp = _tkn(TokenType::macro, "#ifdef " + name);
                    if (std::find_if(_defines.begin(), _defines.end(), [name](auto pair) -> bool { return pair.first == name; }) == _defines.end()) {
                        while (!(current() == '#' && peek() == 'e')) {
                            advance();
                        }
                        pos--;
                    }
                } else if (prepr_macro == "ifndef") {
                    next();
                    if (current() != ' ') {
                        report(Error::lexDomain, "Expected space after ifndef macro", { pos, 1, line, line }, { pos, line});
                        break;
                    }
                    next();
                    const std::string name = multichar(false).contents;
                    if (current() != '\n') {
                        report(Error::lexDomain, "Expected newline at end of ifndef macro", { pos, 1, line, line }, { pos, line});
                        break;
                    }
                    tmp = _tkn(TokenType::macro, "#ifdef " + name);
                    if (std::find_if(_defines.begin(), _defines.end(), [name](auto pair) -> bool { return pair.first == name; }) != _defines.end()) {
                        while (!(current() == '#' && peek() == 'e')) {
                            advance();
                        }
                        pos--;
                    }
                } else if (prepr_macro == "endif") {
                    tmp = _tkn(TokenType::macro, "#endif");
                    break;
                } else if (prepr_macro == "include") {
                    next();
                    if (current() != ' ') {
                        report(Error::lexDomain, "Expected space after include macro", { pos, 1, line, line }, { pos, line});
                        break;
                    }
                    next();
                    
                    if (current() == '<') {
                        pos++;
                        std::string includePath;
                        while (!eof() && current() != '>') {
                            includePath.push_back(current());
                            pos++;
                        }
                        std::string buffer;
                        read("/Users/ethanuppal/Programming/floral-src/include/" + includePath, buffer);
                        const std::string des = "#include <" + includePath + '>';
                        tmp = _tkn(TokenType::macro, des);
                        code.erase(code.begin() + tpos, code.begin() + tpos + des.size());
                        line--;
                        pos = tpos;
                        if (!buffer.empty()) {
                            code.insert(pos, buffer);
                        }
                        pos--;
                        break;
                    } else {
                        const std::string name = simpleStr().contents;
                        
                        std::string buffer;
                        read(name, buffer);
                        if (!buffer.empty()) {
                            code.insert(pos, buffer);
                        }
                        const std::string des = "#include \"" + name + '\"';
                        tmp = _tkn(TokenType::macro, des);
                        code.erase(code.begin() + tpos, code.begin() + tpos + des.size());
                        line--;
                        pos = tpos;
                        break;
                    }
                }
                else {
                    report(Error::lexDomain, "Unknown preproccessor macro", { tpos, pos - tpos, line, line }, { tpos, 1 });
                }
                break;
            }
            case '(':
                tmp = _tkn(TokenType::leftParenthesis, "(");
                break;
            case ')':
                tmp = _tkn(TokenType::rightParenthesis, ")");
                break;
            case '{':
                tmp = _tkn(TokenType::leftBrace, "{");
                break;
            case '}':
                tmp = _tkn(TokenType::rightBrace, "}");
                break;
            case '[':
                tmp = _tkn(TokenType::leftBracket, "[");
                break;
            case ']':
                tmp = _tkn(TokenType::rightBracket, "]");
                break;
            case ';':
                tmp = _tkn(TokenType::semicolon, ";");
                break;
            case ',':
                tmp = _tkn(TokenType::comma, ",");
                break;
            case '+':
                if (peek() == '+') {
                    tmp = _tkn(TokenType::inc, "++");
                    next();
                } else {
                    tmp = _tkn(TokenType::plus, "+");
                }
                break;
            case '-':
                if (peek() == '>') {
                    tmp = _tkn(TokenType::arrow, "->");
                    next();
                } else if (peek() == '-') {
                    tmp = _tkn(TokenType::dec, "--");
                    next();
                } else {
                    tmp = _tkn(TokenType::minus, "-");
                }
                break;
            case '*':
                if (peek() == '=') {
                    tmp = _tkn(TokenType::power, "**");
                    next();
                } else {
                    tmp = _tkn(TokenType::multiply, "*");
                }
                break;
            case '/':
                tmp = _tkn(TokenType::divide, "/");
                break;
            case '=':
                if (peek() == '=') {
                    tmp = _tkn(TokenType::equal, "==");
                    next();
                } else
                    tmp = _tkn(TokenType::assign, "=");
                break;
            case '!':
                if (peek() == '=') {
                    tmp = _tkn(TokenType::unequal, "!=");
                    next();
                } else
                    tmp = _tkn(TokenType::notOp, "!");
                break;
            case '&':
                tmp = _tkn(TokenType::andOp, "&");
                break;
            case '|':
                tmp = _tkn(TokenType::orOp, "|");
                break;
            case '%':
                tmp = _tkn(TokenType::modulus, "%");
                break;
            case '^':
                tmp = _tkn(TokenType::xorOp, "^");
                break;
            case ':':
                tmp = _tkn(TokenType::colon, ":");
                break;
            case '<':
                if (peek() == '=') {
                    tmp = _tkn(TokenType::lessEqual, "<=");
                    next();
                } if (peek() == '-') {
                    tmp = _tkn(TokenType::backarrow, "<-");
                    next();
                } else {
                    tmp = _tkn(TokenType::less, "<");
                }
                break;
            case '>':
                if (peek() == '=') {
                    tmp = _tkn(TokenType::greaterEqual, ">=");
                    next();
                } else
                    tmp = _tkn(TokenType::greater, ">");
                break;
            default:
                break;
        }
        advance();
        return tmp;
    }

    std::vector<Token> Lexer::lex() {
        std::vector<Token> tkns {};
        comments();
        Token tkn { drive() };
        while (tkn.type != TokenType::invalid) {
            tkns.push_back(tkn);
            if (eof()) {
                if (tkn.type != TokenType::invalid && !(tkn == tkns.back()))
                    tkns.push_back(tkn);
                return tkns;
            } else {
                tkn = drive();
            }
        }
        return tkns;
    }

    void Lexer::comments() {
        if (pos + 2 < code.size() && current() == '/') {
            switch (peek()) {
                case '/': {
                    pos += 2;
                    while (!eof() && current() != '\n') pos++;
                    pos++; line++;
                    break;
                }
                case '*': {
                    pos += 2;
                    while (!eof(-1) && current() != '*' && peek() != '/') advance();
                    pos += 2;
                    break;
                }
                default:
                    return;
            }
        }
    }

    // Modified from https://codereview.stackexchange.com/questions/197548/convert-utf8-string-to-utf32-string-in-c
    uint32_t utf8_to_utf32(uint8_t* text) {
        uint32_t c {};
        size_t i = 0;

        if ((text[i] & 0b10000000) == 0) {
            // 1 byte code point, ASCII
            c = (text[i] & 0b01111111);
        }
        else if ((text[i] & 0b11100000) == 0b11000000) {
            // 2 byte code point
            c = (text[i] & 0b00011111) << 6 | (text[i + 1] & 0b00111111);
        }
        else if ((text[i] & 0b11110000) == 0b11100000) {
            // 3 byte code point
            c = (text[i] & 0b00001111) << 12 | (text[i + 1] & 0b00111111) << 6 | (text[i + 2] & 0b00111111);
        }
        else {
            // 4 byte code point
            c = (text[i] & 0b00000111) << 18 | (text[i + 1] & 0b00111111) << 12 | (text[i + 2] & 0b00111111) << 6 | (text[i + 3] & 0b00111111);
        }
        
        return c;
    }
}
