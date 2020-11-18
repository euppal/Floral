//
//  SPA.hpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 7/21/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef SPA_hpp
#define SPA_hpp

#include "AST.hpp"
#include "Error.hpp"
#include "Scope.hpp"
#include <map>

namespace Floral {
    typedef std::pair<std::string, Function::Parameters> FunctionSignature;

    class StaticAnalyzer: public ErrorReporting {
        friend class Compiler;
        
        std::string _path;
        int _warnUninit = true;
        
        std::string strFromFunctionSignature(FunctionSignature funsig);
        Type* localLookupType(const std::string& id);
        std::unordered_map<std::string, GlobalDeclaration*> globalSymbolTable;
        std::unordered_map<std::string, GlobalForwardDeclaration*> globalForwardDeclSymbolTable;
        std::unordered_map<std::string, Function*> functionSymbolTable;
//        std::unordered_map<std::string, FunctionOverloads> functionSymbolTable;
        std::unordered_map<std::string, FunctionForwardDeclaration*> functionForwardDeclSymbolTable;
        
        std::vector<Scope> scopes;
        void pushScope(void);
        void popScope(void);
        Scope& scope(void);
        
        int analyze(Declaration* decl);
        int analyze(Statement* stm);
        int analyze(Expression* expr);
        Type* type(Expression* expr);
        bool isStaticEval(Expression* expr);
        Declaration* lookupFunction(const std::string& name, const Function::Parameters& params);
        Type* lookupRType(const std::string& name, const Function::Parameters& params);
        Function* currentFunc();
        
        std::vector<Expression*> _typeTrace;
        
    public:
        int analyze(const File* file);
        
        bool hasErrors() const;
        const std::vector<Error>& errors() const;
        bool hasWarnings() const;
        const std::vector<Error>& warnings() const;
        void dumpTypeTrace(void) const;
        
        GlobalDeclaration* lookupGlobal(std::string symbol);
        GlobalForwardDeclaration* lookupGlobalDecl(std::string symbol);
        
        void reset(void);
    };
}

#endif /* SPA_hpp */
