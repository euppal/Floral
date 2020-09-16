////
////  Compiler.cpp
////  floral
////
////  Created by Ethan Uppal on 7/3/20.
////  Copyright © 2020 Ethan Uppal. All rights reserved.
////
//
//#include "Compiler.hpp"
//
//#include "AST.hpp"
//#include "File IO.hpp"
//#include "SPA.hpp"
//#include <iostream>
//
//
//namespace Floral { namespace v1 {
//    void Compiler::report(Error::Domain domain, const std::string& text, TextRegion loc) {
//        Error err {domain, text, loc};
//        _errors.push_back(err);
//    }
//    bool Compiler::hasErrors() const {
//        return !_errors.empty();
//    }
//    const std::vector<Error>& Compiler::errors() const {
//        return _errors;
//    }
//    void Compiler::setOutputDestination(const std::string &dest) {
//        outputDest = dest;
//    }
//    void Compiler::reset() {
//        textSection.clear();
//        dataSection = "\nsection .data\n";
//        rodataSection = "\nsection .rodata\n";
//        bssSection = "\nsection .bss\n";
//    }
//    const std::pair<std::string, bool> Compiler::staticEvalulate(Expression* staticEvalExpr, bool wantsFinalSymbol) {
//        exit(1);
//    }
//    void Compiler::compile(const File *file) {
//        reset();
//        
//        analyzer.analyze(file);
//        
//        // Compile
//        if (file->main()) {
//            function(file->main());
//        }
//        
//        for (auto node: file->nodes()) {
//            if (auto decl = dynamic_cast<Declaration*>(node))
//                declaration(decl);
//            else if (auto stm = dynamic_cast<Statement*>(node))
//                statement(stm);
//            else {
//                report(Error::compileDomain, "An expresion cannot be a top-level statement", node->_loc);
//                return;
//            }
//        }
//        
//        textSection.append(
//            "\nglobal _main\n"
//            "_main:\n"
//            "  call " FLORAL_ID_PREFIX "main\n"
//            "  mov rdi, rax\n"
//            "  mov rax, 0x2000001\n"
//            "  syscall\n"
//        );
//        
//        std::string out {};
//        if (!textSection.empty()) {
//            out.append(textSection);
//        }
//        if (dataSection != "\nsection .data\n") out.append(dataSection);
//        if (rodataSection != "\nsection .rodata\n") out.append(rodataSection);
//        if (bssSection != "\nsection .bss\n") out.append(bssSection);
//        
//        Floral::write(outputDest, out);
//    }
//
//    void Compiler::declaration(Declaration *decl) {
//        if (auto func = dynamic_cast<Function*>(decl)) {
//            function(func);
//        } else if (auto gbl = dynamic_cast<GlobalDeclaration*>(decl)) {
//            global(gbl);
//        } else if (auto fgbl = dynamic_cast<GlobalForwardDeclaration*>(decl)) {
//            forwardGlobal(fgbl);
//        } else if (auto letdecl = dynamic_cast<LetDeclaration*>(decl)) {
//            let(letdecl);
//        }
//    }
//    void Compiler::function(Function *func) {
//        frames.push_back({}); // new frame
//        frames.back().data.push_back({ 0, 8, "Old RBP" });
//        
//        const size_t staticAllocationSize {func->staticAllocationSize};
//        textSection.append(
//            FLORAL_ID_PREFIX + func->name().contents + ":\n"
//            "  push rbp ; store old frame\n"
//            "  mov rbp, rsp ; create new frame\n\n"
//        );
//        if (staticAllocationSize) textSection.append("  sub rsp, " + std::to_string(staticAllocationSize) + '\n');
//        for (auto node: func->body()) {
//            if (auto stm = dynamic_cast<Statement*>(node)) statement(stm);
//            if (auto decl = dynamic_cast<Declaration*>(node)) declaration(decl);
//        }
//        
//        // Integer return values up to 64 bits in size are stored in RAX while values up to 128 bit are stored in RAX and RDX
//        
//        frames.pop_back(); // pop frame
//    }
//    void Compiler::global(GlobalDeclaration *gbl) {
//        const auto type { gbl->type };
//        const std::string gblid {FLORAL_ID_PREFIX + gbl->name.contents};
//
//        if (gbl->isZeroInitialized()) {
//            if (type->isChar() || type->isUChar()) bssSection.append("  " + gblid + ": " ZERO_INIT_CHAR);
//            else if (type->isChar() || type->isUChar()) bssSection.append("  " + gblid + ": " ZERO_INIT_SHORT);
//            else if (type->isChar() || type->isUChar()) bssSection.append("  " + gblid + ": " ZERO_INIT_INT32);
//            else if (type->isInt()|| type->isUInt()) bssSection.append("  " + gblid + ": " ZERO_INIT_INT64);
//        } else if (gbl->initializer()->type == Initializer::direct) {
////            if (Literal* literal = dynamic_cast<Literal*>(value->components()[0])) {
////                definedGlobalsTable[gbl->name.contents] = gbl;
////                if (type->isChar() || type->isUChar()) rodataSection.append("  " + gblid + ": db " + literal->value().contents + '\n');
////                else if (type->isChar() || type->isUChar()) rodataSection.append("  " + gblid + ": dw " + literal->value().contents + '\n');
////                else if (type->isChar() || type->isUChar()) rodataSection.append("  " + gblid + ": dd " + literal->value().contents + '\n');
////                else if (type->isInt()|| type->isUInt()) rodataSection.append("  " + gblid + ": dq " + literal->value().contents + '\n');
////            } else if (SymbolExpression* symbol = dynamic_cast<SymbolExpression*>(value->components()[0])) {
////                if (GlobalDeclaration* innerGbl = analyzer.lookupGlobal(symbol->value().contents)) {
////                    if (!definedGlobalsTable[innerGbl->name.contents]) {
////                        global(innerGbl);
////                    }
////                }
////            }
//        } else if (gbl->initializer()->type == Initializer::copy) {
////            if (Literal* literal = dynamic_cast<Literal*>(value->components()[0])) {
////                definedGlobalsTable[gbl->name.contents] = gbl;
////                if (type->isChar() || type->isUChar()) rodataSection.append("  " + gblid + ": db " + literal->value().contents + '\n');
////                else if (type->isChar() || type->isUChar()) rodataSection.append("  " + gblid + ": dw " + literal->value().contents + '\n');
////                else if (type->isChar() || type->isUChar()) rodataSection.append("  " + gblid + ": dd " + literal->value().contents + '\n');
////                else if (type->isInt()|| type->isUInt()) rodataSection.append("  " + gblid + ": dq " + literal->value().contents + '\n');
////            } else if (SymbolExpression* symbol = dynamic_cast<SymbolExpression*>(value->components()[0])) {
////                if (GlobalDeclaration* innerGbl = analyzer.lookupGlobal(symbol->value().contents)) {
////                    if (!definedGlobalsTable[innerGbl->name.contents]) {
////                        global(innerGbl);
////                    }
////                }
////            }
//        }
//    }
//    void Compiler::forwardGlobal(GlobalForwardDeclaration* fgbl) {
//        const std::string gblid {FLORAL_ID_PREFIX + fgbl->name().contents};
//        textSection.append("extern " + gblid + '\n');
//    }
//    void Compiler::let(LetDeclaration *l) {
//        switch (l->initializer()->type) {
//            case Initializer::zero: {
//                const size_t size = l->type()->size();
//                const Variable c { RBPOffsetLocation(frames.back().nextOffset()), size, l->name().contents };
//                frames.back().data.push_back(c);
//                textSection += "  mov ";
//                switch (size) {
//                    case 1:
//                        textSection += "BYTE ";
//                        break;
//                    case 2:
//                        textSection += "WORD ";
//                        break;
//                    case 4:
//                        textSection += "DWORD ";
//                        break;
//                    case 8:
//                        textSection += "QWORD ";
//                        break;
//                    default:
//                        break;
//                }
//                textSection += "[rbp-" + std::to_string(c.loc.stack) + "], 0 ; let " + l->name().contents + "\n\n";
//                break;
//            }
//            case Initializer::direct: {
//                const auto init {static_cast<const DirectInitializer*>(l->initializer())};
//                const std::string result = expression(init->expr());
//                const size_t size = l->type()->size();
//                const Variable c { RBPOffsetLocation(frames.back().nextOffset()), size, l->name().contents };
//                frames.back().data.push_back(c);
//                textSection += "  mov ";
//                switch (size) {
//                    case 1:
//                        textSection += "BYTE ";
//                        break;
//                    case 2:
//                        textSection += "WORD ";
//                        break;
//                    case 4:
//                        textSection += "DWORD ";
//                        break;
//                    case 8:
//                        textSection += "QWORD ";
//                        break;
//                    default:
//                        break;
//                }
//                textSection += "[rbp-" + std::to_string(c.loc.stack) + "], " + result + " ; let " + l->name().contents + "\n\n";;
//                break;
//            }
//            case Initializer::copy: {
//                break;
//            }
//        }
//    }
//    void Compiler::var(VarDeclaration *v) {
//        
//    }
//
//    void Compiler::statement(Statement *stm) {
//        if (auto callStm = dynamic_cast<CallStatement*>(stm)) {
//            this->callStm(callStm);
//        } else if (auto rtnStm = dynamic_cast<ReturnStatement*>(stm)) {
//            returnStm(rtnStm);
//        }
//    }
//    void Compiler::callStm(CallStatement *callStm) {
//        this->call(callStm->call);
//    }
//    void Compiler::returnStm(ReturnStatement *rtnStm) {
//        if (auto value = rtnStm->value()) {
//            const std::string result = expression(value);
//            if (result != "rax") {
//                textSection.append("  mov rax, " + result +  " ; result to be returned\n");
//            }
//            returnRegister(result);
//        }
//        textSection.append(
//            "  leave ; restore old frame\n"
//            "  ret ; return from function\n\n"
//        );
//        
//    }
//
//    void Compiler::returnRegister(const std::string& reg) {
//        for (int i = 0; i < static_cast<int>(Register::r15); i++) {
//            if (registerNames[i] == reg) {
//                frames.back().returnScratchRegister(static_cast<Register>(i));
//            }
//        }
//    }
//    void Compiler::push(const std::vector<Register>& registers) {
//        for (std::vector<Register>::const_reverse_iterator i = registers.rbegin(); i != registers.rend(); ++i) {
//            textSection.append("  push " + registerNames[static_cast<int>(*i)] + '\n');
//        }
//    }
//    void Compiler::pop(const std::vector<Register>& registers) {
//        for (auto reg: registers) {
//            textSection.append("  pop " + registerNames[static_cast<int>(reg)] + '\n');
//        }
//    }
//    const std::string Compiler::emitBinaryExpr(Expression* left, Expression* right, const std::string& opName, const std::string& opDes) {
//        const std::string lhs = expression(left);
//        const std::string rhs = expression(right);
//        
//        // avaliableScratch can return -1 if no regs avaliable, but ignore for now
//        const std::string resultr = registerNames[frames.back().avaliableScratch()];
//        const std::string rhsr = registerNames[frames.back().avaliableScratch()];
//        
//        textSection.append(
//            "  mov " + resultr + ", " + lhs + "\n"
//            "  mov " + rhsr + ", " + rhs + "\n"
//            "  " + opName + " " + resultr + ", " + rhsr + " ; "  + opDes + "\n\n"
//        );
//        
//        returnRegister(rhs);
//        returnRegister(lhs);
//        returnRegister(rhsr);
//        
//        return resultr;
//    }
//
//    std::string Compiler::expression(Expression *expr) {
//        
//        if (auto binary = dynamic_cast<BinaryExpression*>(expr)) {
//            // assume true binary expression for now
//            Expression* left {binary->left()};
//            const OperatorComponentExpression* op {binary->op()};
//            Expression* right {binary->right()};
//            
//            switch (op->tkntype()) {
//                case TokenType::plus: {
//                    return emitBinaryExpr(left, right, "add", "sum");
//                }
//                case TokenType::minus: {
//                    return emitBinaryExpr(left, right, "sub", "difference");
//                }
//                case TokenType::multiply: {
//                    return emitBinaryExpr(left, right, "imul", "product");
//                }
//                case TokenType::divide: {
//                    // idiv uses rax/rdx
//                    push({Register::rax, Register::rdx});
//                    frames.back().registersInUse.push_back(Register::rax);
//                    frames.back().registersInUse.push_back(Register::rdx);
//                    
//                    const std::string lhs = expression(left);
//                    const std::string rhs = expression(right);
//                    
//                    
//                    // avaliableScratch can return -1 if no regs avaliable, but ignore for now
//                    const std::string resultr = registerNames[frames.back().avaliableScratch()];
//                    const std::string rhsr = registerNames[frames.back().avaliableScratch()];
//                    
//                    textSection.append(
//                        "  mov rdx, 0\n" // set to one of rax < 0
//                        "  mov rax, " + lhs + "\n"
//                        "  mov " + rhsr + ", " + rhs + "\n"
//                        "  idiv " + rhsr + "; quotient\n"
//                        "  mov " + resultr + ", rax\n"
//                    );
//                    
//                    returnRegister(rhs);
//                    returnRegister(lhs);
//                    returnRegister(rhsr);
//                    
//                    pop({Register::rax, Register::rdx});
//                    frames.back().returnScratchRegister(Register::rax);
//                    frames.back().returnScratchRegister(Register::rdx);
//                    
//                    textSection.push_back('\n');
//                    
//                    return resultr;
//                }
//                default: {
//                    // Unsupported operation
//                    exit(1);
//                    break;
//                }
//            }
//        } else if (auto literal = dynamic_cast<Literal*>(expr)) {
//            const std::string des {literal->description()};
//            if (des == TYPE_STRING_INDICATOR) {
//                static long strlitCount = 0;
//                const std::string lbl {"strlit" + std::to_string(strlitCount++)};
//                rodataSection += "  " + lbl + ": `" +  literal->value().contents + "`\n";
//                const std::string resultr = registerNames[frames.back().avaliableScratch()];
//                textSection += "  mov QWORD " + resultr + ", [rel " + lbl + "]; string literal\n";
//                return resultr;
//            }
//            return des;
//        } else if (auto symbol = dynamic_cast<SymbolExpression*>(expr)) {
//            // MARK: literally looks for defined stuff in this frame will fix later
//            const auto index = std::find_if(frames.back().data.begin(), frames.back().data.end(), [symbol](Variable v){
//                return v.name == symbol->value().contents;
//            });
//            if (index != frames.back().data.end()) {
//                const long offset {(*index).loc.stack};
//                const size_t size {(*index).size};
//                
//                const std::string resultr = registerNames[frames.back().avaliableScratch()];
//                textSection += "  mov ";
//                switch (size) {
//                    case 1:
//                        textSection += "BYTE ";
//                        break;
//                    case 2:
//                        textSection += "WORD ";
//                        break;
//                    case 4:
//                        textSection += "DWORD ";
//                        break;
//                    case 8:
//                        textSection += "QWORD ";
//                        break;
//                    default:
//                        break;
//                }
//                textSection.append(
//                     resultr + ", [rbp-" + std::to_string(offset) + "] ; store " + (*index).name + " in " + resultr + "\n\n"
//                );
//                
//                return resultr;
//            }
//        } else if (auto call = dynamic_cast<Call*>(expr)) {
//            this->call(call);
//            return "rax";
//        }
//        
//        // Should not reach here
//        exit(1);
//    }
//
//    size_t Compiler::arguments(const std::vector<Expression*>& args) {
//        // https://en.wikipedia.org/wiki/X86_calling_conventions#System_V_AMD64_ABI -
//        // The first six integer or pointer arguments are passed in registers RDI, RSI, RDX, RCX, R8, R9
//        // while XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6 and XMM7 are used for the first floating point arguments. As in the Microsoft x64 calling convention, additional arguments are passed on the stack.
//        const size_t size = args.size();
//        if (size == 0) return 0;
//        
//        int integers{};
////        int floats{};
//        
//        static const Register integerRegs[] {
//            Register::rdi, Register::rsi, Register::rdx, Register::rcx, Register::r8, Register::r9
//        };
////        static const Register floatRegs[] {
////
////        };
//        
//        std::vector<Expression*> stackArgs;
//        stackArgs.reserve(max(6, size) - 6);
//        
//        for (size_t index = 0; index < size; index++) {
//            Expression* arg {args[index]};
//            if (true || arg->type->isInteger()) {
//                if (integers >= 6) { // only 6 registers for arguments
//                    stackArgs.push_back(arg);
//                } else {
//                    const std::string reg = registerNames[static_cast<int>(integerRegs[integers++])];
//                    const std::string result = expression(arg);
//                    if (result != reg) textSection += "  mov " + reg + ", " + result + " ; Argument # " + std::to_string(index) + "\n";
//                }
//            }
////            else {
////                floats++;
////            }
//        }
//        
//        const size_t stackArgC = stackArgs.size();
//        while (!stackArgs.empty()) {
//            Expression* arg = stackArgs.back();
//            stackArgs.pop_back();
//            
//            const std::string result = expression(arg);
//            textSection += "  push " + result + " ; Push argument onto stack\n";
//        }
//        return stackArgC;
//    }
//
//
//    void Compiler::call(Call *call) {
//        const std::vector<Register> registersInUse {frames.back().registersInUse};
//        push(registersInUse);
//        
//        const size_t stackArgC = arguments(call->args);
//        textSection += "  call " FLORAL_ID_PREFIX + call->name.contents + " ; Call function\n";
//        if (stackArgC) textSection += "  add rsp, " + std::to_string(stackArgC * 8) + " ; Remove args\n\n";
//        
//        pop(registersInUse);
//
//    }
//}}
