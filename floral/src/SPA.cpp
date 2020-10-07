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
    void StaticAnalyzer::report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {domain, text, loc, errloc};
        err.fix = fix;
        err.path = _path;
        _errors.push_back(err);
    }
    void StaticAnalyzer::warn(const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {Error::warning, text, loc, errloc};
        err.fix = fix;
        err.isWarning = true;
        _warnings.push_back(err);
    }
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
        } else if (auto exprStm = dynamic_cast<ExpressionStatement*>(stm)) {
            if (analyze(exprStm->expr()) != 0) return 1;
        } else if (auto ptrAssignStm = dynamic_cast<PointerAssignment*>(stm)) {
            analyze(ptrAssignStm->ptrExpr());
            analyze(ptrAssignStm->newValue());
            if (!ptrAssignStm->ptrExpr()->type->isPointer()) {
                report(
                       Error::typeDomain,
                       "Cannot perform pointer assignment to non-pointer type",
                       ptrAssignStm->_loc,
                       { ptrAssignStm->_loc.pos, 0 },
                       "Maybe insert '&' before the left hand side expression"
                );
                return 1;
            }
            if (ptrAssignStm->ptrExpr()->type->_ptrType->isConst()) {
                report(
                       Error::typeDomain,
                       "Cannot assign to const value",
                       ptrAssignStm->_loc,
                       { ptrAssignStm->ptrExpr()->_loc.pos, 0 },
                       "Try changing the declaration of the pointer's value to a variable"
                );
                return 1;
            }
            if (!(*ptrAssignStm->ptrExpr()->type->_ptrType == *ptrAssignStm->newValue()->type)) {
                report(
                       Error::typeDomain,
                       "Cannot assign value of type " + ptrAssignStm->newValue()->type->des() + " to pointer to value of type " + ptrAssignStm->ptrExpr()->type->_ptrType->des(),
                       ptrAssignStm->_loc,
                       { ptrAssignStm->newValue()->_loc.pos, 0 }
                );
                return 1;
            }
        } else if (auto assignStm = dynamic_cast<Assignment*>(stm)) {
            analyze(assignStm->lval());
            analyze(assignStm->rval());
            if (assignStm->lval()->type->isConst()) {
                report(
                       Error::typeDomain,
                       "Cannot assign to const value",
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
                       assignStm->_loc,
                       { assignStm->rval()->_loc.pos, 0 }
                );
                return 1;
            }
        } else if (auto ifStm = dynamic_cast<IfStatement*>(stm)) {
            if (analyze(ifStm->condition()) != 0) return 1;
            if (!ifStm->condition()->type->isBool()) {
                report(
                       Error::typeDomain,
                       "If statement condition does not resolve to boolean",
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
                           const Type* declaredType {let->type()};
                           if (!declaredType->isIncomplete()) {
                               if (!(*declaredType == *initexpr->type)) {
                                   if (declaredType->isUInt() && initexpr->type->isInt()) {
                                       
                                   } else {
                                       report(
                                              Error::typeDomain,
                                              "Expression resolves to type different from type declared in let declaration",
                                              let->_loc,
                                              { let->_loc.pos, 0 }
                                       );
                                   }
                               }
                           } else {
                               let->setType(initexpr->type);
                           }
                           if (scope().exists(let->name().contents)) {
                               report(
                                      Error::resolutionDomain,
                                       "Invalid redeclaration of '" + let->name().contents + "'",
                                      let->_loc,
                                      { let->_loc.pos + 3, let->name().contents.size() }
                               );
                               return 1;
                           }
                           scope().insert(let->name().contents, const_cast<Type*>(let->type()), initexpr);
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
                                   if (declaredType->isUInt() && initexpr->type->isInt()) {
                                       
                                   } else {
                                       report(
                                              Error::typeDomain,
                                              "Expression resolves to type different from type declared in let declaration",
                                              let->_loc,
                                              { let->_loc.pos, 0 }
                                       );
                                   }
                               }
                           } else {
                               let->setType(initexpr->type);
                           }
                           if (scope().exists(let->name().contents)) {
                               report(
                                      Error::resolutionDomain,
                                       "Invalid redeclaration of '" + let->name().contents + "'",
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
                       warn(
                            "Variable is unitialized",
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
                                              let->_loc,
                                              { let->_loc.pos, 0 }
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
                                              let->_loc,
                                              { let->_loc.pos, 0 }
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
            pushScope();
            scope().func = func;
            if (functionExists(func->name().contents, func->parameters())) {
                report(
                       Error::resolutionDomain,
                       "Invalid redeclaration of '" + func->name().contents + "'",
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
                func->setRType(new Type(new Token({0,0}, TokenType::voidType, "Void"), true));
            }
            if (func->returnType()->isVoid() && (!func->body().empty() ? !dynamic_cast<ReturnStatement*>(func->body().back()) : true)) {
                func->insert(new ReturnStatement({ func->_loc.pos + func->_loc.length - 1, 0, func->_loc.endLine, func->_loc.endLine }, nullptr));
            }
            for (auto node: func->body()) {
                if (auto stm = dynamic_cast<Statement*>(node)) {
                    if (analyze(stm) != 0) return 1;
                    if (auto let = dynamic_cast<LetStatement*>(decl)) {
                        func->staticAllocationSize += let->type()->size();
                    } else if (auto var = dynamic_cast<VarStatement*>(decl)) {
                        func->staticAllocationSize += var->type()->size();
                    }
                } else if (auto decl = dynamic_cast<Declaration*>(node)) {
                    if (analyze(decl) != 0) return 1;
                }
            }
            popScope();
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
        }
        return 0;
    }

    int StaticAnalyzer::analyze(Expression* expr) {
        expr->type = type(expr); // get the type
        _typeTrace.push_back(expr);
        if (!expr->type) {
            return 1;
        }
        return 0;
    }

    void StaticAnalyzer::dumpTypeTrace() const {
        ColoredStream out;
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
            if (binaryExpression->right()) {
                rightType = type(binaryExpression->right());
                binaryExpression->right()->type = rightType;
            }
            
            const TokenType tkntype { binaryExpression->op()->tkntype() };
            Operator op { tkntype };
            if (auto rtype = op.overload(leftType, rightType))
                return rtype;
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
            // MARK: BADLY BROKEN
            switch (literal->type()) {
                case Literal::LType::boolean: {
                    return new Type(new Token({0,0}, TokenType::boolType, "Bool"), true);
                }
                case Literal::LType::decimalInteger:
                case Literal::LType::hexadecimalInteger: {
                    return new Type(new Token({0,0}, TokenType::int64Type, "Int"), true); // MARK: NOOOO IT CAN BE ANY INTEGER LITERAL TYPEEEEEE
                }
                case Literal::LType::decimalByte: {
                    return new Type(new Token({0,0}, TokenType::charType, "Char"), true); // MARK: NOOOO IT CAN BE ANY INTEGER LITERAL TYPEEEEEE
                }
                case Literal::LType::decimalShort: {
                    return new Type(new Token({0,0}, TokenType::shortType, "Short"), true); // MARK: NOOOO IT CAN BE ANY INTEGER LITERAL TYPEEEEEE
                }
                case Literal::LType::decimalInt32: {
                    return new Type(new Token({0,0}, TokenType::int32Type, "Int32"), true); // MARK: NOOOO IT CAN BE ANY INTEGER LITERAL TYPEEEEEE
                }
                case Literal::LType::decimalUInteger: {
                    return new Type(new Token({0,0}, TokenType::uint64Type, "UInt"), true); // MARK: NOOOO IT CAN BE ANY INTEGER LITERAL TYPEEEEEE
                }
                case Literal::LType::decimalUByte: {
                    return new Type(new Token({0,0}, TokenType::ucharType, "UChar"), true); // MARK: NOOOO IT CAN BE ANY INTEGER LITERAL TYPEEEEEE
                }
                case Literal::LType::decimalUShort: {
                    return new Type(new Token({0,0}, TokenType::ushortType, "UShort"), true); // MARK: NOOOO IT CAN BE ANY INTEGER LITERAL TYPEEEEEE
                }
                case Literal::LType::decimalUInt32: {
                    return new Type(new Token({0,0}, TokenType::uint32Type, "UInt32"), true); // MARK: NOOOO IT CAN BE ANY INTEGER LITERAL TYPEEEEEE

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
                analyze(arg);
                argtypes.push_back({
                    *Token::invalid,
                    arg->type
                });
            }
            call->_spa_params = argtypes;
            Type* r = lookupRType(call->name.contents, argtypes);
            if (!r) {
                report(
                       Error::resolutionDomain,
                       "No function '" + call->generateTypeDescription() + "' exists",
                       call->_loc,
                       { call->_loc.pos, 0 },
                       "Try forward-declaring or defining the function silence this error"

                );
            }
            return r;
        } else if (SizeOfType* sizeofexpr = dynamic_cast<SizeOfType*>(expr)) {
            return new Type(new Token({0,0}, TokenType::uint64Type, "UInt"), true);
        } else if (UnsafeCast* unsafecast = dynamic_cast<UnsafeCast*>(expr)) {
            analyze(unsafecast->expr());
            if (unsafecast->expr()->type->size() != unsafecast->type()->size()) {
                report(
                       Error::typeDomain,
                       "Cannot cast between types of different sizes",
                       unsafecast->_loc,
                       { unsafecast->_loc.pos + 12, 0 }

                );
                return nullptr;
            }
            unsafecast->expr()->type = unsafecast->type();
            return unsafecast->type();
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
            if (auto initexpr = scope().lookup(name)) return isStaticEval(initexpr);
            return false;
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
