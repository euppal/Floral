//
//  Compiler.cpp
//  floral
//
//  Created by Ethan Uppal on 7/3/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Compiler.hpp"

#include "AST.hpp"
#include "Files.hpp"

namespace Floral {
    void Compiler::setOutputDestination(const std::string &dest) {
        outputDest = dest;
    }
    void Compiler::compile(const File *file) {
        std::string out {
            "global _main\nsection .text\n\n"
        };
        
        // Compile
        for (auto node: file->nodes()) {
            if (auto decl = dynamic_cast<Declaration*>(node))
                declaration(decl, out);
            else if (auto stm = dynamic_cast<Statement*>(node))
                statement(stm, out);
            else
                // idk error
                ;
        }
        mainFunction(file->main(), out);
        
        Floral::write(outputDest, out);
    }

    void Compiler::mainFunction(Function *main, std::string &out) {
        out.append("_main:\n");
        for (auto stm: main->body()) {
            statement(stm, out);
        }
        out.append("\n  mov rax, 0x2000001\n  xor rdi, rdi\n  syscall");
        out.append("\n");
    }

    void Compiler::declaration(Declaration *decl, std::string &out) {
        if (auto func = dynamic_cast<Function*>(decl)) {
            function(func, out);
        }
    }
    void Compiler::function(Function *func, std::string &out) {
        out.append(func->name().contents + ":\n");
        for (auto stm: func->body()) {
            statement(stm, out);
        }
        out.append("  ret\n\n");
    }

    void Compiler::statement(Statement *stm, std::string &out) {
        if (auto callStm = dynamic_cast<CallStatement*>(stm)) {
            this->callStm(callStm, out);
        }
    }
    void Compiler::callStm(CallStatement *callStm, std::string &out) {
        // push args
        // (add a path for funcs marked inline)
        out.append("  call " + callStm->name());
        // result should now be pushed on stack
    }

    void Compiler::expression(Expression *expr, std::string &out) {
        
    }
    void Compiler::call(Call *call, std::string &out) {
        
    }
}
