//
//  Compiler.hpp
//  floral
//
//  Created by Ethan Uppal on 7/3/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Compiler_hpp
#define Compiler_hpp

#include "AST.hpp"

namespace Floral {
    class Compiler {
        std::string outputDest;
        
        public:
        void setOutputDestination(const std::string &dest);
        
        void compile(const File *file);
        
        void mainFunction(Function *main, std::string &out);
        
        void declaration(Declaration *decl, std::string &out);
        void function(Function *func, std::string &out);
        
        void statement(Statement *stm, std::string &out);
        void callStm(CallStatement *callStm, std::string &out);
        
        void expression(Expression *expr, std::string &out);
        void call(Call *call, std::string &out);
    };
}

#endif /* Compiler_hpp */
