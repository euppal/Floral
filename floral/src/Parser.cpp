//
//  Parser.cpp
//  floral
//
//  Created by Ethan Uppal on 6/29/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Parser.hpp"
#include "AST.hpp"
#include "Token.hpp"
#include <vector>

namespace Floral {
void Parser::report(Error::Domain domain, const std::string& text, TextRegion loc) {
    Error err {domain, text, loc};
    _errors.push_back(err);
}
void Parser::reset() {
    _errors.clear();
    index = 0;
}

Token Parser::current() {
    // Returns the current token
    return tokens.at(index);
}
Token Parser::peek() {
    // Peeks at the next tokem
    return tokens.at(index + 1);
}
void Parser::advance() {
    ++index;
}
bool Parser::eof() {
    return index == tokens.size();
}
Token Parser::match(TokenType type, const std::string& withinCtx = "") {
    if (current().type == type) {
        Token t { current() };
        advance();
        return t;
    }
    report(Error::parseDomain, "Expected " + tokenTypeDescription(type) + " but received " + tokenTypeDescription(current().type) + withinCtx, TextRegion(current()));
    return { {0, 0}, TokenType::invalid, "" };
}
void Parser::synchronize() {
    while (!(eof() || current().isDeclarator())) {
        advance();
    }
}

Type* Parser::type() {
    if (current().isType()) {
        auto cache { new Token(current().loc, current().type, current().contents) };
        advance();
        
        if (current().type == TokenType::colon) {
            advance();
            return new Type(new Type(cache), type());
        }
        return new Type(cache);
    }
    switch (current().type) {
        case TokenType::andOp:
            advance();
            return new Type(type(), true);
        case TokenType::leftBracket: {
            advance();
            auto t { type() };
            if (match(TokenType::rightBracket, " in array type").isInvalid()) return nullptr;
            return new Type(t, false);
        }
        case TokenType::leftParenthesis: {
            Type* tuple[MAX_TUPLE_SIZE];
            size_t i {};
            while (!eof() && current().type != TokenType::rightParenthesis) {
                tuple[i] = type();
                if (current().type == TokenType::comma)
                    advance();
                ++i;
            }
            if (match(TokenType::rightParenthesis, " in tuple type").isInvalid()) return nullptr;
            return new Type(tuple, i);
        }
        default:
            report(Error::parseDomain, "Unknown type signature", TextRegion(current()));
            return nullptr;
    }
}
Initializer* Parser::initializer() {
    switch (current().type) {
        case TokenType::leftParenthesis: {
            advance();
            if (current().type == TokenType::rightParenthesis) {
                advance();
                return new ZeroInitializer();
            }
            else {
                auto val { expr() };
                if (match(TokenType::rightParenthesis, " in direct initializer").isInvalid()) return nullptr;
                return new DirectInitializer(val);
            }
        }
        case TokenType::assign: {
            advance();
            auto val { expr() };
            return new CopyInitializer(val);
        }
        default:
            return nullptr;
    }
}
Function* Parser::function() {
    Token start { current() };
    advance();
    
    Token name { match(TokenType::identifier, " in function name") };
    if (name.contents == "_main") {
        report(Error::generalRejectionDomain, "Cannot declare a function named '_main' as that is reserved as the entry point for execution", { name });
        return nullptr;
    }
    if (match(TokenType::leftParenthesis, " in function").isInvalid()) return nullptr;
    // Parameters
    Function::Parameters parameters;
    while (!eof() && current().type != TokenType::rightParenthesis) {
        auto pname = match(TokenType::identifier, " in parameter");
        match(TokenType::colon, " in parameter");
        auto ptype = type();
        parameters.push_back({ pname, ptype });
    }
    if (match(TokenType::rightParenthesis, " in function").isInvalid()) return nullptr;
    // Return type
    Type* rtype { new Type(Token::invalid) };
    if (current().type == TokenType::colon) {
        advance();
        rtype = type();
    }
    if (match(TokenType::leftBrace, " at start of function body").isInvalid()) return nullptr;
    // Statement body
    std::vector<Statement*> body {};
    while (!eof() && current().type != TokenType::rightBrace) {
        if (auto stm = statement())
            body.push_back(stm);
        else
            return nullptr;
    }
    Token end { match(TokenType::rightBrace, " at end of function body") };
    TextRegion loc { start, end };
    Function* func { new Function(loc, name, parameters, rtype) };
    func->setBody(body);
    return func;
}
GlobalDeclaration* Parser::global() {
    Token start { current() };
    advance();
    Token name { match(TokenType::identifier, " in global constant declaration") };
    Type* gtype { new Type(Token::invalid) };
    if (current().type == TokenType::colon) {
        advance();
        gtype = type();
    }
    Initializer* init { initializer() };
    // Globals must be initialized
    if (!init) return report(Error::parseDomain, "Global constant declaration missing initializer", { start, current() }), nullptr;
    // Can't do type inference with a zero initializer
    if (init->type == Initializer::zero && gtype->isIncomplete()) return report(Error::parseDomain, "Cannot zero initialize a global constant without a type specifier", { start, current() }), nullptr;
    Token end { match(TokenType::semicolon, " at end of global constant declaration") };
    TextRegion loc { start, end };
    return new GlobalDeclaration(loc, name, gtype, init);
}
LetDeclaration* Parser::let() {
    Token start { current() };
    advance();
    Token name { match(TokenType::identifier, " in local constant declaration") };
    Type* ltype { new Type(Token::invalid) };
    if (current().type == TokenType::colon) {
        advance();
        ltype = type();
    }
    Initializer* init { initializer() };
    // Local constants must be initialized
    if (!init) return report(Error::parseDomain, "Local constant declaration missing initializer", { start, current() }), nullptr;
    // Can't do type inference with a zero initializer
     if (init->type == Initializer::zero && ltype->isIncomplete()) return report(Error::parseDomain, "Cannot zero initialize a global constant without a type specifier", { start, current() }), nullptr;
    Token end { match(TokenType::semicolon, " at end of local constant declaration") };
    TextRegion loc { start, end };
    return new LetDeclaration(loc, name, ltype, init);
}
VarDeclaration* Parser::var() {
    Token start { current() };
    advance();
    Token name { match(TokenType::identifier, " in local variable declaration") };
    Type* vtype { new Type(Token::invalid) };
    if (current().type == TokenType::colon) {
        advance();
        vtype = type();
    }
    if (current().type == TokenType::semicolon) {
        Token end { current() };
        TextRegion loc { start, end };
        return new VarDeclaration(loc, name, vtype, nullptr);
    }
    Initializer* init { initializer() };
    // Can't do type inference with a zero initializer
    if (init && init->type == Initializer::zero && vtype->isIncomplete()) return report(Error::parseDomain, "Cannot zero initialize a global constant without a type specifier", { start, current() }), nullptr;
    Token end { match(TokenType::semicolon, " at end of local variable declaration") };
    TextRegion loc { start, end };
    return new VarDeclaration(loc, name, vtype, init);
}
CallStatement* Parser::callStm() {
    Call* call { this->call() };
    if (!call) return nullptr;
    Token end { match(TokenType::semicolon, " in call statement") };
    if (end.isInvalid()) return nullptr;
    TextRegion loc { call->name, end };
    return new CallStatement(loc, call);
}
EmptyStatment* Parser::emptyStm() {
    Token semicolon { match(TokenType::semicolon, " for empty statement") };
    TextRegion loc { semicolon };
    return new EmptyStatment(loc);
}
Literal* Parser::literal() {
    switch (current().type) {
        case TokenType::boolTrue:
        case TokenType::boolFalse:
            return new Literal({ current(), current() }, Literal::LType::boolean, current());
            break;
        case TokenType::simpleString:
            return new Literal({ current(), current() }, Literal::LType::simpleString, current());
            break;
        case TokenType::numFloating:
            return new Literal({ current(), current() }, Literal::LType::floatingPointNumber, current());
            break;
        case TokenType::numIntHex:
            return new Literal({ current(), current() }, Literal::LType::hexadecimalInteger, current());
            break;
        case TokenType::numIntDec:
            return new Literal({ current(), current() }, Literal::LType::decimalInteger, current());
            break;
        default:
            return nullptr;
    }
}
FlatExpression* Parser::expr() {
    std::vector<Expression*> components;
    Token start { current() };
    start.print();
    if (start.isLiteral()) {
        components.push_back(literal());
        advance();
    } else if (start.isOperator()) {
        components.push_back(new OperatorComponentExpression(start));
        advance();
    } else if (auto cll = call())
        components.push_back(cll);
    else {
        return nullptr;
    }
    if (auto next = expr())
        for (auto component: next->components())
            components.push_back(component);
    Token end { current() };
    return new FlatExpression({ start, end }, components);
}
Call* Parser::call() {
    if (current().type != TokenType::identifier)
        return nullptr;
    Token name { match(TokenType::identifier, " in call") };
    if (name.isInvalid()) return nullptr;
    advance(); advance(); // advance past name and left parenthesis
    std::vector<Expression*> arguments;
    arguments.push_back(expr());
    Token end { match(TokenType::rightParenthesis, " in call") };
    if (end.isInvalid()) return nullptr;
    TextRegion loc { name, end };
    return new Call(loc, name, arguments);
}
LiteralStatement* Parser::literalStm() {
    Token start { current() };
    Literal* lit { literal() };
    advance();
    Token end { match(TokenType::semicolon, " in literal statement") };
    return new LiteralStatement({ start, end }, lit);
}
Statement* Parser::statement() {
    if (current().isLiteral())
        return literalStm();
    switch (current().type) {
        case TokenType::semicolon:
            return emptyStm();
        case TokenType::identifier:
            if (peek().type == TokenType::leftParenthesis)
                return callStm();
            break;
        default:
            break;
    }
    return nullptr;
}

File* Parser::parse() {
    Token lastTkn { tokens.back() };
    TextRegion fileLoc { 0, lastTkn.end(), 1, lastTkn.line() };
    File *file { new File(fileLoc, path, std::vector<Node*>()) };
    while (!eof()) {
        switch (current().type) {
            case TokenType::func: {
                if (auto func = function())
                    file->insert(func);
                else
                    synchronize();
                }
                break;
            case TokenType::global: {
                if (auto gbl = global())
                    file->insert(gbl);
                else
                    synchronize();
                }
                break;
            case TokenType::let: {
                if (auto l = let())
                    file->insert(l);
                else
                    synchronize();
                }
                break;
            case TokenType::var: {
                if (auto v = var())
                    file->insert(v);
                else
                    synchronize();
                }
                break;
            default: {
                if (auto stm = statement())
                    file->insert(stm);
                else
                    synchronize();
                }
                break;
        }
    }
    return file;
}

Parser::Parser(std::vector<Token> &tokens): tokens(tokens), index(0) {}

bool Parser::hasErrors() const {
    return !_errors.empty();
}
const std::vector<Error>& Parser::errors() const {
    return _errors;
}
}
