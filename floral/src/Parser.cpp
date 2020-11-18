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
#include "LexerKeywords.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

namespace Floral {
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
    size_t Parser::isAhead(TokenType goal, const std::vector<TokenType>& allowed) {
        size_t i = index;
        while (i < tokens.size()) {
            if (tokens[i].type == goal) {
                return i - index;
            }
            if (std::find(allowed.begin(), allowed.end(), tokens[i].type) == allowed.end()) {
                return 0;
            }
            i++;
        }
        return 0;
    }
    void Parser::pacman() {
        ++index;
    }
    bool Parser::eof() {
        return index == tokens.size();
    }
    Token Parser::match(TokenType type, const std::string& withinCtx, const std::string& fix) {
        if (eof()) {
            index--;
            report(Error::parseDomain, "Unexpected end of file", current().loc.filename, TextRegion(current()), { current().pos(), current().contents.size() });
            return { current().loc, TokenType::invalid, "" };
        }
        if (current().type == type) {
            Token t { current() };
            pacman();
            return t;
        }
        report(
               Error::parseDomain,
               "Expected " + tokenTypeDescription(type) + " but received " + tokenTypeDescription(current().type) + withinCtx,
               current().loc.filename,
               TextRegion(current()),
               { current().pos(), current().contents.size() },
               fix
        );
        return { current().loc, TokenType::invalid, "" };
    }
    void Parser::synchronize() {
        _synchr_count++;
        std::string fix;
        const auto similars {similarTo(current().contents, true)};
        if (!similars.empty()) {
            fix += "Did you mean '" + similars.front().first + "' instead?";
            if (similars.size() > 1) {
                fix += " Other options include ";
                for (auto iter = similars.begin() + 1; iter != similars.end(); iter++) {
                    fix += '\x27' + (*iter).first + '\x27';
                    if (iter + 2 < similars.end()) {
                        fix += ", ";
                    } else if (iter + 1 < similars.end()) {
                        fix += " and ";
                    }
                }
            }
        }
        report(
               Error::parseDomain,
               current().contents + " is not a declarator",
               current().loc.filename,
               {current()},
               { current().pos(), current().contents.size() },
               fix
        );
        while (!(eof() || current().isDeclarator())) {
            pacman();
        }
    }

    Type* Parser::type() {
        bool isConst{};
        if (current().type == TokenType::const_) isConst = true, pacman();
        auto aliased = Type::typealiases.find(current().contents);
        if (aliased != Type::typealiases.end()) {
            pacman();
            return aliased->second;
        }
        if (current().type == TokenType::struct_) {
            pacman();
            const auto structName = match(TokenType::identifier, " in struct type");
            if (structName.isInvalid()) return nullptr;
            return new Type(0, structName.contents, isConst);
        }
        if (current().isType()) {
            auto t { new Type(new Token(current().loc, current().type, current().contents), isConst) };
            pacman();
            
            if (current().type == TokenType::arrow) {
                pacman();
                return new Type(t, type(), isConst);
            } else if (current().type == TokenType::leftBracket) {
                pacman();
                const size_t l = strtoul(current().contents.c_str(), NULL, 10);
                pacman();
                if (match(TokenType::rightBracket, " in array type").isInvalid()) return nullptr;
                return new Type(t, l, true);
            }
            return t;
        }
        switch (current().type) {
            case TokenType::bit_and:
                pacman();
                if (current().isType() || current().type == TokenType::leftParenthesis || current().type == TokenType::leftBracket || current().type == TokenType::bit_and || current().type == TokenType::const_ || current().type == TokenType::struct_) {
                    auto t = type();
                    return new Type(t, true, isConst);
                } else {
                    auto lhs {new Type(type(), true, isConst)};
                    if (match(TokenType::arrow).isInvalid()) return nullptr;
                    return new Type(lhs, type(), isConst);
                }
            case TokenType::leftBracket: {
                pacman();
                auto t { type() };
                if (match(TokenType::rightBracket, " in array type").isInvalid()) return nullptr;
                auto arrt {new Type(t, false, isConst)};
                if (current().type == TokenType::arrow) {
                    pacman();
                    return new Type(arrt, type(), isConst);
                }
                return arrt;
            }
            case TokenType::leftParenthesis: {
                Type* tuple[MAX_TUPLE_SIZE];
                size_t i {};
                pacman();
                while (!eof() && current().type != TokenType::rightParenthesis) {
                    tuple[i] = type();
                    if (current().type == TokenType::comma)
                        pacman();
                    ++i;
                    if (i >= MAX_TUPLE_SIZE) {
                        report(
                               Error::parseDomain,
                               "Tuples must be less than " + std::to_string(MAX_TUPLE_SIZE) + " in length",
                               current().loc.filename,
                               TextRegion(current()),
                               { current().pos(), current().contents.size() }
                        );
                        return nullptr;
                    }
                }
                if (match(TokenType::rightParenthesis, " in tuple type").isInvalid()) return nullptr;
                auto t { i == 1 ? tuple[0] : new Type(tuple, i, isConst) };
                if (current().type == TokenType::arrow) {
                    pacman();
                    return new Type(t, type(), isConst);
                }
                return t;
            }
            default: {
                const auto similars {similarTo(current().contents)};
                std::string fix;
                if (!similars.empty()) {
                    fix += "Did you mean '" + similars.front().first + "' instead?";
                    if (similars.size() > 1) {
                        fix += " Other options include ";
                        for (auto iter = similars.begin() + 1; iter != similars.end(); iter++) {
                            fix += '\x27' + (*iter).first + '\x27';
                            if (iter + 2 < similars.end()) {
                                fix += ", ";
                            } else if (iter + 1 < similars.end()) {
                                fix += " and ";
                            }
                        }
                    }
                }
                report(
                       Error::parseDomain,
                       "Unknown type signature",
                       current().loc.filename,
                       TextRegion(current()),
                       { current().pos(), current().contents.size() },
                       fix
                );
                return nullptr;
            }
        }
    }
    Initializer* Parser::initializer() {
        switch (current().type) {
            case TokenType::leftParenthesis: {
                pacman();
                if (current().type == TokenType::rightParenthesis) {
                    pacman();
                    return new ZeroInitializer();
                }
                else {
                    auto val { expr() };
                    if (match(TokenType::rightParenthesis, " in direct initializer").isInvalid()) return nullptr;
                    return new DirectInitializer(val);
                }
            }
            case TokenType::assign: {
                pacman();
                auto val { expr() };
                return new CopyInitializer(val);
            }
            default:
                return nullptr;
        }
    }
    StructConstructor* Parser::structConstr() {
        if (match(TokenType::identifier, " in struct constructor").isInvalid()) return nullptr;
        if (match(TokenType::leftParenthesis, " in struct constructor").isInvalid()) return nullptr;
        
        Function::Parameters params;
        // parse input args
        while (!eof() && current().type != TokenType::rightParenthesis) {
            auto pname = match(TokenType::identifier, " in struct constructor parameter");
            match(TokenType::colon, " in struct constructor parameter");
            auto ptype = type();
            params.push_back({ pname, ptype });
            if (current().type != TokenType::rightParenthesis) {
                if (match(TokenType::comma, " separating struct constructor parameters").isInvalid()) return nullptr;
            }
        }
        
        if (match(TokenType::rightParenthesis, " in struct constructor").isInvalid()) return nullptr;
        
        std::vector<std::pair<Token, Expression*>> inits;
        if (current().type == TokenType::colon) {
            pacman();
            // parse direct inits
            while (!eof()) {
                auto dname = match(TokenType::identifier, " in struct constructor initialization sequence");
                if (dname.isInvalid()) return nullptr;
                if (match(TokenType::assign, " in struct constructor initialization sequence").isInvalid()) return nullptr;
                Expression* e = expr();
                if (!e) return nullptr;
                inits.push_back({ dname, e });
                if (current().type != TokenType::comma) {
                    break;
                }
            }
        }
        
        Statement* after = statement();
        return new StructConstructor(params, inits, after);
    }
    Declaration* Parser::function() {
        const Token start { match(TokenType::func) };
        
        const Token name { match(TokenType::identifier, " in function name") };
        if (name.isInvalid()) return nullptr;
        /*
         * Match generics foo<Type: Behavior, Type2: Behavior2 & Behavior3, etc...>(args);
         */
        
        if (match(TokenType::leftParenthesis, " in function").isInvalid()) return nullptr;
        // Parameters
        Function::Parameters parameters;
        while (!eof() && current().type != TokenType::rightParenthesis) {
            if (peek().type != TokenType::colon) {
                parameters.push_back({ *Token::invalid, type() });
                if (current().type != TokenType::rightParenthesis) {
                    if (match(TokenType::comma, " separating function parameters").isInvalid()) return nullptr;
                }
                continue;
            }
            auto pname = match(TokenType::identifier, " in parameter");
            match(TokenType::colon, " in parameter");
            auto ptype = type();
            parameters.push_back({ pname, ptype });
            if (current().type != TokenType::rightParenthesis) {
                if (match(TokenType::comma, " separating function parameters").isInvalid()) return nullptr;
            }
        }
        if (match(TokenType::rightParenthesis, " in function").isInvalid()) return nullptr;
        // Return type
        Type* rtype { new Type(Token::invalid, true) };
        if (current().type == TokenType::colon) {
            pacman();
            const Token typeStart = current();
            rtype = type();
            if (!rtype) {
                return nullptr;
            }
        }
        if (current().type == TokenType::semicolon) {
            const Token end { current() };
            pacman();
            TextRegion loc { start, end };
            return new FunctionForwardDeclaration(loc, name, parameters, rtype);
        } else if (current().type == TokenType::leftBrace) {
            pacman();
        } else {
            report(
                   Error::parseDomain,
                   "Unexpected token at end of function parameters",
                   current().loc.filename,
                   { current(), current() },
                   { current().pos(), current().contents.size() },
                   "Try inserting a colon: ': " + current().contents + '\''
            );
            return nullptr;
        }
        // Statement body
        std::vector<Statement*> body {};
        while (!eof() && current().type != TokenType::rightBrace) {
            if (auto stm = statement()) {
                body.push_back(stm);
            } else {
                report(
                       Error::parseDomain,
                       "Something wrong with function body - cannot parse statement",
                       current().loc.filename,
                       { start, current() },
                       { current().pos(), current().contents.size() }
                );
                synchronize();
            }
        }
        Token end { match(TokenType::rightBrace, " at end of function body") };
        TextRegion loc { start, end };
        Function* func { new Function(loc, name, parameters, rtype) };
        for (auto stm: body) func->insert(stm);
        func->staticAllocationSize = 0;
        func->setInline(_attrs.find(Function::Attributes::inline_) != _attrs.end());
        func->setStatic(_attrs.find(Function::Attributes::static_) != _attrs.end());
        _attrs.clear();
        return func;
    }
    Declaration* Parser::global() {
        const Token start { current() };
        pacman();
        Token name { match(TokenType::identifier, " in global constant declaration") };
        Type* gtype { new Type(Token::invalid) };
        if (current().type == TokenType::colon) {
            pacman();
            gtype = type();
            if (current().type == TokenType::semicolon) {
                const Token end { current() };
                pacman();
                return new GlobalForwardDeclaration({ start, end }, name, gtype);
            }
        }
        Initializer* init { initializer() };
        // Globals must be initialized
        if (!init) {
            report(
                   Error::parseDomain,
                   "Global constant declaration missing initializer",
                   start.loc.filename,
                   { start, current() },
                   { current().pos(), current().contents.size() }
            );
            return nullptr;
        }
        // Can't do type inference with a zero initializer
        if (init->type == Initializer::zero && gtype->isIncomplete()) {
            report(
                   Error::parseDomain,
                   "Cannot zero initialize a global constant without a type specifier",
                   start.loc.filename,
                   { start, current() },
                   { current().pos(), current().contents.size() }
            );
            return nullptr;
        }
        const Token end { match(TokenType::semicolon, " at end of global constant declaration") };
        const TextRegion loc { start, end };
        return new GlobalDeclaration(loc, name, gtype, init);
    }
    StructDeclaration* Parser::structdef() {
        const Token start = match(TokenType::struct_, " at start of struct");
        if (start.isInvalid()) return nullptr;
        const Token name = match(TokenType::identifier, " in struct name");
        if (match(TokenType::leftBrace, " in struct").isInvalid()) return nullptr;
        std::vector<Statement*> dataMembers;
        std::vector<Function*> functionMembers;
        std::vector<StructConstructor*> constructors;
        while (!eof() && current().type != TokenType::rightBrace) {
            switch (current().type) {
                case TokenType::var:
                    dataMembers.push_back(var());
                    break;
                case TokenType::let:
                    dataMembers.push_back(let());
                    break;
                case TokenType::identifier:
                    if (current().contents == name.contents) {
                        constructors.push_back(structConstr());
                    } else {
                        report(Error::parseDomain, "Unexpected identifier in function body", current().loc.filename, { current() }, { current().pos(), current().contents.size() });
                        return nullptr;
                    }
                    break;
                case TokenType::func: {
                    auto decl = function();
                    if (auto func = dynamic_cast<Function*>(decl)) {
                        functionMembers.push_back(func);
                    } else {
                        report(Error::parseDomain, "Function forward declarations are not allowed within a struct body", decl->_loc.path, decl->_loc, { decl->_loc.pos, 4 });
                        return nullptr;
                    }
                    break;
                }
                default:
                    synchronize();
                    return nullptr;
                    break;
            }
        }
        if (match(TokenType::rightBrace, " in struct").isInvalid()) return nullptr;
        const Token end = match(TokenType::semicolon, " at end of struct");
        if (end.isInvalid()) return nullptr;
        auto struct_ = new StructDeclaration({ start, end }, name, dataMembers, functionMembers, constructors);
        Type::structs.push_back(struct_);
        return struct_;
    }

    TypeAliasDeclaration* Parser::typealias() {
        const Token start = match(TokenType::typealias, " in type alias declaration");
        const Token name = match(TokenType::identifier, " in type alias declaration");
        if (name.isInvalid()) return nullptr;
        if (match(TokenType::assign, " in type alias declaration").isInvalid()) return nullptr;
        Type* t = type();
        if (match(TokenType::semicolon, " in type alias declaration").isInvalid()) return nullptr;
        const Token end = current();
        return new TypeAliasDeclaration({ start, end }, name, t);
    }
    NamespaceDeclaration* Parser::nmspace() {
        const Token start { current() };
        pacman();
        const Token name { match(TokenType::identifier, " as namespace name") };
        if (name.isInvalid()) return nullptr;
        if (match(TokenType::leftBrace, " in namespace").isInvalid()) return nullptr;
        std::vector<Node*> nodes;
        while (!eof() && current().type != TokenType::rightBrace) {
            switch (current().type) {
                case TokenType::static_:
                    _attrs.insert(Function::Attributes::static_);
                    pacman();
                    break;
                case TokenType::inline_:
                    _attrs.insert(Function::Attributes::inline_);
                    pacman();
                    break;
                case TokenType::macro:
                    pacman();
                    break;
                case TokenType::typealias: {
                    if (auto ta = typealias()) {
                        ta->alias().contents.insert(0, name.contents + NAMESPACE_DELIMITER);
                    } else {
                        synchronize();
                    }
                }
                case TokenType::func: {
                    if (auto decl = function()) {
                        if (auto func = dynamic_cast<Function*>(decl)) {
                            func->name().contents.insert(0, name.contents + NAMESPACE_DELIMITER);
                        } else if (auto ffunc = dynamic_cast<FunctionForwardDeclaration*>(decl)) {
                            ffunc->name().contents.insert(0, name.contents + NAMESPACE_DELIMITER);
                        }
                        nodes.push_back(decl);
                    } else {
                        synchronize();
                    }
                    break;
                }
                case TokenType::global: {
                    if (auto gbl = global()) {
                        if (auto ggbl = dynamic_cast<GlobalDeclaration*>(gbl)) {
                            ggbl->name.contents.insert(0, name.contents + NAMESPACE_DELIMITER);
                            nodes.push_back(ggbl);
                        } else if (auto fgbl = dynamic_cast<GlobalForwardDeclaration*>(gbl)) {
                            return nullptr;
                        }
                    } else {
                        return nullptr;
                    }
                    break;
                }
                default: {
                    synchronize();
                    break;
                }
            }
        }
        const Token end { match(TokenType::rightBrace, " at end of namespace") };
        if (end.isInvalid()) return nullptr;
        return new NamespaceDeclaration({ start, end }, name, nodes);
    }

    LetStatement* Parser::let(bool checkSemicolon) {
        const Token start { current() };
        pacman();
        const Token name { match(TokenType::identifier, " in local constant statement") };
        Type* ltype { new Type(Token::invalid) };
        if (current().type == TokenType::colon) {
            pacman();
            ltype = type();
        }
        const Token store { current() };
        Initializer* init { initializer() };
        // Local constants must be initialized
        if (!init) {
            report(
                   Error::parseDomain,
                   "Local constant statement missing initializer",
                   start.loc.filename,
                   { start, current() },
                   { store.pos(), store.contents.size() }
            );
            return nullptr;
        }
        // Can't do type inference with a zero initializer
        if (init->type == Initializer::zero && ltype->isIncomplete()) {
            report(
                   Error::parseDomain,
                   "Cannot zero initialize a local constant without a type specifier",
                   start.loc.filename,
                   { start, current() },
                   { current().pos(), current().contents.size() }
            );
            return nullptr;
        }
        if (checkSemicolon && match(TokenType::semicolon, " at end of local constant statement").isInvalid()) { return nullptr; }
        index--;
        Token end = current();
        index++;
        TextRegion loc { start, end };
        return new LetStatement(loc, name, ltype, init);
    }
    VarStatement* Parser::var(bool checkSemicolon) {
        Token start { current() };
        pacman();
        Token name { match(TokenType::identifier, " in local variable statement") };
        Type* vtype { new Type(Token::invalid) };
        if (current().type == TokenType::colon) {
            pacman();
            vtype = type();
        }
        if (current().type == TokenType::semicolon) {
            Token end { current() };
            pacman();
            TextRegion loc { start, end };
            return new VarStatement(loc, name, vtype, nullptr);
        }
        Initializer* init { initializer() };
        // Can't do type inference with a zero initializer
        if (init && init->type == Initializer::zero && vtype->isIncomplete()) {
            report(
                   Error::parseDomain,
                   "Cannot zero initialize a local variable without a type specifier",
                   start.loc.filename,
                   { start, current() },
                   { current().pos(), current().contents.size() }
            );
            return nullptr;
        }
        if (checkSemicolon && match(TokenType::semicolon, " at end of local variable statement").isInvalid()) { return nullptr; }
        index--;
        Token end = current();
        index++;
        TextRegion loc { start, end };
        return new VarStatement(loc, name, vtype, init);
    }
    CallStatement* Parser::callStm(bool checkSemicolon) {
        Call* call { this->callexpr() };
        if (!call) return nullptr;
        if (checkSemicolon && match(TokenType::semicolon, " at end of call statement").isInvalid()) { return nullptr; }
        index--;
        Token end = current();
        index++;
        TextRegion loc { call->name, end };
        return new CallStatement(loc, call);
    }
    EmptyStatment* Parser::emptyStm() {
        Token semicolon { match(TokenType::semicolon, " for empty statement") };
        TextRegion loc { semicolon };
        return new EmptyStatment(loc);
    }
    Statement* Parser::assignmentStm(bool checkSemicolon) {
        const Token start = current();
        Expression* assignTo = expr();
        switch (current().type) {
            case TokenType::semicolon: {
                const Token end = current();
                pacman();
                if (auto call = dynamic_cast<Call*>(assignTo)) {
                    return new CallStatement({ start, end }, call);
                }
                return new ExpressionStatement({ start, end }, assignTo);
            }
            case TokenType::assign: {
                pacman();
                Expression* assignFrom = expr();
                if (checkSemicolon && match(TokenType::semicolon, " in assignment statement", "Insert a semicolon at the end of the statement").isInvalid()) { return nullptr; }
                index--;
                Token end = current();
                index++;
                TextRegion loc { start, end };
                return new Assignment(loc, assignTo, assignFrom);
                break;
            }
            case TokenType::backarrow: {
                pacman();
                Expression* assignFrom = expr();
                if (checkSemicolon && match(TokenType::semicolon, " in pointer assignment statement", "Insert a semicolon at the end of the statement").isInvalid()) { return nullptr; }
                index--;
                Token end = current();
                index++;
                TextRegion loc { start, end };
                return new PointerAssignment(loc, assignTo, assignFrom);
                break;
            }
            default: {
                if (!checkSemicolon) {
                    const Token end = current();
                    if (auto call = dynamic_cast<Call*>(assignTo)) {
                        return new CallStatement({ start, end }, call);
                    }
                    return new ExpressionStatement({ start, end }, assignTo);
                }
                report(Error::parseDomain, "Unexpected expression", start.loc.filename, { start, current() }, { start.pos(), current().pos() - start.pos() });
                return nullptr;
            }
        }
    }

    Literal* Parser::literalexpr() {
        Token cpy = current(); pacman();
        const TextRegion loc { cpy, cpy };
        switch (cpy.type) {
            case TokenType::boolTrue:
            case TokenType::boolFalse:
                return new Literal(loc, Literal::LType::boolean, cpy);
            case TokenType::asciiString:
                while (!eof() && current().type == TokenType::asciiString) {
                    cpy.contents += current().contents;
                    index++;
                }
                return new Literal(loc, Literal::LType::cString, cpy);
            case TokenType::wideString:
                while (!eof() && current().type == TokenType::wideString) {
                    cpy._wstr.reserve(cpy._wstr.size() + current()._wstr.size());
                    for (uint32_t wchar: current()._wstr) {
                        cpy._wstr.push_back(wchar);
                    }
                    index++;
                }
                return new Literal(loc, Literal::LType::wideString, cpy);
            case TokenType::numFloating:
                return new Literal(loc, Literal::LType::floatingPointNumber, cpy);
            case TokenType::numIntHex:
                return new Literal(loc, Literal::LType::hexadecimalInteger, cpy);
            case TokenType::numIntDec:
                return new Literal(loc, Literal::LType::decimalInteger, cpy);
            case TokenType::numByteDec:
                return new Literal(loc, Literal::LType::decimalByte, cpy);
            case TokenType::numWideChar:
                return new Literal(loc, Literal::LType::decimalWideChar, cpy);
            case TokenType::numShortDec:
                return new Literal(loc, Literal::LType::decimalShort, cpy);
            case TokenType::numInt32Dec:
                return new Literal(loc, Literal::LType::decimalInt32, cpy);
            case TokenType::numUIntDec:
                return new Literal(loc, Literal::LType::decimalUInteger, cpy);
            case TokenType::numUByteDec:
                return new Literal(loc, Literal::LType::decimalUByte, cpy);
            case TokenType::wideUCharType:
                return new Literal(loc, Literal::LType::decimalWideUChar, cpy);
            case TokenType::numUShortDec:
                return new Literal(loc, Literal::LType::decimalUShort, cpy);
            case TokenType::numUInt32Dec:
                return new Literal(loc, Literal::LType::decimalUInt32, cpy);
            case TokenType::null:
                cpy.contents = "0";
                return new Literal(loc, Literal::LType::decimalUInteger, cpy);
            default:
                return nullptr;
        }
    }
    UnsafeCast* Parser::unsafecastexpr() {
        const Token start = current();
        pacman();
        if (match(TokenType::less, " in unsafe cast expression").isInvalid()) return nullptr;
        Type* t = type();
        if (match(TokenType::greater, " in unsafe cast expression").isInvalid()) return nullptr;
        if (match(TokenType::leftParenthesis, " in unsafe cast expression").isInvalid()) return nullptr;
        Expression* e = expr();
        const Token end = match(TokenType::rightParenthesis, " in unsafe cast expression");
        if (end.isInvalid()) return nullptr;
        return new UnsafeCast({ start, end }, t, e);
    }

    Expression* Parser::expr(bool acceptsRightBracket) {
        Expression* lhs = primaryexpr();
        if (current().type == TokenType::rightBracket && !acceptsRightBracket) {
            return lhs;
        }
        if (current().isOperator()) {
            OperatorComponentExpression* op = this->op();
            return binaryexpr(lhs, op, acceptsRightBracket);
        }
        else return lhs;
    }
    BinaryExpression* Parser::binaryexpr(Expression* lhs, OperatorComponentExpression* op, bool acceptsRightBracket) {
        Expression* rhs = primaryexpr();
        const Token start { current() };
        if (start.type == TokenType::rightBracket && !acceptsRightBracket && !lhs && op->tkntype() != TokenType::leftBracket) {
            return new BinaryExpression({ start }, lhs, op, rhs);
        }
        BAD_NEVER_AGAIN:
        if (start.isOperator()) {
            if (current().type == TokenType::rightBracket && op->tkntype()== TokenType::leftBracket) {
                pacman();
                goto BAD_NEVER_AGAIN;
            }
            OperatorComponentExpression* nextOp = this->op();
            if (!nextOp) {
                const Token end { current() };
                return new BinaryExpression({ start, end }, lhs, op, rhs);
            }
            if (nextOp->precedence(OperatorComponentExpression::infix) > op->precedence(OperatorComponentExpression::infix)) {
                Expression* next = binaryexpr(rhs, nextOp, acceptsRightBracket);
                const Token end { current() };
                return new BinaryExpression({ start, end }, lhs, op, next);
            } else {
                const Token end { current() };
                return binaryexpr(new BinaryExpression({ start, end }, lhs, op, rhs), nextOp, acceptsRightBracket);
            }
        } else {
            const Token end { current() };
            return new BinaryExpression({ start, end }, lhs, op, rhs);
        }
    }
    Expression* Parser::primaryexpr() {
        if (current().type == TokenType::sizeof_) {
            const Token start = current();
            pacman();
            if (match(TokenType::leftParenthesis, " in sizeof expression").isInvalid()) return nullptr;
            Type* t = type();
            if (match(TokenType::rightParenthesis, " in sizeof expression").isInvalid()) return nullptr;
            return new SizeOfType({ start, current() }, t);
        } else if (current().type == TokenType::unsafe_cast) {
            return unsafecastexpr();
        } else if (current().type == TokenType::leftBracket) {
            const Token start = current();
            pacman();
            std::vector<Expression*> vals;
            while (!eof() && current().type != TokenType::rightBracket) {
                if (current().type == TokenType::rightBracket) {
                    break;
                }
                Expression* e = expr(false);
                if (!e) return nullptr;
                vals.push_back(e);
                if (current().type != TokenType::rightBracket) {
                    if (match(TokenType::comma, " in array literal").isInvalid()) return nullptr;
                }
            }
            if (match(TokenType::rightBracket, " at end of array literal").isInvalid()) return nullptr;
            return new ArrayLiteralExpression({ start, current() }, vals);
        } else if (current().type == TokenType::leftParenthesis) {
            pacman();
            auto parsedExpr = expr();
            if (match(TokenType::rightParenthesis, " to match opening '('").isInvalid()) return nullptr;
            return parsedExpr;
        } else if (current().isLiteral()) {
            return literalexpr();
        } else if (current().isId()) {
            if (auto skip = isAhead(TokenType::leftParenthesis, { TokenType::identifier, TokenType::scopeResolve })) {
                auto n = current();
                auto iter = Type::typealiases.find(n.contents);
                if (iter != Type::typealiases.end()) {
                    auto t = iter->second;
                    if (!t->isStruct()) {
                        return nullptr;
                    }
                    n.contents = t->structValue()->name().contents;
                }
                if (std::find_if(Type::structs.begin(), Type::structs.end(), [n](StructDeclaration* s){
                    return s->name().contents == n.contents;
                }) != Type::structs.end()) {
                    return constructexpr(n);
                } else {
                    return callexpr();
                }
            } else {
                return symbolexpr();
            }
        }
        return nullptr;
    }

    OperatorComponentExpression* Parser::op() {
        if (current().isOperator()) {
            const Token cpy = current();
            pacman();
            return new OperatorComponentExpression(cpy);
        }
        else return nullptr;
    }
    SymbolExpression* Parser::symbolexpr() {
        Token start { match(TokenType::identifier, " in symbol") };
        while (current().type == TokenType::scopeResolve) {
            pacman();
            const Token next { match(TokenType::identifier, " in symbol") };
            if (next.isInvalid()) return nullptr;
            start.contents.push_back(NAMESPACE_DELIMITER);
            start.contents += next.contents;
        }
        return new SymbolExpression({ start }, start);
    }
    Call* Parser::callexpr() {
        if (current().type != TokenType::identifier)
            return nullptr;
        Token name { match(TokenType::identifier, " in call") };
        if (name.isInvalid()) return nullptr;
        while (current().type == TokenType::scopeResolve) {
            pacman();
            const Token next { match(TokenType::identifier, " in symbol") };
            if (next.isInvalid()) return nullptr;
            name.contents.push_back(NAMESPACE_DELIMITER);
            name.contents += next.contents;
        }
        pacman(); // advance past left parenthesis
        std::vector<Expression*> arguments;
        while (!eof() && current().type != TokenType::rightParenthesis) {
            arguments.push_back(expr());
            if (current().type != TokenType::rightParenthesis) {
                const Token comma { match(TokenType::comma, " in call arguments") };
                if (comma.isInvalid()) return nullptr;
            }
        }
        Token end { match(TokenType::rightParenthesis, " in call") };
        if (end.isInvalid()) return nullptr;
        TextRegion loc { name, end };
        return new Call(loc, name, arguments);
    }
    ConstructExpression* Parser::constructexpr(const Token& n) {
        Token name { match(TokenType::identifier, " in struct construction") };
        if (name.isInvalid()) return nullptr;
        
        pacman(); // advance past left parenthesis
        std::vector<Expression*> arguments;
        while (!eof() && current().type != TokenType::rightParenthesis) {
            arguments.push_back(expr());
            if (current().type != TokenType::rightParenthesis) {
                const Token comma { match(TokenType::comma, " in constructor arguments") };
                if (comma.isInvalid()) return nullptr;
            }
        }
        const Token end { match(TokenType::rightParenthesis, " in struct construction") };
        if (end.isInvalid()) return nullptr;
        TextRegion loc { name, end };
        return new ConstructExpression(loc, n, arguments, ConstructExpression::Mode::stack);
    }
    ReturnStatement* Parser::returnStm(bool checkSemicolon) {
        Token start { match(TokenType::return_, " in return statement") };
        if (current().type != TokenType::semicolon) {
            auto value { expr() };
            if (checkSemicolon && match(TokenType::semicolon, " at end of return statement").isInvalid()) { return nullptr; }
            index--;
            Token end = current();
            index++;
            TextRegion loc { start, end };
            return new ReturnStatement(loc, value);
        }
        Token end { current() };
        pacman();
        TextRegion loc { start, end };
        return new ReturnStatement(loc, nullptr);
    }
    IfStatement* Parser::ifStm() {
        Token start { match(TokenType::if_, " in if statement") };
        if (start.isInvalid() || match(TokenType::leftParenthesis, " in condition").isInvalid()) return nullptr;
        Expression* condition = expr();
        if (match(TokenType::rightParenthesis, " in condition").isInvalid()) return nullptr;
        Block* body = block();
        index--;
        Token end { current() };
        index++;
        return new IfStatement({ start, end }, condition, body);
    }
    WhileStatement* Parser::whileStm() {
        Token start { match(TokenType::while_, " in while statement") };
        if (start.isInvalid() || match(TokenType::leftParenthesis, " in condition").isInvalid()) return nullptr;
        Expression* condition = expr();
        if (match(TokenType::rightParenthesis, " in condition").isInvalid()) return nullptr;
        Block* body = block();
        index--;
        Token end { current() };
        index++;
        return new WhileStatement({ start, end }, condition, body);
    }
    ForStatement* Parser::forStm() {
        Token start { match(TokenType::for_, " in for statement") };
        if (start.isInvalid() || match(TokenType::leftParenthesis, " in for statement").isInvalid()) return nullptr;
        Statement* init = statement();
        Expression* check = expr();
        if (match(TokenType::semicolon, " in for statement").isInvalid()) return nullptr;
        Statement* modify = statement(false);
        if (match(TokenType::rightParenthesis, " in for statement").isInvalid()) return nullptr;
        Block* body = block();
        index--;
        Token end { current() };
        index++;
        return new ForStatement({ start, end }, init, check, modify, body);
    }
    Block* Parser::block() {
        Token start { match(TokenType::leftBrace, " in block") };
        if (start.isInvalid()) return nullptr;
        std::vector<Node*> body {};
        while (!eof() && current().type != TokenType::rightBrace) {
            if (auto stm = statement()) {
                body.push_back(stm);
            } else {
                report(
                       Error::parseDomain,
                       "Something wrong with block body - cannot parse a statement",
                       start.loc.filename,
                       { start, current() },
                       { current().pos(), current().contents.size() }
                );
                synchronize();
            }
        }
        const Token end = match(TokenType::rightBrace, " at end of block");
        if (end.isInvalid()) return nullptr;
        return new Block({ start, end }, body);
    }
    Statement* Parser::statement(bool checkSemicolon) {
        switch (current().type) {
            case TokenType::var:
                return var(checkSemicolon);
            case TokenType::let:
                return let(checkSemicolon);
            case TokenType::return_:
                return returnStm(checkSemicolon);
            case TokenType::if_:
                return ifStm();
            case TokenType::while_:
                return whileStm();
            case TokenType::for_:
                return forStm();
            case TokenType::leftBrace:
                return block();
            case TokenType::semicolon:
                return emptyStm();
            default:
                return assignmentStm(checkSemicolon);
        }
        return nullptr;
    }

    const std::vector<Use>& Parser::use() const {
        return _use;
    }

    File* Parser::parse() {
        if (tokens.empty()) return nullptr;
        Token lastTkn { tokens.back() };
        TextRegion fileLoc { 0, lastTkn.end(), 1, lastTkn.line() };
        File *file { new File(fileLoc, _path, {}) };
        while (!eof()) {
            switch (current().type) {
                case TokenType::static_:
                    _attrs.insert(Function::Attributes::static_);
                    pacman();
                    break;
                case TokenType::inline_:
                    _attrs.insert(Function::Attributes::inline_);
                    pacman();
                    break;
                case TokenType::macro:
                    pacman();
                    break;
                case TokenType::using_: {
                    const Token start {current()};
                    pacman();
                    const Token id = match(TokenType::identifier, " in using directive");
                    if (id.isInvalid()) {
                        report(
                               Error::parseDomain,
                               "Expected identifier in using directive",
                               start.loc.filename,
                               { start, current() },
                               { current().pos(), current().contents.size() }
                        );
                        synchronize();
                        break;
                    }
                    if (match(TokenType::semicolon, " at end of using directive").isInvalid()) {
                        synchronize();
                    } else {
                        if (id.contents == "stdlib") {
                            _use.push_back(Use::stl);
                        } else if (id.contents == "libc") {
                            _use.push_back(Use::libc);
                        }
                    }
                    break;
                }
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
                case TokenType::struct_: {
                    if (auto strct = structdef()) {
                        file->insert(strct);
                    } else {
                        synchronize();
                    }
                    break;
                }
                case TokenType::namespace_: {
                    if (auto ns = nmspace()) {
                        file->insert(ns);
                    } else {
                        synchronize();
                    }
                    break;
                }
                case TokenType::typealias: {
                    if (auto ta = typealias()) {
                        file->insert(ta);
                        if (std::find_if(Type::typealiases.begin(), Type::typealiases.end(), [ta](auto pair){
                            return pair.first == ta->alias().contents;
                        }) != Type::typealiases.end()) {
                            report(Error::parseDomain, "Realiasing of synonym " + ta->alias().contents + " to different type", ta->_loc.path, ta->_loc, { ta->alias().pos(), ta->alias().contents.size() });
                            break;
                        }
                        Type::typealiases.insert({ ta->alias().contents, ta->aliased() });
                    } else {
                        synchronize();
                    }
                    break;
                }
                default: {
                    if (_synchr_count > 3) {
                        return nullptr;
                    }
                    synchronize();
                    break;
                }
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
    bool Parser::hasWarnings() const {
        return !_warnings.empty();
    }
    const std::vector<Error>& Parser::warnings() const {
        return _warnings;
    }
    void Parser::setPath(const std::string& path) {
        _path = path;
    }
    
    int distance(const std::string& s, const std::string& t) {
        int i {}; int j {};
        const int slength = (int)s.size();
        const int tlength = (int)t.size();
        while (i < slength && j < tlength) {
            if (s[i] == t[j]) {
                j++;
            }
            i++;
        }
        return abs(i - j) + abs(slength - tlength);
    }

    std::vector<std::pair<std::string, size_t>> Parser::similarTo(const std::string& str, bool wantsDeclarators) {
        std::vector<std::pair<std::string, size_t>> didYouMean;
        for (auto pair: keywordMap) {
            const int difference = distance(str, pair.first);
            if (difference < 3) {
                if (!wantsDeclarators || tokenTypeIsDeclarator(pair.second)) {
                    didYouMean.push_back({pair.first, difference});
                }
            }
        }
        std::sort(didYouMean.begin(), didYouMean.end(), [](std::pair<std::string, size_t> lhs, std::pair<std::string, size_t> rhs) -> bool {
            return lhs.second < rhs.second;
        });
        return didYouMean;
    }
}
