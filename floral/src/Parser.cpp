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
    void Parser::report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {domain, text, loc, errloc};
        err.path = _path;
        err.fix = fix;
        _errors.push_back(err);
    }
    void Parser::warn(const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {Error::warning, text, loc, errloc};
        err.fix = fix;
        err.isWarning = true;
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
    Token Parser::match(TokenType type, const std::string& withinCtx, const std::string& fix) {
        if (current().type == type) {
            Token t { current() };
            advance();
            return t;
        }
        report(
               Error::parseDomain,
               "Expected " + tokenTypeDescription(type) + " but received " + tokenTypeDescription(current().type) + withinCtx,
               TextRegion(current()),
               { current().pos(), current().contents.size() },
               fix
        );
        return { {current().pos(), current().contents.size()}, TokenType::invalid, "" };
    }
    void Parser::synchronize() {
        _synchr_count++;
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
               current().contents + " is not a declarator",
               {current()},
               { current().pos(), 0 },
               fix
        );
        while (!(eof() || current().isDeclarator())) {
            if (current().isOperator()) {
                report(
                       Error::parseDomain,
                       "An operator cannot be a top-level declaration",
                       {current()},
                       { current().pos(), current().contents.size() }
                );
            } else if (current().isLiteral()) {
                report(
                       Error::parseDomain,
                       "An literal cannot be a top-level declaration",
                       {current()},
                       { current().pos(), current().contents.size() }
                );
            } else if (current().isId()) {
                report(
                       Error::parseDomain,
                       "An identifier cannot be a top-level declaration",
                       {current()},
                       { current().pos(), current().contents.size() }
                );
            }
            advance();
        }
    }

    Type* Parser::type() {
        bool isConst{};
        if (current().type == TokenType::const_) isConst = true, advance();
        if (current().isType()) {
            auto t { new Type(new Token(current().loc, current().type, current().contents), isConst) };
            advance();
            
            if (current().type == TokenType::arrow) {
                advance();
                return new Type(t, type(), isConst);
            }
            return t;
        }
        switch (current().type) {
            case TokenType::andOp:
                advance();
                if (current().isType() || current().type == TokenType::leftParenthesis || current().type == TokenType::leftBracket || current().type == TokenType::andOp || current().type == TokenType::const_) {
                    return new Type(type(), true, isConst);
                } else {
                    auto lhs {new Type(type(), true, isConst)};
                    if (match(TokenType::arrow).isInvalid()) return nullptr;
                    return new Type(lhs, type(), isConst);
                }
            case TokenType::leftBracket: {
                advance();
                auto t { type() };
                if (match(TokenType::rightBracket, " in array type").isInvalid()) return nullptr;
                auto arrt {new Type(t, false, isConst)};
                if (current().type == TokenType::arrow) {
                    advance();
                    return new Type(arrt, type(), isConst);
                }
                return arrt;
            }
            case TokenType::leftParenthesis: {
                Type* tuple[MAX_TUPLE_SIZE];
                size_t i {};
                advance();
                while (!eof() && current().type != TokenType::rightParenthesis) {
                    tuple[i] = type();
                    if (current().type == TokenType::comma)
                        advance();
                    ++i;
                    if (i >= MAX_TUPLE_SIZE) {
                        report(
                               Error::parseDomain,
                               "Tuples must be less than " + std::to_string(MAX_TUPLE_SIZE) + " in length",
                               TextRegion(current()),
                               { current().pos(), current().contents.size() }
                        );
                        return nullptr;
                    }
                }
                if (match(TokenType::rightParenthesis, " in tuple type").isInvalid()) return nullptr;
                auto t { i == 1 ? tuple[0] : new Type(tuple, i, isConst) };
                if (current().type == TokenType::arrow) {
                    advance();
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
            advance();
            rtype = type();
        }
        if (current().type == TokenType::semicolon) {
            const Token end { current() };
            advance();
            TextRegion loc { start, end };
            return new FunctionForwardDeclaration(loc, name, parameters, rtype);
        } else if (current().type == TokenType::leftBrace) {
            advance();
        } else {
            report(
                   Error::parseDomain,
                   "Unexpected token at end of function parameters",
                   { start, current() },
                   { start.pos(), start.contents.size() + current().contents.size() }
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
        return func;
    }
    Declaration* Parser::global() {
        const Token start { current() };
        advance();
        Token name { match(TokenType::identifier, " in global constant declaration") };
        Type* gtype { new Type(Token::invalid) };
        if (current().type == TokenType::colon) {
            advance();
            gtype = type();
            if (current().type == TokenType::semicolon) {
                const Token end { current() };
                advance();
                return new GlobalForwardDeclaration({ start, end }, name, gtype);
            }
        }
        Initializer* init { initializer() };
        // Globals must be initialized
        if (!init) {
            report(
                   Error::parseDomain,
                   "Global constant declaration missing initializer",
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
        while (!eof() && current().type != TokenType::rightBrace) {
            switch (current().type) {
                case TokenType::var:
                    dataMembers.push_back(var());
                    break;
                case TokenType::func: {
                    auto decl = function();
                    if (auto func = dynamic_cast<Function*>(decl)) {
                        functionMembers.push_back(func);
                    } else {
                        report(Error::parseDomain, "Function forward declarations are not allowed within a struct body", decl->_loc, { decl->_loc.pos, 4 });
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
        return new StructDeclaration({ start, end }, name, dataMembers, functionMembers);
    }

    LetStatement* Parser::let(bool checkSemicolon) {
        Token start { current() };
        advance();
        Token name { match(TokenType::identifier, " in local constant statement") };
        Type* ltype { new Type(Token::invalid) };
        if (current().type == TokenType::colon) {
            advance();
            ltype = type();
        }
        const Token store { current() };
        Initializer* init { initializer() };
        // Local constants must be initialized
        if (!init) {
            report(
                   Error::parseDomain,
                   "Local constant statement missing initializer",
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
        advance();
        Token name { match(TokenType::identifier, " in local variable statement") };
        Type* vtype { new Type(Token::invalid) };
        if (current().type == TokenType::colon) {
            advance();
            vtype = type();
        }
        if (current().type == TokenType::semicolon) {
            Token end { current() };
            advance();
            TextRegion loc { start, end };
            return new VarStatement(loc, name, vtype, nullptr);
        }
        Initializer* init { initializer() };
        // Can't do type inference with a zero initializer
        if (init && init->type == Initializer::zero && vtype->isIncomplete()) {
            report(
                   Error::parseDomain,
                   "Cannot zero initialize a local variable without a type specifier",
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
                advance();
                return new ExpressionStatement({ start, end }, assignTo);
            }
            case TokenType::assign: {
                advance();
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
                advance();
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
                report(Error::parseDomain, "Unexpected expression", { start, current() }, { start.pos(), current().pos() - start.pos() });
                return nullptr;
            }
        }
    }

    Literal* Parser::literalexpr() {
        Token cpy = current(); advance();
        const TextRegion loc { cpy, cpy };
        switch (cpy.type) {
            case TokenType::boolTrue:
            case TokenType::boolFalse:
                return new Literal(loc, Literal::LType::boolean, cpy);
            case TokenType::simpleString:
                while (!eof() && current().type == TokenType::simpleString) {
                    cpy.contents += current().contents;
                    index++;
                }
                return new Literal(loc, Literal::LType::cString, cpy);
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
        advance();
        if (match(TokenType::less, " in unsafe cast expression").isInvalid()) return nullptr;
        Type* t = type();
        if (match(TokenType::greater, " in unsafe cast expression").isInvalid()) return nullptr;
        if (match(TokenType::leftParenthesis, " in unsafe cast expression").isInvalid()) return nullptr;
        Expression* e = expr();
        const Token end = match(TokenType::rightParenthesis, " in unsafe cast expression");
        if (end.isInvalid()) return nullptr;
        return new UnsafeCast({ start, end }, t, e);
    }

    Expression* Parser::expr() {
        if (current().type == TokenType::sizeof_) {
            const Token start = current();
            advance();
            if (match(TokenType::leftParenthesis, " in sizeof expression").isInvalid()) return nullptr;
            Type* t = type();
            if (match(TokenType::rightParenthesis, " in sizeof expression").isInvalid()) return nullptr;
            return new SizeOfType({ start, current() }, t);
        } else if (current().type == TokenType::unsafe_cast) {
            return unsafecastexpr();
        }
        Expression* lhs = primaryexpr();
        if (current().isOperator()) {
            OperatorComponentExpression* op = this->op();
            return binaryexpr(lhs, op);
        }
        else return lhs;
    }
    BinaryExpression* Parser::binaryexpr(Expression* lhs, OperatorComponentExpression* op) {
        Expression* rhs = primaryexpr();
        const Token start { current() };
        BAD_NEVER_AGAIN:
        if (current().isOperator()) {
            if (current().type == TokenType::rightBracket && op->tkntype()== TokenType::leftBracket) {
                advance();
                goto BAD_NEVER_AGAIN;
            }
            OperatorComponentExpression* nextOp = this->op();
            if (nextOp->precedence(OperatorComponentExpression::infix) > op->precedence(OperatorComponentExpression::infix)) {
                Expression* next = binaryexpr(rhs, nextOp);
                const Token end { current() };
                return new BinaryExpression({ start, end }, lhs, op, next);
            } else {
                const Token end { current() };
                return binaryexpr(new BinaryExpression({ start, end }, lhs, op, rhs), nextOp);
            }
        } else {
            const Token end { current() };
            return new BinaryExpression({ start, end }, lhs, op, rhs);
        }
    }
    Expression* Parser::primaryexpr() {
        if (current().isLiteral()) return literalexpr();
        if (current().isId()) {
            if (peek().type == TokenType::leftParenthesis) return callexpr();
            else return symbolexpr();
        }
        return nullptr;
    }

    OperatorComponentExpression* Parser::op() {
        if (current().isOperator()) {
            const Token cpy = current();
            advance();
            return new OperatorComponentExpression(cpy);
        }
        else return nullptr;
    }
    SymbolExpression* Parser::symbolexpr() {
        Token start { match(TokenType::identifier, " in symbol") };
        return new SymbolExpression({ start }, start);
    }
    Call* Parser::callexpr() {
        if (current().type != TokenType::identifier)
            return nullptr;
        Token name { match(TokenType::identifier, " in call") };
        if (name.isInvalid()) return nullptr;
        advance(); // advance past left parenthesis
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
        advance();
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
                case TokenType::macro:
                    advance();
                    break;
                case TokenType::using_: {
                    const Token start {current()};
                    advance();
                    const Token id = match(TokenType::identifier, " in using directive");
                    if (id.isInvalid()) {
                        report(
                               Error::parseDomain,
                               "Expected identifier in using directive",
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
                            _use.push_back(Use::stdlib);
                        } else if (id.contents == "C") {
                            _use.push_back(Use::C);
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
                default: {
                    if (auto stm = statement()) {
                        file->insert(stm);
                    } else {
                        if (_synchr_count > 3) {
                            return nullptr;
                        }
                        synchronize();
                    }
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
    std::vector<std::pair<std::string, size_t>> Parser::similarTo(const std::string& str) {
        std::vector<std::pair<std::string, size_t>> didYouMean;
        for (auto pair: keywordMap) {
            size_t correct{};
            for (auto c: str) {
                if (c == pair.first[correct]) {
                    correct++;
                }
            }
            if (pair.first.size() - correct <= 2) {
                didYouMean.push_back({pair.first, correct});
            }
        }
        std::sort(didYouMean.begin(), didYouMean.end(), [](std::pair<std::string, size_t> lhs, std::pair<std::string, size_t> rhs) -> bool {
            return lhs.second < rhs.second;
        });
        return didYouMean;
    }
}
