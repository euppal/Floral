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
        return current() == 'u' || current() == 'b' || current() == 'w' || current() == 'd';
    }
    bool Lexer::isQuoteChar() {
        return current() == '\"';
    }
    bool Lexer::isDotChar() {
        return current() == '.';
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
        return { cachedLoc, TokenType::identifier, id };
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
                    flags |= 0b0001;
                    break;
                case 'b':
                    flags |= 0b0010;
                    break;
                case 'w':
                    flags |= 0b0100;
                    break;
                case 'd':
                    flags |= 0b1000;
                    break;
            }
            pos++;
        }
        static const TokenType num_types[16] {
            TokenType::numIntDec,
            TokenType::numUIntDec,
            TokenType::numByteDec,
            TokenType::numUByteDec,
            TokenType::numShortDec,
            TokenType::numUShortDec,
            TokenType::invalid,
            TokenType::invalid,
            TokenType::numInt32Dec,
            TokenType::numUInt32Dec,
            TokenType::invalid,
            TokenType::invalid,
            TokenType::invalid,
            TokenType::invalid,
            TokenType::invalid,
            TokenType::invalid,
        };
        const TokenType num_type { num_types[flags] };
        if (num_type == TokenType::invalid) {
            report(Error::lexDomain, "Unknown integer type suffix", { pos, 1, line, line }, { pos, 1 });
        }
        return { cachedLoc, dot ? TokenType::numFloating : num_type, num };
    }
    
    Token Lexer::drive() {
        while (isSpaceChar()) {
            advance();
        }
        
        if (eof())
            return _tkn(TokenType::invalid, "");

        comments();

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
                const char c = current();
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
}
