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
#include "Colors.hpp"

namespace Floral {
    bool StaticAnalyzer::hasErrors() const {
        return !_errors.empty();
    }
    const std::vector<Error>& StaticAnalyzer::errors() const {
        return _errors;
    }
    bool StaticAnalyzer::hasWarnings() const {
        return !_warnings.empty();
    }
    const std::vector<Error>& StaticAnalyzer::warnings() const {
        return _warnings;
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
            if (auto type = (*iter).typeOf(id)) t = type;
        }
        return t;
    }
    Function* StaticAnalyzer::currentFunc() {
        Function* f = nullptr;
        for (auto iter = scopes.rbegin(); iter != scopes.rend(); iter++) {
            if (iter->func) {
                f = iter->func;
                break;
            }
        }
        return f;
    }


    int StaticAnalyzer::analyze(const File *file) {
        _path = file->path();
        pushScope();
        for (auto node: file->nodes()) {
            if (auto decl = dynamic_cast<Declaration*>(node))
                if (analyze(decl) != 0) return 1;
        }
        if (analyze(file->main()) != 0) return 1;
        popScope();
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
                           rtn->_loc.path,
                           rtn->_loc,
                           { rtn->_loc.pos, 0 }
                    );
                    return 1;
                }
            } else if (!currentFunc()->returnType()->isVoid()) {
                report(
                    Error::typeDomain,
                    "Expected return value in non-Void function",
                    rtn->_loc.path,
                    rtn->_loc,
                    { rtn->_loc.pos + 6, 0 }
                );
                return 1;
            }
        } else if (auto callStm = dynamic_cast<CallStatement*>(stm)) {
            if (analyze(callStm->call) != 0) return 1;
        } else if (auto exprStm = dynamic_cast<ExpressionStatement*>(stm)) {
            if (analyze(exprStm->expr()) != 0) return 1;
        } else if (auto ptrAssignStm = dynamic_cast<PointerAssignment*>(stm)) {
            if (analyze(ptrAssignStm->ptrExpr()) != 0) return 1;
            if (analyze(ptrAssignStm->newValue()) != 0) return 1;
            if (!ptrAssignStm->ptrExpr()->type->isPointer()) {
                report(
                       Error::typeDomain,
                       "Cannot perform pointer assignment to non-pointer type",
                       ptrAssignStm->_loc.path,
                       ptrAssignStm->_loc,
                       { ptrAssignStm->_loc.pos, 0 },
                       "Maybe insert '&' before the left hand side expression"
                );
                return 1;
            }
            if (GET_PTRTYYPE(ptrAssignStm->ptrExpr()->type)->isConst()) {
                report(
                       Error::typeDomain,
                       "Cannot assign to const value",
                       ptrAssignStm->_loc.path,
                       ptrAssignStm->_loc,
                       { ptrAssignStm->ptrExpr()->_loc.pos, 0 },
                       "Try changing the declaration of the pointer's value to a variable"
                );
                return 1;
            }
            if (!(*GET_PTRTYYPE(ptrAssignStm->ptrExpr()->type) == *ptrAssignStm->newValue()->type)) {
                report(
                       Error::typeDomain,
                       "Cannot assign value of type " + ptrAssignStm->newValue()->type->des() + " to pointer to value of type " + ptrAssignStm->ptrExpr()->type->_ptrType->des(),
                       ptrAssignStm->_loc.path,
                       ptrAssignStm->_loc,
                       { ptrAssignStm->newValue()->_loc.pos, 0 }
                );
                return 1;
            }
        } else if (auto assignStm = dynamic_cast<Assignment*>(stm)) {
            if (analyze(assignStm->lval()) != 0) return 1;
            if (analyze(assignStm->rval()) != 0) return 1;
            if (assignStm->lval()->type->isConst()) {
                report(
                       Error::typeDomain,
                       "Cannot assign to const value",
                       assignStm->_loc.path,
                       assignStm->_loc,
                       { assignStm->lval()->_loc.pos, 0 },
                       "Try changing the declaration of the value to a variable"
                       );
                return 1;
            }
            if (!(*assignStm->lval()->type == *assignStm->rval()->type)) {
                report(
                       Error::typeDomain,
                       "Cannot assign value of type " + assignStm->rval()->type->des() + " to value of type " + assignStm->lval()->type->des(),
                       assignStm->_loc.path,
                       assignStm->_loc,
                       { assignStm->rval()->_loc.pos, 0 }
                );
                return 1;
            }
        } else if (auto ifStm = dynamic_cast<IfStatement*>(stm)) {
            if (analyze(ifStm->condition()) != 0) return 1;
            if (!(ifStm->condition()->type->isBool() || ifStm->condition()->type->isPointer())) {
                report(
                       Error::typeDomain,
                       "If statement condition does not resolve to boolean (Cannot convert value of type " + ifStm->condition()->type->des() + " to Bool)",
                       ifStm->_loc.path,
                       ifStm->_loc,
                       { ifStm->condition()->_loc.pos, 0 }
                );
            }
            analyze(ifStm->body());
        }  else if (auto whileStm = dynamic_cast<WhileStatement*>(stm)) {
            if (analyze(whileStm->condition()) != 0) return 1;
            if (!whileStm->condition()->type->isBool()) {
                report(
                       Error::typeDomain,
                       "While statement condition does not resolve to boolean",
                       whileStm->_loc.path,
                       whileStm->_loc,
                       { whileStm->condition()->_loc.pos, 0 }
                );
            }
            analyze(whileStm->body());
        } else if (auto forStm = dynamic_cast<ForStatement*>(stm)) {
            analyze(forStm->init());
            if (analyze(forStm->check()) != 0) return 1;
            analyze(forStm->modify());
            if (!forStm->check()->type->isBool()) {
                report(
                       Error::typeDomain,
                       "For statement check does not resolve to boolean",
                       forStm->_loc.path,
                       forStm->_loc,
                       { forStm->check()->_loc.pos, 0 }
                );
            }
            analyze(forStm->body());
        } else if (auto block = dynamic_cast<Block*>(stm)) {
            pushScope();
            for (auto node: block->body()) {
                if (auto stm = dynamic_cast<Statement*>(node)) {
                    analyze(stm);
                } else if (auto decl = dynamic_cast<Declaration*>(node)) {
                    analyze(decl);
                }
            }
            popScope();
        } else if (auto let = dynamic_cast<LetStatement*>(stm)) {
                   const Initializer* init = let->initializer();
                   switch (init->type) {
                       case Initializer::zero: {
                           scope().insert(let->name().contents, const_cast<Type*>(let->type()), nullptr);
                           break;
                       }
                       case Initializer::direct: {
                           auto initexpr {dynamic_cast<const DirectInitializer*>(init)->expr()};
                           if (analyze(initexpr) != 0) {
                               return 1;
                           }
                           initexpr->type->_setConst(true);
                           Type* declaredType {const_cast<Type*>(let->type())};
                           if (!declaredType->isIncomplete()) {
                               if (!(*declaredType == *initexpr->type)) {
                                   if (declaredType->isUInt() && initexpr->type->isInt()) {
                                       
                                   } else {
                                       report(
                                              Error::typeDomain,
                                              "Expression resolves to type different from type declared in let statement",
                                              let->_loc.path,
                                              let->_loc,
                                              { let->_loc.pos, 0 }
                                       );
                                   }
                               } else {
                                   declaredType->_setConst(true);
                                   initexpr->type = declaredType;
                                   let->setType(declaredType);
                               }
                           } else {
                               let->setType(initexpr->type);
                           }
                           if (scope().exists(let->name().contents)) {
                               report(
                                      Error::resolutionDomain,
                                       "Invalid redeclaration of '" + let->name().contents + "'",
                                      let->_loc.path,
                                      let->_loc,
                                      { let->_loc.pos + 3, let->name().contents.size() }
                               );
                               return 1;
                           }
                           scope().insert(let->name().contents, const_cast<Type*>(let->type()), initexpr);
                           break;
                       }
                       case Initializer::copy: {
                           auto initexpr {dynamic_cast<const CopyInitializer*>(init)->expr()};
                           if (analyze(initexpr) != 0) {
                               return 1;
                           }
                           initexpr->type->_setConst(true);
                           Type* declaredType {const_cast<Type*>(let->type())};
                           if (!declaredType->isIncomplete()) {
                               if (!(*declaredType == *initexpr->type)) {
                                   if (declaredType->isUInt() && initexpr->type->isInt()) {
                                       
                                   } else {
                                       report(
                                              Error::typeDomain,
                                              "Expression resolves to type different from type declared in let declaration",
                                              let->_loc.path,
                                              let->_loc,
                                              { let->_loc.pos, 0 }
                                       );
                                   }
                               } else {
                                    declaredType->_setConst(true);
                                    initexpr->type = declaredType;
                                    let->setType(declaredType);
                               }
                           } else {
                               let->setType(initexpr->type);
                           }
                           if (scope().exists(let->name().contents)) {
                               report(
                                      Error::resolutionDomain,
                                       "Invalid redeclaration of '" + let->name().contents + "'",
                                      let->_loc.path,
                                      let->_loc,
                                      { let->_loc.pos + 4, let->name().contents.size() }
                               );
                               return 1;
                           }
                           scope().insert(let->name().contents, const_cast<Type*>(let->type()), initexpr);
                           break;
                       }
                   }
               } else if (auto var = dynamic_cast<VarStatement*>(stm)) {
                   const Initializer* init = var->initializer();
                   if (!init) {
                       if (_warnUninit) warn(
                            "Variable is uninitialized",
                            var->_loc.path,
                            var->_loc,
                            { var->_loc.pos + var->_loc.length - 1, 0 },
                            "Initialize the variable to silence this warning"
                       );
                       scope().insert(var->name().contents, const_cast<Type*>(var->type()), nullptr);
                       return 0;
                   }
                   switch (init->type) {
                       case Initializer::zero: {
                           scope().insert(var->name().contents, const_cast<Type*>(var->type()), nullptr);
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
                                      var->_loc.path,
                                      var->_loc,
                                      { var->_loc.pos, 0 }
                               );
                               return 1;
                           }
                           const Type* declaredType {var->type()};
                           if (!declaredType->isIncomplete()) {
                               if (!(*declaredType == *initexpr->type)) {
                                   if (declaredType->isUInt() && initexpr->type->isInt()) {
                                       
                                   } else {
                                       report(
                                              Error::typeDomain,
                                              "Expression resolves to type different from type declared in let declaration",
                                              var->_loc.path,
                                              var->_loc,
                                              { var->_loc.pos, 0 }
                                       );
                                   }
                               }
                           } else {
                               var->setType(initexpr->type);
                           }
                           if (scope().exists(var->name().contents)) {
                               report(
                                      Error::resolutionDomain,
                                       "Invalid redeclaration of '" + var->name().contents + "'",
                                      var->_loc.path,
                                      var->_loc,
                                      { var->_loc.pos + 3, var->name().contents.size() }
                               );
                               return 1;
                           }
                           scopes.back().insert(var->name().contents, const_cast<Type*>(var->type()), initexpr);
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
                                   if (declaredType->isUInt() && initexpr->type->isInt()) {
                                       
                                   } else {
                                       report(
                                              Error::typeDomain,
                                              "Expression resolves to type different from type declared in let declaration",
                                              var->_loc.path,
                                              var->_loc,
                                              { var->_loc.pos, 0 }
                                       );
                                   }
                               }
                           } else {
                               var->setType(initexpr->type);
                           }
                           if (scope().exists(var->name().contents)) {
                               report(
                                      Error::resolutionDomain,
                                       "Invalid redeclaration of '" + var->name().contents + "'",
                                      var->_loc.path,
                                      var->_loc,
                                      { var->_loc.pos + 3, var->name().contents.size() }
                               );
                               return 1;
                           }
                           scope().insert(var->name().contents, const_cast<Type*>(var->type()), initexpr);
                           break;
                       }
                   }
               }
        return 0;
    }

    Declaration* StaticAnalyzer::lookupFunction(const std::string& name, const Function::Parameters& params) {
        for (auto [sigkey, func]: functionSymbolTable) {
            if (func->arity() == params.size() && func->name().contents == name) {
                size_t i = func->arity();
                while (i) {
                    auto declaredType = func->parameters()[i - 1].type;
                    auto providedType = params[i - 1].type;
                    if (!(*declaredType == *providedType) || (
                        (declaredType->isPointer() ? (!GET_PTRTYYPE(declaredType)->isConst() ? GET_PTRTYYPE(providedType)->isConst() : false) : false)
                    )) {
                        break;
                    }
                    --i;
                }
                if (!i) return func;
            }
        }
        for (auto [sigkey, ffunc]: functionForwardDeclSymbolTable) {
            if (ffunc->arity() == params.size() && ffunc->name().contents == name) {
                size_t i = ffunc->arity();
                while (i) {
                    auto declaredType = ffunc->parameters()[i - 1].type;
                    auto providedType = params[i - 1].type;
                    if (!(*declaredType == *providedType) || (
                        (declaredType->isPointer() && !GET_PTRTYYPE(declaredType)->isConst()) ? GET_PTRTYYPE(providedType)->isConst() : false
                    )) {
                        break;
                    }
                    --i;
                }
                if (!i) return ffunc;
                
            }
        }
        return nullptr;
    }
    Type* StaticAnalyzer::lookupRType(const std::string& name, const Function::Parameters& params) {
        if (auto decl = lookupFunction(name, params)) {
            if (auto func = dynamic_cast<Function*>(decl)) {
                return const_cast<Type*>(func->returnType());
            } else if (auto ffunc = dynamic_cast<FunctionForwardDeclaration*>(decl)) {
                return const_cast<Type*>(ffunc->returnType());
            }
        }
        return nullptr;
    }

    int StaticAnalyzer::analyze(Declaration *decl) {
        if (auto func = dynamic_cast<Function*>(decl)) {
            pushScope();
            scope().func = func;
            if (lookupFunction(func->name().contents, func->parameters())) {
                report(
                       Error::resolutionDomain,
                       "Invalid redeclaration of '" + func->name().contents + "'",
                       func->_loc.path,
                       func->_loc,
                       { func->name().pos(), func->name().contents.size() }
                );
                return 1;
            } else {
                const std::string key = strFromFunctionSignature({func->name().contents, func->parameters()});
                functionSymbolTable[key] = func;
            }
            for (auto param: func->parameters()) {
                scope().insert(param.name.contents, param.type, nullptr);
            }
            if (func->returnType()->isIncomplete()) {
                func->setRType(new Type(new Token(TokenLoc::zero, TokenType::voidType, "Void"), true));
            }
            if (func->returnType()->isVoid() && (!func->body().empty() ? !dynamic_cast<ReturnStatement*>(func->body().back()) : true)) {
                func->insert(new ReturnStatement({ func->_loc.pos + func->_loc.length - 1, 0, func->_loc.endLine, func->_loc.endLine }, nullptr));
            }
            for (auto node: func->body()) {
                if (auto stm = dynamic_cast<Statement*>(node)) {
                    if (analyze(stm) != 0) return 1;
                    if (auto let = dynamic_cast<LetStatement*>(stm)) {
                        #define cond (let->type()->isArray() && let->initializer()->type == Initializer::InitializerType::zero)
                        if (!cond) func->staticAllocationSize += let->type()->alignment();
                        #undef cond
                    } else if (auto var = dynamic_cast<VarStatement*>(stm)) {
                        #define cond (var->type()->isArray() && var->initializer()->type == Initializer::InitializerType::zero)
                        if (!cond) func->staticAllocationSize += var->type()->alignment();
                        #undef cond
                    } else if (auto block = dynamic_cast<Block*>(stm)) {
                        func->staticAllocationSize += block->size();
                    } else if (auto forStm = dynamic_cast<ForStatement*>(stm)) {
                        func->staticAllocationSize += forStm->size();
                    }
                } else if (auto decl = dynamic_cast<Declaration*>(node)) {
                    if (analyze(decl) != 0) return 1;
                }
            }
            popScope();
        } else if (auto ffunc = dynamic_cast<FunctionForwardDeclaration*>(decl)) {
            if (lookupFunction(ffunc->name().contents, ffunc->parameters())) {
                report(
                       Error::resolutionDomain,
                       "Invalid redeclaration of '" + ffunc->name().contents + "'",
                       ffunc->_loc.path,
                       ffunc->_loc,
                       { ffunc->name().pos(), ffunc->name().contents.size() }
                );
                return 1;
            } else {
                functionForwardDeclSymbolTable[strFromFunctionSignature({ffunc->name().contents, ffunc->parameters()})] = ffunc;
            }
            if (ffunc->returnType()->isIncomplete()) {
                ffunc->setRType(new Type(new Token(TokenLoc::zero, TokenType::voidType, "Void"), true));
            }
        } else if (auto gbl = dynamic_cast<GlobalDeclaration*>(decl)) {
            if (globalSymbolTable[gbl->name.contents]) {
                report(
                       Error::resolutionDomain,
                       "Invalid redeclaration of '" + gbl->name.contents + "'",
                       gbl->_loc.path,
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
                auto initexpr = direct->expr();
                gbl->info.isStaticEval = isStaticEval(initexpr);
                if (analyze(initexpr) != 0) return 1;
                if (!gbl->info.isStaticEval) {
                    report(Error::compileDomain, "Global constant expression could not be statically evaluated", gbl->_loc.path, gbl->_loc, { initexpr->_loc.pos, initexpr->_loc.length });
                    return 1;
                }
                Type* declaredType { gbl->type };
                if (!declaredType->isIncomplete()) {
                    if (!(*declaredType == *initexpr->type)) {
                        if (declaredType->isUInt() && initexpr->type->isInt()) {
                            
                        } else {
                            report(
                                   Error::typeDomain,
                                   "Expression resolves to type different from type declared in global declaration",
                                   gbl->_loc.path,
                                   gbl->_loc,
                                   { initexpr->_loc.pos, 0 }
                            );
                        }
                    }
                } else {
                    gbl->type = initexpr->type;
                }
                scope().insert(gbl->name.contents, gbl->type, initexpr);
            } else if (auto copy = dynamic_cast<const CopyInitializer*>(initializer)) {
                auto initexpr = copy->expr();
                gbl->info.isStaticEval = isStaticEval(initexpr);
                if (analyze(initexpr) != 0) return 1;
                if (!gbl->info.isStaticEval) {
                    report(Error::compileDomain, "Global constant expression could not be statically evaluated", gbl->_loc.path, gbl->_loc, { initexpr->_loc.pos, initexpr->_loc.length });
                    return 1;
                }
                if (analyze(initexpr) != 0) return 1;
                Type* declaredType { gbl->type };
                if (!declaredType->isIncomplete()) {
                    if (!(*declaredType == *initexpr->type)) {
                        report(
                               Error::typeDomain,
                               "Expression resolves to type different from type declared in global declaration",
                               gbl->_loc.path,
                               gbl->_loc,
                               { initexpr->_loc.pos, 0 }
                        );
                    }
                } else {
                    gbl->type = initexpr->type;
                }
                scope().insert(gbl->name.contents, gbl->type, initexpr);
            }
        } else if (auto fgbl = dynamic_cast<GlobalForwardDeclaration*>(decl)) {
            if (globalSymbolTable[fgbl->name().contents]) {
                report(
                       Error::resolutionDomain,
                       "Invalid redeclaration of '" + fgbl->name().contents + "'",
                       fgbl->_loc.path,
                       fgbl->_loc,
                       { fgbl->name().pos(), fgbl->name().contents.size() }
                );
                return 1;
            } else {
                globalForwardDeclSymbolTable[fgbl->name().contents] = fgbl;
            }
        } else if (auto structdecl = dynamic_cast<StructDeclaration*>(decl)) {
            pushScope();
            scope().insert("this", new Type(new Type(0, structdecl->name().contents), true, true), new SymbolExpression({ structdecl->name(), structdecl->name() }, { structdecl->name().loc, TokenType::identifier, "this" }));
            _warnUninit = false;
            for (auto &stm: structdecl->dataMembers()) {
                if (analyze(stm) != 0) return 1;
            }
            _warnUninit = true;
            for (auto &func: structdecl->functionMembers()) {
                if (analyze(func) != 0) return 1;
            }
            for (auto constr: structdecl->constructors()) {
                pushScope();
                for (auto param: constr->params) {
                    scope().insert(param.name.contents, param.type, nullptr);
                }
                for (auto init: constr->inits) {
                    if (analyze(init.second) != 0) return 1;
                }
                if (analyze(constr->after) != 0) return 1;
                popScope();
            }
            popScope();
        } else if (auto nmspace = dynamic_cast<NamespaceDeclaration*>(decl)) {
            for (auto node: nmspace->nodes()) {
                if (auto decl = dynamic_cast<Declaration*>(node)) {
                    if (analyze(decl) != 0) return 1;
                }
            }
        }
        return 0;
    }

    int StaticAnalyzer::analyze(Expression* expr) {
        expr->info.isStaticEval = isStaticEval(expr);
        expr->type = type(expr); // get the type
        _typeTrace.push_back(expr);
        if (!expr->type) {
            return 1;
        }
        return 0;
    }

    void StaticAnalyzer::dumpTypeTrace() const {
        ColoredStream out(std::cerr);
        out.resetAutomatically = false;
        out << Color::blue << Color::bold << "Static Analyzer: Type Trace" << Color::reset << "\n----------------------------\n";
        for (Expression* expr: _typeTrace) {
            out << "Type of expression" << Color::white << " [";;
            expr->pretty();
            out << ']' << Color::reset << ": ";
            if (expr->type) {
//                if (auto call = dynamic_cast<Call*>(expr)) {
//                    if (!call->_spa_params.empty()) {
//                        std::cout << '(';
//                        for (size_t i = 0; i < call->_spa_params.size(); i++) {
//                            call->_spa_params[i].type->print();
//                            if (i + 1 < call->_spa_params.size()) std::cout << ", ";
//                        }
//                        std::cout << ") > ";
//                    }
//                }
                expr->type->print();
                if (expr->info.isStaticEval) {
                    std::cout << " [constexpr]";
                }
                std::cout << '\n';
            } else {
                std::cout << "Invalid\n";
            }
        }
        fputc('\n', stdout);
    }

    std::string StaticAnalyzer::strFromFunctionSignature(FunctionSignature funsig) {
        std::string out {funsig.first};
        for (auto param: funsig.second) {
            out += "_" + param.type->shortID();
        }
        return out;
    }

    // MARK:: BROKEN FUNCTION WHAT ABIUT AN EORESSION THAT CAN HAVE MULTIPLE TYPES
    Type* StaticAnalyzer::type(Expression *expr) {
        if (BinaryExpression* binaryExpression = dynamic_cast<BinaryExpression*>(expr)) {
            Type* leftType = nullptr;
            Type* rightType = nullptr;
            
            if (binaryExpression->left()) {
                leftType = type(binaryExpression->left());
                binaryExpression->left()->type = leftType;
            }
            
            const TokenType optkntype { binaryExpression->op()->tkntype() };
            Operator op { optkntype };
            
            if (optkntype == TokenType::dot) {
                auto member = dynamic_cast<SymbolExpression*>(binaryExpression->right());
                auto call = dynamic_cast<Call*>(binaryExpression->right());
                if (leftType->isStruct() && member) {
                    auto structType = leftType->structValue();
                    auto iter = std::find_if(structType->dataMembers().begin(), structType->dataMembers().end(), [member](Statement* s) -> bool {
                        if (auto var = dynamic_cast<VarStatement*>(s)) {
                            return var->name().contents == member->value().contents;
                        }
                        return false;
                    });
                    if (iter != structType->dataMembers().end()) {
                        if (auto var = dynamic_cast<VarStatement*>(*iter)) {
                            return var->type();
                        }
                    } else {
                        return nullptr;
                    }
                } else if (leftType->isStruct() && call) {
                    for (auto arg: call->args) {
                        analyze(arg);
                    }
                    auto structType = leftType->structValue();
                    auto iter = std::find_if(structType->functionMembers().begin(), structType->functionMembers().end(), [call](Function* func) -> bool {
                        return call->name.contents == func->name().contents;
                    });
                    if (iter != structType->functionMembers().end()) {
                        for (auto param: (*iter)->parameters()) {
                            call->_spa_params.push_back(Function::Parameter(param.name, nullptr));
                            call->_spa_params.back().type = (Type*)malloc(sizeof(Type));
                            memcpy((void*)call->_spa_params.back().type, (const void*)param.type, sizeof(Type));
                        }
                        return const_cast<Type*>((*iter)->returnType());
                    } else {
                        return nullptr;
                    }
                } else {
                    report(
                           Error::typeDomain,
                           "Attempted to perform member access on non-struct data type",
                           binaryExpression->_loc.path,
                           binaryExpression->op()->_loc,
                           { binaryExpression->op()->_loc.pos, 0 }
                    );
                    return nullptr;
                }
            }
            
            if (binaryExpression->right()) {
                rightType = type(binaryExpression->right());
                binaryExpression->right()->type = rightType;
            }
            
            if (auto rtype = op.overload(leftType, rightType))
                return rtype;
            else {
                report(
                       Error::typeDomain,
                       "No such overload exists for the operation " + binaryExpression->op()->tkn().contents,
                       expr->_loc.path,
                       expr->_loc,
                       { expr->_loc.pos, 0 }
                );
                return nullptr;
            }
        } else if (Literal* literal = dynamic_cast<Literal*>(expr)) {
            switch (literal->type()) {
                case Literal::LType::boolean: {
                    return new Type(new Token(literal->value().loc, TokenType::boolType, "Bool"), true);
                }
                case Literal::LType::decimalInteger:
                case Literal::LType::hexadecimalInteger: {
                    return new Type(new Token(literal->value().loc, TokenType::int64Type, "Int64"), true);
                }
                case Literal::LType::decimalByte: {
                    return new Type(new Token(literal->value().loc, TokenType::charType, "Char"), true);
                }
                case Literal::LType::decimalWideChar: {
                    return new Type(new Token(literal->value().loc, TokenType::wideCharType, "WideChar"), true);
                }
                case Literal::LType::decimalShort: {
                    return new Type(new Token(literal->value().loc, TokenType::shortType, "Short"), true);
                }
                case Literal::LType::decimalInt32: {
                    return new Type(new Token(literal->value().loc, TokenType::int32Type, "Int32"), true);
                }
                case Literal::LType::decimalUInteger: {
                    return new Type(new Token(literal->value().loc, TokenType::uint64Type, "UInt64"), true);
                }
                case Literal::LType::decimalUByte: {
                    return new Type(new Token(literal->value().loc, TokenType::ucharType, "UChar"), true);
                }
                case Literal::LType::decimalWideUChar: {
                    return new Type(new Token(literal->value().loc, TokenType::wideUCharType, "WideUChar"), true);
                }
                case Literal::LType::decimalUShort: {
                    return new Type(new Token(literal->value().loc, TokenType::ushortType, "UShort"), true);
                }
                case Literal::LType::decimalUInt32: {
                    return new Type(new Token(literal->value().loc, TokenType::uint32Type, "UInt32"), true);
                }
                case Literal::LType::cString: {
                    return new Type(new Type(new Token(literal->value().loc, TokenType::charType, "Char"), true), literal->value().contents.size() + 1, true);
                }
                case Literal::LType::wideString: {
                    return new Type(new Type(new Token(literal->value().loc, TokenType::wideCharType, "WideChar"), true), literal->value()._wstr.size() + 1, true);
                }
                default:
                    break;
            }
        } else if (SymbolExpression* symbol = dynamic_cast<SymbolExpression*>(expr)) {
            Type* type = localLookupType(symbol->value().contents);
            if (!type) {
                if (auto gbl = lookupGlobal(symbol->value().contents)) type = gbl->type;
                if (!type) {
                    report(
                           Error::resolutionDomain,
                           "The symbol '" + symbol->value().contents + "' could not be found",
                           symbol->_loc.path,
                           symbol->_loc,
                           { symbol->_loc.pos, 0 },
                           "Try declaring or defining the symbol with a global, let or var to silence this error"
                        );
                    return nullptr;
                }
            }
            return type;
        } else if (Call* call = dynamic_cast<Call*>(expr)) {
            Function::Parameters argtypes;
            argtypes.reserve(call->args.size());
            for (auto arg: call->args) {
                analyze(arg);
                argtypes.push_back({
                    *Token::invalid,
                    arg->type
                });
            }
            for (auto param: argtypes) {
                call->_spa_params.push_back(Function::Parameter(param.name, nullptr));
                call->_spa_params.back().type = (Type*)malloc(sizeof(Type));
                memcpy((void*)call->_spa_params.back().type, (const void*)param.type, sizeof(Type));
            }
            Type* r = lookupRType(call->name.contents, argtypes);
            if (!r) {
                report(
                       Error::resolutionDomain,
                       "No function '" + call->generateTypeDescription() + "' exists",
                       call->_loc.path,
                       call->_loc,
                       { call->_loc.pos, 0 },
                       "Try forward-declaring or defining the function silence this error"
                );
            }
            return r;
        } else if (SizeOfType* sizeofexpr = dynamic_cast<SizeOfType*>(expr)) {
            return new Type(new Token(TokenLoc::zero, TokenType::int64Type, "Int64"), true);
        } else if (UnsafeCast* unsafecast = dynamic_cast<UnsafeCast*>(expr)) {
            analyze(unsafecast->expr());
            if (unsafecast->expr()->type->size() != unsafecast->type()->size()) {
                report(
                       Error::typeDomain,
                       "Cannot cast between types of different sizes",
                       unsafecast->_loc.path,
                       unsafecast->_loc,
                       { unsafecast->_loc.pos + 12, 0 }

                );
                return nullptr;
            }
            unsafecast->expr()->type = unsafecast->type();
            return unsafecast->type();
        } else if (auto constructor = dynamic_cast<ConstructExpression*>(expr)) {
            return constructor->type();
        } else if (auto arraylit = dynamic_cast<ArrayLiteralExpression*>(expr)) {
            if (arraylit->values().empty()) return nullptr;
            if (analyze(arraylit->values().front()) != 0) return nullptr;
            Type* t = arraylit->values().front()->type;
            for (auto val: arraylit->values()) {
                if (analyze(val) != 0) return nullptr;
                if (!(*t == *val->type)) {
                    report(Error::typeDomain, "Mismatching element types in array literal", arraylit->_loc.path, arraylit->_loc, { val->_loc.pos, 0 });
                    return nullptr;
                }
            }
            return new Type(t, arraylit->values().size());
        }
        return nullptr;
    }

    bool StaticAnalyzer::isStaticEval(Expression* expr) {
        if (!expr) {
            return true;
        }
        if (BinaryExpression* binaryExpression = dynamic_cast<BinaryExpression*>(expr)) {
            const bool lhsIsSE = (!binaryExpression->left() ? true : (binaryExpression->left()->info.isStaticEval = isStaticEval(binaryExpression->left())));
            const bool rhsIsSE = (!binaryExpression->right() ? true : (binaryExpression->right()->info.isStaticEval = isStaticEval(binaryExpression->right())));
            binaryExpression->info.isStaticEval = lhsIsSE && rhsIsSE;
            return binaryExpression->info.isStaticEval;
        } else if (auto literal = dynamic_cast<Literal*>(expr)) {
            expr->info.isStaticEval = true;
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
            if (globalSymbolTable.find(name) != globalSymbolTable.end()) {
                symbol->info.isStaticEval = true;
                return true;
            }
            if (auto fgbl = globalForwardDeclSymbolTable[name]) {
                fgbl->info.isStaticEval = true;
                return true;
            }
            if (auto initexpr = scope().lookup(name)) {
                return isStaticEval(initexpr);
            }
            return false;
        } else if (auto cast = dynamic_cast<UnsafeCast*>(expr)) {
            bool isSE = isStaticEval(cast->expr());
            cast->info.isStaticEval = isSE;
            return isSE;
        } else if (auto constructor = dynamic_cast<ConstructExpression*>(expr)) {
            return false;
        } else if (auto arraylit = dynamic_cast<ArrayLiteralExpression*>(expr)) {
            for (auto elem: arraylit->values()) {
                elem->info.isStaticEval = isStaticEval(elem);
                if (!elem->info.isStaticEval) {
                    arraylit->info.isStaticEval = false;
                    return false;
                }
            }
            arraylit->info.isStaticEval = true;
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

    void StaticAnalyzer::reset() {
        _errors.clear();
        _warnings.clear();
        globalSymbolTable.clear();
        globalForwardDeclSymbolTable.clear();
        functionSymbolTable.clear();
        functionForwardDeclSymbolTable.clear();
        scopes.clear();
        _typeTrace.clear();
    }
}
