//
//  SPA.cpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 7/21/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "SPA.hpp"
#include "Operator.hpp"
#include <string>
#include <stdexcept>

namespace Floral {
    void StaticAnalyzer::report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {domain, text, loc, errloc};
        err.fix = fix;
        err.path = _path;
        _errors.push_back(err);
    }
    bool StaticAnalyzer::hasErrors() const {
        return !_errors.empty();
    }
    const std::vector<Error>& StaticAnalyzer::errors() const {
        return _errors;
    }
    void StaticAnalyzer::pushScope() {
        scopes.push_back({});
    }
    void StaticAnalyzer::popScope() {
        scopes.pop_back();
    }
    Scope& StaticAnalyzer::scope() {
        return scopes.back();
    }
    Type* StaticAnalyzer::localLookupType(const std::string& id) {
        Type* t = nullptr;
        for (auto iter = scopes.rbegin(); iter != scopes.rend(); iter++) {
            t = (*iter).typeOf(id);
        }
        return t;
    }

    int StaticAnalyzer::analyze(const File *file) {
        _path = file->path();
        scopes.push_back({});
        for (auto node: file->nodes()) {
            if (auto decl = dynamic_cast<Declaration*>(node))
                if (analyze(decl) != 0) return 1;
        }
        if (analyze(file->main()) != 0) return 1;
        scopes.pop_back();
        return 0;
    }
    int StaticAnalyzer::analyze(Statement* stm) {
        if (auto rtn = dynamic_cast<ReturnStatement*>(stm)) {
            rtn->info.isStaticEval = isStaticEval(rtn->value());
            if (rtn->value()) {
                if (analyze(rtn->value()) != 0) return 1;
                if (!(*scope().func->returnType() == *rtn->value()->type)) {
                    report(
                           Error::typeDomain,
                           "Cannot return a value of type " + rtn->value()->type->des() + " in function returning " + scope().func->returnType()->des(),
                           rtn->_loc,
                           { rtn->_loc.pos, 0 }
                    );
                    return 1;
                }
            }
        } else if (auto callStm = dynamic_cast<CallStatement*>(stm)) {
            if (analyze(callStm->call) != 0) return 1;
        }
        return 0;
    }

    bool StaticAnalyzer::functionExists(const std::string& name, const Function::Parameters& params) {
        const std::string key {strFromFunctionSignature({name, params})};
        return functionSymbolTable.find(key) != functionSymbolTable.end() || functionForwardDeclSymbolTable.find(key) != functionForwardDeclSymbolTable.end();
    }
    Type* StaticAnalyzer::lookupRType(const std::string& name, const Function::Parameters& params) {
        const std::string key {strFromFunctionSignature({name, params})};
        if (functionSymbolTable.find(key) != functionSymbolTable.end())
            return const_cast<Type*>(functionSymbolTable[key]->returnType());
        else if (functionForwardDeclSymbolTable.find(key) != functionForwardDeclSymbolTable.end())
            return const_cast<Type*>(functionForwardDeclSymbolTable[key]->returnType());
        else return nullptr;
    }

    int StaticAnalyzer::analyze(Declaration *decl) {
        if (auto func = dynamic_cast<Function*>(decl)) {
            scopes.push_back({});
            if (functionExists(func->name().contents, func->parameters())) {
                report(
                       Error::resolutionDomain,
                       "Invalid redeclaration of '" + func->name().contents + "'",
                       func->_loc,
                       { func->name().pos(), func->name().contents.size() }
                );
                return 1;
            } else {
                functionSymbolTable[strFromFunctionSignature({func->name().contents, func->parameters()})] = func;
            }
            for (auto param: func->parameters()) {
                scopes.back().insert(param.name.contents, param.type);
            }
            if (func->returnType()->isIncomplete()) {
                func->setRType(new Type(new Token({0,0}, TokenType::voidType, "Void"), true));
            }
            for (auto node: func->body()) {
                if (auto stm = dynamic_cast<Statement*>(node)) {
                    if (analyze(stm) != 0) return 1;
                } else if (auto decl = dynamic_cast<Declaration*>(node)) {
                    if (analyze(decl) != 0) return 1;
                    if (auto let = dynamic_cast<LetDeclaration*>(decl)) {
                        func->staticAllocationSize += let->type()->size();
                    } else if (auto var = dynamic_cast<VarDeclaration*>(decl)) {
                        func->staticAllocationSize += var->type()->size();
                    }
                }
            }
            scopes.pop_back();
        } else if (auto ffunc = dynamic_cast<FunctionForwardDeclaration*>(decl)) {
            if (functionExists(ffunc->name().contents, ffunc->parameters())) {
                report(
                       Error::resolutionDomain,
                       "Invalid redeclaration of '" + ffunc->name().contents + "'",
                       ffunc->_loc,
                       { ffunc->name().pos(), ffunc->name().contents.size() }
                );
                return 1;
            } else {
                functionForwardDeclSymbolTable[strFromFunctionSignature({ffunc->name().contents, ffunc->parameters()})] = ffunc;
            }
            if (ffunc->returnType()->isIncomplete()) {
                ffunc->setRType(new Type(new Token({0,0}, TokenType::voidType, "Void"), true));
            }
        } else if (auto gbl = dynamic_cast<GlobalDeclaration*>(decl)) {
            if (globalSymbolTable[gbl->name.contents]) {
                report(
                       Error::resolutionDomain,
                       "Invalid redeclaration of '" + gbl->name.contents + "'",
                       gbl->_loc,
                       { gbl->name.pos(), gbl->name.contents.size() }
                );
                return 1;
            } else {
                globalSymbolTable[gbl->name.contents] = gbl;
            }
            auto initializer {gbl->initializer()};
            if (initializer->type == Initializer::zero) {
                gbl->info.isStaticEval = true;
            } else if (auto direct = dynamic_cast<const DirectInitializer*>(initializer)) {
                gbl->info.isStaticEval = isStaticEval(direct->expr());
            } else if (auto copy = dynamic_cast<const CopyInitializer*>(initializer)) {
                gbl->info.isStaticEval = isStaticEval(copy->expr());
            }
        } else if (auto fgbl = dynamic_cast<GlobalForwardDeclaration*>(decl)) {
            if (globalSymbolTable[fgbl->name().contents]) {
                report(
                       Error::resolutionDomain,
                       "Invalid redeclaration of '" + fgbl->name().contents + "'",
                       fgbl->_loc,
                       { fgbl->name().pos(), fgbl->name().contents.size() }
                );
                return 1;
            } else {
                globalForwardDeclSymbolTable[fgbl->name().contents] = fgbl;
            }
        } else if (auto let = dynamic_cast<LetDeclaration*>(decl)) {
            const Initializer* init = let->initializer();
            switch (init->type) {
                case Initializer::zero: {
                    scopes.back().insert(let->name().contents, const_cast<Type*>(let->type()));
                    break;
                }
                case Initializer::direct: {
                    auto initexpr {dynamic_cast<const DirectInitializer*>(init)->expr()};
                    if (analyze(initexpr) != 0) {
                        return 1;
                    }
                    initexpr->type->_setConst(true);
                    const Type* declaredType {let->type()};
                    if (!declaredType->isIncomplete()) {
                        if (!(*declaredType == *initexpr->type)) {
                            report(
                                   Error::typeDomain,
                                   "Expression resolves to type different from type declared in let declaration",
                                   let->_loc,
                                   { let->_loc.pos, 0 }
                            );
                        }
                    } else {
                        let->setType(initexpr->type);
                    }
                    scopes.back().insert(let->name().contents, const_cast<Type*>(let->type()));
                    break;
                    break;
                }
                case Initializer::copy: {
                    auto initexpr {dynamic_cast<const CopyInitializer*>(init)->expr()};
                    if (analyze(initexpr) != 0) {
                        return 1;
                    }
                    initexpr->type->_setConst(true);
                    const Type* declaredType {let->type()};
                    if (!declaredType->isIncomplete()) {
                        if (!(*declaredType == *initexpr->type)) {
                            report(
                                   Error::typeDomain,
                                   "Expression resolves to type different from type declared in let declaration",
                                   let->_loc,
                                   { let->_loc.pos, 0 }
                            );
                        }
                    } else {
                        let->setType(initexpr->type);
                    }
                    scopes.back().insert(let->name().contents, const_cast<Type*>(let->type()));
                    break;
                }
            }
        } else if (auto var = dynamic_cast<VarDeclaration*>(decl)) {
            const Initializer* init = var->initializer();
            switch (init->type) {
                case Initializer::zero: {
                    scopes.back().insert(var->name().contents, const_cast<Type*>(var->type()));
                    break;
                }
                case Initializer::direct: {
                    auto initexpr {dynamic_cast<const DirectInitializer*>(init)->expr()};
                    if (analyze(initexpr) != 0) {
                        return 1;
                    }
                    if (initexpr->type->canBeImplicitlyUnconst()) {
                        initexpr->type->print();
                        std::cout << " was implicitly converted to ";
                        initexpr->type->_setConst(false);
                        initexpr->type->print();
                        std::cout << " for asignment to a variable \n";
                    } else if (initexpr->type->isConst()) {
                        report(
                               Error::typeDomain,
                               "Cannot assign a const type to a variable",
                               var->_loc,
                               { var->_loc.pos, 0 }
                        );
                        return 1;
                    }
                    const Type* declaredType {var->type()};
                    if (!declaredType->isIncomplete()) {
                        if (!(*declaredType == *initexpr->type)) {
                            report(
                                   Error::typeDomain,
                                   "Expression resolves to type different from type declared in var declaration",
                                   var->_loc,
                                   { var->_loc.pos, 0 }
                            );
                        }
                    } else {
                        var->setType(initexpr->type);
                    }
                    scopes.back().insert(var->name().contents, const_cast<Type*>(var->type()));
                    break;
                }
                case Initializer::copy: {
                    auto initexpr {dynamic_cast<const CopyInitializer*>(init)->expr()};
                    if (analyze(initexpr) != 0) {
                        return 1;
                    }
                    initexpr->type->_setConst(false); // copying allows for "unconsting"
                    const Type* declaredType {var->type()};
                    if (!declaredType->isIncomplete()) {
                        if (!(*declaredType == *initexpr->type)) {
                            report(
                                   Error::typeDomain,
                                   "Expression resolves to type different from type declared in let declaration",
                                   var->_loc,
                                   { var->_loc.pos, 0 }
                            );
                        }
                    } else {
                        var->setType(initexpr->type);
                    }
                    scopes.back().insert(var->name().contents, const_cast<Type*>(var->type()));
                    break;
                }
            }
        }
        return 0;
    }

    int StaticAnalyzer::analyze(Expression* expr) {
        expr->type = type(expr); // get the type
        std::cout << "SPA: Type of expression [";
        expr->pretty();
        std::cout << "]: ";
        if (expr->type) {
            if (auto call = dynamic_cast<Call*>(expr)) {
                if (!call->_spa_params.empty()) {
                    std::cout << '(';
                    for (size_t i = 0; i < call->_spa_params.size(); i++) {
                        call->_spa_params[i].type->print();
                        if (i + 1 < call->_spa_params.size()) std::cout << ", ";
                    }
                    std::cout << ") > ";
                }
            }
            expr->type->print();
            std::cout << '\n';
        } else {
            std::cout << "Invalid\n";
            return 1;
        }
        return 0;
    }

    std::string StaticAnalyzer::strFromFunctionSignature(FunctionSignature funsig) {
        std::string out {funsig.first};
        for (auto param: funsig.second) {
            out += "_" + param.type->shortID();
        }
        return out;
    }

    Type* StaticAnalyzer::type(Expression *expr) {
        if (BinaryExpression* binaryExpression = dynamic_cast<BinaryExpression*>(expr)) {
            Type* leftType = nullptr;
            Type* rightType = nullptr;
            
            if (binaryExpression->left()) leftType = type(binaryExpression->left());
            if (binaryExpression->right()) rightType = type(binaryExpression->right());
            
            const TokenType tkntype { binaryExpression->op()->tkntype() };
            Operator op { tkntype };
            if (auto rtype = op.overloadExists(leftType, rightType)) return rtype;
            else {
                report(
                       Error::typeDomain,
                       "No such overload exists for the operation " + binaryExpression->op()->tkn().contents,
                       expr->_loc,
                       { expr->_loc.pos, 0 }
                );
                return nullptr;
            }
        } else if (Literal* literal = dynamic_cast<Literal*>(expr)) {
            switch (literal->type()) {
                case Literal::LType::boolean: {
                    return new Type(new Token({0,0}, TokenType::boolType, "Bool"), true);
                }
                case Literal::LType::decimalInteger:
                case Literal::LType::hexadecimalInteger: {
                    return new Type(new Token({0,0}, TokenType::int64Type, "Int"), true);
                }
                case Literal::LType::simpleString: {
                    return new Type(new Type(new Token({0,0}, TokenType::charType, "Char")), true, true);
                }
                default:
                    break;
            }
        } else if (SymbolExpression* symbol = dynamic_cast<SymbolExpression*>(expr)) {
            // MARK: BADDDDDD ONLY LOOKS IN THIS SCOPE ADD FUNCTION THAT LOOKS IN ALLLLLLL!!!!!!
            Type* type = localLookupType(symbol->value().contents);
            if (!type) {
                report(
                       Error::resolutionDomain,
                       "The symbol '" + symbol->value().contents + "' could not be found",
                       symbol->_loc,
                       { symbol->_loc.pos, 0 },
                       "Try declaring or defining the symbol with a global, let or var to silence this error"
                    );
                return nullptr;
            }
            return type;
        } else if (Call* call = dynamic_cast<Call*>(expr)) {
            Function::Parameters argtypes;
            argtypes.reserve(call->args.size());
            for (auto arg: call->args) {
                Type* argtype = type(arg);
                argtypes.push_back({
                    *Token::invalid,
                    argtype
                });
            }
            call->_spa_params = argtypes;
            Type* r = lookupRType(call->name.contents, argtypes);
            if (!r) {
                report(
                       Error::resolutionDomain,
                       "No function '" + call->name.contents + "' exists",
                       call->_loc,
                       { call->_loc.pos, 0 },
                       "Try forward-declaring or defining the function silence this error"

                );
            }
            return r;
        }
        return nullptr;
    }

    bool StaticAnalyzer::isStaticEval(Expression* expr) {
        if (!expr) return true;
        if (BinaryExpression* binaryExpression = dynamic_cast<BinaryExpression*>(expr)) {
            binaryExpression->info.isStaticEval = isStaticEval(binaryExpression->left()) && isStaticEval(binaryExpression->right());
        } else if (auto literal = dynamic_cast<Literal*>(expr)) {
            literal->info.isStaticEval = true;
            return true;
        } else if (auto call = dynamic_cast<Call*>(expr)) {
            //if (!functionSymbolTable[call->id()]) report error that no overload exists or smth
            call->info.isStaticEval = false;
            return false;
        } else if (auto op = dynamic_cast<OperatorComponentExpression*>(expr)) {
            op->info.isStaticEval = true;
            return true;
        } else if (auto symbol = dynamic_cast<SymbolExpression*>(expr)) {
            const std::string name { symbol->value().contents };
            if (auto gbl = globalSymbolTable[name]) return gbl->info.isStaticEval;
            if (auto fgbl = globalForwardDeclSymbolTable[name]) return true;
            //report(Error::resolutionDomain, "No declaration found for symbol '" + symbol->value().contents + "'", symbol->_loc);
            // MARK: fix local resolution, for now return true
            return true;
        }
        return false;
    }
    GlobalDeclaration* StaticAnalyzer::lookupGlobal(std::string symbol) {
        return globalSymbolTable[symbol];
    }
    GlobalForwardDeclaration* StaticAnalyzer::lookupGlobalDecl(std::string symbol) {
        return globalForwardDeclSymbolTable[symbol];
    }
}
