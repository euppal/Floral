//
//  Compiler_v2.cpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 9/9/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Compiler.hpp"
#include "File IO.hpp"
#include <cassert>

namespace Floral { namespace v2 {
    // MARK: Constructor/Deinitializer
    Compiler::Compiler(): textSection(SectionType::text), bssSection(SectionType::bss), rodataSection(SectionType::rodata), dataSection(SectionType::data) {}
    Compiler::~Compiler() {}

    // MARK: Error and utility
    void Compiler::report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {domain, text, loc, errloc};
        err.fix = fix;
        _errors.push_back(err);
    }
    bool Compiler::hasErrors() const {
        return !_errors.empty();
    }
    const std::vector<Error>& Compiler::errors() const {
        return _errors;
    }
    void Compiler::setOutputDestination(const std::string &dest) {
        outputDest = dest;
    }
    void Compiler::reset() {
        textSection.instructions.clear();
        bssSection.instructions.clear();
        rodataSection.instructions.clear();
        dataSection.instructions.clear();
        
        textSection.spaceOutLabels = true;
    }

    // MARK: General emission
    void Compiler::emit(Instruction* instr, SectionType section) {
        switch (section) {
            case SectionType::text: {
                textSection.add(instr);
                break;
            }
            case SectionType::bss: {
                bssSection.add(instr);
                break;
            }
            case SectionType::rodata: {
                rodataSection.add(instr);
                break;
            }
            case SectionType::data: {
                dataSection.add(instr);
                break;
            }
        }
    }

    // MARK: Emit general statement
    void Compiler::emitStatement(Statement *stm) {
        if (auto callStm = dynamic_cast<CallStatement*>(stm)) {
            emitCallStatement(callStm); // perform call with necessary setup/teardown
        } else if (auto rtnStm = dynamic_cast<ReturnStatement*>(stm)) {
            emitReturnStatement(rtnStm); // get a specific value into RETURN_VALUE_LOC (Register::rax)
        }
    }

    // MARK: Call statement
    void Compiler::emitCallStatement(CallStatement *callStm) {
        emitCall(callStm->call); /* call statements && calls are the same
                                  * thing in assembly, however plain calls
                                  * can also be used in the context of expressions */
    }

    // MARK: Return specific value
    void Compiler::emitReturnSpecificValue(Location src) {
        if (src.reg != static_cast<int>(Register::rax)) { // if the expression result isn't already in rax...
            emit(new MoveOperation(RegisterLocation(Register::rax), src, SizeType::qword, "result to be returned"), SectionType::text); // ...move it to rax
        }
    }

    // MARK: Return statement
    void Compiler::emitReturnStatement(ReturnStatement *rtnStm) {
        if (rtnStm->value()) { // if we are retruning a value...
            if (auto literal = dynamic_cast<Literal*>(rtnStm->value())) {
                if (literal->value().contents == "0") { // if returning zero
                    emit(new XorOperation(RETURN_VALUE_LOC_32b, RETURN_VALUE_LOC_32b, "result to be returned"), SectionType::text); // then just xor rax, rax
                } else {
                    Location result = emitExpression(rtnStm->value()); // calculate the value
                    emitReturnSpecificValue(result); // put the result in the RETURN_VALUE_LOC
                    if (result.reg != LOC_IS_NOT_REG) returnRegister(static_cast<Register>(result.reg));
                }
            } else { // otherwise
                Location result = emitExpression(rtnStm->value()); // calculate the value
                emitReturnSpecificValue(result); // put the result in the RETURN_VALUE_LOC
                if (result.reg != LOC_IS_NOT_REG) returnRegister(static_cast<Register>(result.reg));
            }
        }
        emitLeave(); // pop frame
        emitRet(); // restore old point of execution
    }

    // MARK: Emit general declaration
    void Compiler::emitDeclaration(Declaration *decl) {
        if (auto func = dynamic_cast<Function*>(decl)) {
            emitFunction(func);
        } else if (auto ffunc = dynamic_cast<FunctionForwardDeclaration*>(decl)) {
            emitExternFunc(ffunc);
        } else if (auto gbl = dynamic_cast<GlobalDeclaration*>(decl)) {
            emitGlobal(gbl);
        } else if (auto fgbl = dynamic_cast<GlobalForwardDeclaration*>(decl)) {
            emitExternGlobal(fgbl);
        } else if (auto letdecl = dynamic_cast<LetDeclaration*>(decl)) {
            emitLocalConst(letdecl);
        } else if (auto vardecl = dynamic_cast<VarDeclaration*>(decl)) {
            emitLocalVar(vardecl);
        }
    }

    // MARK: Emit global constant
    void Compiler::emitGlobal(GlobalDeclaration *gbl) {
        
    }

    // MARK: Emit forward-declared global constant
    void Compiler::emitExternGlobal(GlobalForwardDeclaration *fgbl) {
        emit(new Extern(fgbl->name().contents), SectionType::text);
    }

    // MARK: Emit local variable
    void Compiler::emitLocalVar(VarDeclaration *v) {
        switch (v->initializer()->type) {
            case Initializer::zero: {
                const size_t size = v->type()->size();
                const std::string name = v->name().contents;
                
                emit(
                    new MoveOperation(
                        RBPOffsetLocation(-currentFrame().nextOffset()),
                        NumLL(false, SU(0ULL)),
                        OPSIZE_FROM_NUM(size),
                        "var " + name + " = 0"
                    ),
                    SectionType::text
                );
                currentFrame().addData(RBPOffsetLocation(currentFrame().nextOffset()), size, name);

                break;
            }
            case Initializer::direct: {
                const auto init {static_cast<const DirectInitializer*>(v->initializer())};
                const Location result = emitExpression(init->expr());
                const size_t size = v->type()->size();
                const std::string name = v->name().contents;
                emit(
                    new MoveOperation(
                        RBPOffsetLocation(-currentFrame().nextOffset()),
                        result,
                        OPSIZE_FROM_NUM(size),
                        "var " + name + ""
                    ),
                    SectionType::text
                );
                currentFrame().addData(RBPOffsetLocation(currentFrame().nextOffset()), size, name);
                break;
            }
            case Initializer::copy: {
                const auto init {static_cast<const CopyInitializer*>(v->initializer())};
                const Location result = emitExpression(init->expr());
                const size_t size = v->type()->size();
                const std::string name = v->name().contents;
                emit(
                    new MoveOperation(
                        RBPOffsetLocation(-currentFrame().nextOffset()),
                        result,
                        OPSIZE_FROM_NUM(size),
                        "var " + name + ""
                    ),
                    SectionType::text
                );
                currentFrame().addData(RBPOffsetLocation(currentFrame().nextOffset()), size, name);
                break;
            }
        }
    }
    
    // MARK: Emit local constant
    void Compiler::emitLocalConst(LetDeclaration *l) {
        switch (l->initializer()->type) {
            case Initializer::zero: {
                const size_t size = l->type()->size();
                const std::string name = l->name().contents;
                
                emit(
                    new MoveOperation(
                        RBPOffsetLocation(-currentFrame().nextOffset()),
                        NumLL(false, SU(0ULL)),
                        OPSIZE_FROM_NUM(size),
                        "let " + name + " = 0"
                    ),
                    SectionType::text
                );
                currentFrame().addData(RBPOffsetLocation(currentFrame().nextOffset()), size, name);

                break;
            }
            case Initializer::direct: {
                const auto init {static_cast<const DirectInitializer*>(l->initializer())};
                const Location result = emitExpression(init->expr());
                const size_t size = l->type()->size();
                const std::string name = l->name().contents;
                emit(
                    new MoveOperation(
                        RBPOffsetLocation(-currentFrame().nextOffset()),
                        result,
                        OPSIZE_FROM_NUM(size),
                        "let " + name + ""
                    ),
                    SectionType::text
                );
                currentFrame().addData(RBPOffsetLocation(currentFrame().nextOffset()), size, name);
                break;
            }
            case Initializer::copy: {
                const auto init {static_cast<const CopyInitializer*>(l->initializer())};
                const Location result = emitExpression(init->expr());
                const size_t size = l->type()->size();
                const std::string name = l->name().contents;
                emit(
                    new MoveOperation(
                        RBPOffsetLocation(-currentFrame().nextOffset()),
                        result,
                        OPSIZE_FROM_NUM(size),
                        "let " + name + ""
                    ),
                    SectionType::text
                );
                currentFrame().addData(RBPOffsetLocation(currentFrame().nextOffset()), size, name);
                break;
            }
        }
    }

    // MARK: Emit function
    void Compiler::emitFunction(Function *func) {
        const FunctionSignature funsig {func->name().contents, func->parameters()};
        const auto flbl {analyzer.strFromFunctionSignature(funsig)};
        /* if (!func->isStatic()) */
        emit(new Label(flbl, true), SectionType::text); // label this code
        emitEnter(); // create new frame
        loadParametersIntoFrame(func->parameters());
        
        // [rbp-N] = first function parameter, where N is the
        //           sum of the sizes of the function parameter
        //           aligned to 16 bytes
        // [rbp] = old stack frame
        // [rbp-8] = first local variable
        const long long staticAllocationSize { (long long)func->staticAllocationSize };

        if (staticAllocationSize) emit(new SubOperation(RegisterLocation(Register::rsp), NumLL(true, SU(staticAllocationSize))), SectionType::text);
        for (auto node: func->body()) {
            if (auto stm = dynamic_cast<Statement*>(node)) emitStatement(stm);
            if (auto decl = dynamic_cast<Declaration*>(node)) emitDeclaration(decl);
        }
        
        // Integer return values up to 64 bits in size are stored in RAX while values up to 128 bit are stored in RAX and RDX
    }

    void Compiler::emitExternFunc(FunctionForwardDeclaration* ffunc) {
        const FunctionSignature funsig {ffunc->name().contents, ffunc->parameters()};
        emit(new Extern(analyzer.strFromFunctionSignature(funsig)), SectionType::text);
    }

    // MARK: Various subroutine components
    void Compiler::emitEnter() {
        enterFrame(); // create a new Frame and push_back it to the std::vector frames
        emit(new PushOperation(RegisterLocation(Register::rbp), "store old frame"), SectionType::text); // save old stack frame pointer
        emit(new MoveOperation(RegisterLocation(Register::rbp), RegisterLocation(Register::rsp), SizeType::qword, "create new frame"), SectionType::text); // create new stack frame pointer
        currentFrame().addData(RBPOffsetLocation(0), 8, "old_rbp"); // the old rbp is pushed and thus stored at offset 0 in this frame
    }
        void Compiler::emitLeave() {
            leaveFrame(); // pop_back the current Frame from std::vector frames
            //emit(new LeaveOperation("restore old frame"), SectionType::text); // leave current stack frame
            emit(new MoveOperation(RegisterLocation(Register::rsp), RegisterLocation(Register::rbp), SizeType::qword), SectionType::text);
            emit(new PopOperation(RegisterLocation(Register::rbp)), SectionType::text);
        }
        void Compiler::emitRet() {
            emit(new ReturnOperation("return from function"), SectionType::text); // pop return address from stack and jump to it
        }
    void Compiler::loadParametersIntoFrame(const Function::Parameters& params) {
            const size_t size = params.size();
                if (size == 0) return;
                
                int integers{};
        //        int floats{};
                
                static const Register integerRegs[] {
                    Register::rdi, Register::rsi, Register::rdx, Register::rcx, Register::r8, Register::r9
                };
        //        static const Register floatRegs[] {
        //
        //        };
                        
                std::vector<Function::Parameter> stackArgs;
                stackArgs.reserve(max(6, size) - 6);
                        
                for (size_t index = 0; index < size; index++) {
                    Function::Parameter param {params[index]};
                    if ((true) || param.type->isInteger()) {
                        if (integers >= 6) { // only 6 registers for arguments
                            stackArgs.push_back(param);
                        } else {
                            currentFrame().addData(RegisterLocation(integerRegs[integers++]), param.type->size(), param.name.contents);
                        }
                    }
        //            else {
        //                floats++;
        //            }
                }
    }

        // MARK: Variable lookup


        std::pair<Variable, bool> Compiler::lookup(const std::string& name) {
            for (std::vector<Frame>::const_reverse_iterator i = frames.rbegin(); i != frames.rend(); ++i) {
                const auto result = (*i).localLookup(name);
                if (result.second) return result;
            }
            return { {0, 0}, false };
        }

        // MARK: Emit general expression (COMPLEX)
        Location Compiler::emitExpression(Expression* expr) {
            if (auto binary = dynamic_cast<BinaryExpression*>(expr)) {
                // assume true binary expression for now
                Expression* left {binary->left()};
                const OperatorComponentExpression* op {binary->op()};
                Expression* right {binary->right()};
                
                switch (op->tkntype()) {
                    case TokenType::plus: {
                        return emitBinaryExpr(left, right, OpType::add);
                    }
                    case TokenType::minus: {
                        return emitBinaryExpr(left, right, OpType::sub);
                    }
                    case TokenType::multiply: {
                        return emitBinaryExpr(left, right, OpType::imul);
                    }
                    case TokenType::andOp: {
                        if (left && right) emitBinaryExpr(left, right, OpType::and_);
                        else if (!left && right) {
                            if (auto symbol = dynamic_cast<SymbolExpression*>(right)) {
                                const auto result = lookup(symbol->value().contents);
                                if (result.second && IS_RBPOFFSET(result.first.loc)) {
                                    const auto reg = RegisterLocation(static_cast<Register>(currentFrame().avaliableScratch()));
                                    emit(new LoadAddressOperation(reg, RBPOffsetLocation(-result.first.loc.stack), SizeType::qword, "address of"), SectionType::text);
                                    return reg;
                                }
                            }
                        }
                    }
                    case TokenType::divide: {
                        // idiv uses rax/rdx
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
    //                                       "  mov rdx, 0\n" // set to one of rax < 0
    //                                       "  mov rax, " + lhs + "\n"
    //                                       "  mov " + rhsr + ", " + rhs + "\n"
    //                                       "  idiv " + rhsr + "; quotient\n"
    //                                       "  mov " + resultr + ", rax\n"
    //                                       );
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
                    }
                    default: {
                        // Unsupported operation
                        assert(false && "Unsupported operation");
                        break;
                    }
                }
            }
            else if (auto literal = dynamic_cast<Literal*>(expr)) {
                switch (literal->type()) {
                    case Literal::LType::simpleString: {
                        static long strlitCount = 0;
                        const std::string lbl {"str_literal_" + std::to_string(strlitCount++)}; // create the label
                        emit(new StringData(lbl, literal->value().contents), SectionType::rodata); // add the labeled string as bytes in section .rodata
                        
                        const Register resultr = static_cast<Register>(frames.back().avaliableScratch()); // get a new register
                        emit(new LoadAddressOperation(RegisterLocation(resultr), RelLabelL(lbl), SizeType::qword, "string literal"), SectionType::text); // dump the string into this new register
                        return RegisterLocation(resultr);
                    }
                    case Literal::LType::boolean: {
                        return literal->value().type == TokenType::boolTrue ? NumLL(false, SU(1ULL)) : NumLL(false, SU(0ULL)); // 1 == true, 0 == false
                    }
                    case Literal::LType::decimalInteger: {
                        return NumLL(false, SU(atoll(literal->value().contents.c_str()))); // simply return the integer value
                    }
                    case Literal::LType::hexadecimalInteger: {
                        assert(false && "Unsupported currently"); // need to work out radix conversion stuff
                    }
                    case Literal::LType::floatingPointNumber: {
                        // WRONG!!!!!!! should return an xmm register but WILL FIX
                        union {
                            double f;
                            uint64_t b;
                        } floatbits;
                        floatbits.f = atof(literal->value().contents.c_str());
                        return NumLL(false, SU(floatbits.b)); // return the floating point bits
                    }
                }
            }
            else if (auto symbol = dynamic_cast<SymbolExpression*>(expr)) {
                // MARK: literally looks for defined stuff in this frame will fix later
                const auto result = lookup(symbol->value().contents); // PROBLEM: only valid in reference to that frame. Add member to struct variable called TOTAL OFFSET or ABSOLUTE OFFSET which contains the offset FROM THE START and hence make the calculation of the offset from the current rbp possible
                if (!result.second) {
                    assert(false && "Static analyzer should catch this");
                }
                const Register resultr = static_cast<Register>(frames.back().avaliableScratch());
                auto loc = result.first.loc;
                if (loc.stack) loc.stack = -loc.stack;
                emit(new MoveOperation(RegisterLocation(resultr), loc, OPSIZE_FROM_NUM(result.first.size), "store " + result.first.name + " in " + registerNames[static_cast<int>(resultr)]), SectionType::text);
                return RegisterLocation(resultr);
            }
            else if (auto call = dynamic_cast<Call*>(expr)) {
                emitCall(call);
                return RETURN_VALUE_LOC;
            }
            
            // Should not reach here
            assert(false && "Should not reach here");
        }

        // MARK: Mark registers as avaliable
        void Compiler::returnRegister(const Register r) {
            const int reg = static_cast<int>(r);
            for (int i = 0; i < static_cast<int>(Register::r15); i++) {
                if (i == reg) {
                    currentFrame().returnScratchRegister(static_cast<Register>(i));
                }
            }
        }

        // MARK: Emit binary expression
        Location Compiler::emitBinaryExpr(Expression *left, Expression *right, const OpType op) {
            const Location lhs = emitExpression(left); // evaluate the left-hand side
            const Location rhs = emitExpression(right); // evaluate the right-hand side
            
            // avaliableScratch can return -1 if no regs avaliable, but ignore for now
            // get a result and rhs temporary register for calculation
            const Register resultr = static_cast<Register>(frames.back().avaliableScratch());
            const Register rhsr = static_cast<Register>(frames.back().avaliableScratch());
            
            // move the expression results into the new temp registers
            emit(new MoveOperation(RegisterLocation(resultr), lhs, SizeType::qword), SectionType::text);
            emit(new MoveOperation(RegisterLocation(rhsr), rhs, SizeType::qword), SectionType::text);
            
            // Perform the operation
            switch (op) {
                case OpType::add: {
                    emit(new AddOperation(RegisterLocation(resultr), RegisterLocation(rhsr)), SectionType::text);
                    break;
                }
                case OpType::sub: {
                    emit(new SubOperation(RegisterLocation(resultr), RegisterLocation(rhsr)), SectionType::text);
                    break;
                }
                case OpType::imul: {
                    emit(new MulOperation(RegisterLocation(resultr), RegisterLocation(rhsr)), SectionType::text);
                    break;
                }
                case OpType::and_: {
                    emit(new AndOperation(RegisterLocation(resultr), RegisterLocation(rhsr)), SectionType::text);
                    break;
                }
                default: {
                    assert(false && "Should never reach here");
                    break;
                }
            }
            
            // Return all unused registers
            if (lhs.reg != LOC_IS_NOT_REG) returnRegister(static_cast<Register>(lhs.reg));
            if (rhs.reg != LOC_IS_NOT_REG) returnRegister(static_cast<Register>(rhs.reg));
            returnRegister(rhsr);
            
            return RegisterLocation(resultr);
        }

        // MARK: Caller save registers
        void Compiler::emitSaveRegisters(const std::vector<Register> &registers) {
            for (std::vector<Register>::const_reverse_iterator i = registers.rbegin(); i != registers.rend(); ++i) {
                emit(new PushOperation(RegisterLocation(*i)), SectionType::text);
            }
        }
        void Compiler::emitRestoreRegisters(const std::vector<Register> &registers) {
            for (auto reg: registers) {
                emit(new PopOperation(RegisterLocation(reg)), SectionType::text);
            }
        }

        // MARK: Load arguments pre-call
        long long Compiler::emitCallArguments(const std::vector<Expression *> &args) {
            // https://en.wikipedia.org/wiki/X86_calling_conventions#System_V_AMD64_ABI -
            // The first six integer or pointer arguments are passed in registers RDI, RSI, RDX, RCX, R8, R9
            // while XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6 and XMM7 are used for the first floating point arguments. As in the Microsoft x64 calling convention, additional arguments are passed on the stack.
            const size_t size = args.size();
            if (size == 0) return 0;
            
            int integers{};
    //        int floats{};
            
            static const Register integerRegs[] {
                Register::rdi, Register::rsi, Register::rdx, Register::rcx, Register::r8, Register::r9
            };
    //        static const Register floatRegs[] {
    //
    //        };
                    
            std::vector<Expression*> stackArgs;
            stackArgs.reserve(max(6, size) - 6);
                    
            for (size_t index = 0; index < size; index++) {
                Expression* arg {args[index]};
                if ((true) || arg->type->isInteger()) {
                    if (integers >= 6) { // only 6 registers for arguments
                        stackArgs.push_back(arg);
                    } else {
                        const Register argreg = integerRegs[integers++];
                        const Location result = emitExpression(arg);
                        if (result.reg != static_cast<int>(argreg)) emit(new MoveOperation(RegisterLocation(argreg), result, SizeType::qword, "argument " + std::to_string(index)), SectionType::text);
                        if (result.reg != LOC_IS_NOT_REG) returnRegister(static_cast<Register>(result.reg));
                    }
                }
    //            else {
    //                floats++;
    //            }
            }
                    
            const long long stackArgC = stackArgs.size();
            while (!stackArgs.empty()) {
                Expression* arg = stackArgs.back();
                stackArgs.pop_back();
                        
                const Location result = emitExpression(arg);
                emit(new PushOperation(result, "push argument onto stack"), SectionType::text);
            }
            return stackArgC;
        }

        // MARK: Emit call
        void Compiler::emitCall(Call *call) {
            const FunctionSignature funsig {call->name.contents, call->_spa_params};

            const std::vector<Register> registersInUse {currentFrame().registersInUse};
            emitSaveRegisters(registersInUse); // pop all registers in use onto the stack, as the callee may modify them
            
            const long long stackArgC = emitCallArguments(call->args); // load the arguments
            emit(new CallOperation(analyzer.strFromFunctionSignature(funsig), "call function"), SectionType::text); // call the function
            if (stackArgC) emit(new AddOperation(RegisterLocation(Register::rsp), NumLL(false, SU(stackArgC * 8)), "remove args"), SectionType::text); // remove arguments
            emitRestoreRegisters(registersInUse); // restore all the saved registers to original state
        }

        // MARK: Frame handling
        void Compiler::enterFrame() {
            frames.push_back({});
        }
        void Compiler::leaveFrame() {
            frames.pop_back();
        }
        Frame& Compiler::currentFrame() {
            return frames.back();
        }

        // MARK: Emit entry point for execution
        void Compiler::generateEntryPoint(Function* main) {
            emit(new RawText(
                "\nglobal _main ; _main is the entry point in macOS nasm\n" // make entry point visible to linker
                "_main:" // _main is the entry point in macOS nasm
            ), SectionType::text);
            emit(new SubOperation(RegisterLocation(Register::rsp), NumLL(false, SU(8LLU)), "so stack is aligned upon call"), SectionType::text); // align stack to 16 bytes
            emit(new CallOperation(main->name().contents, "call the floral main function"), SectionType::text); // call the Floral main function
            emit(new AddOperation(RegisterLocation(Register::rsp), NumLL(false, SU(8LLU)), "restore stack pointer"), SectionType::text); // restore stack pointer
            emit(
                new MoveOperation(RegisterLocation(Register::rdi), RETURN_VALUE_LOC, SizeType::qword, "exit code"),
                SectionType::text
            ); // store return value (rax) as exit code in rdi (first syscall argment)
            emit(
                //new MoveOperation(RegisterLocation(Register::rax), NumLL(false, SU(0x2000001ULL)), SizeType::qword, "0x200001 = exit"),
                new RawText("  mov rax, 0x2000001 ; exit syscall"),
                SectionType::text
            ); // set rax to indicate the exit syscall
            emit(new Syscall(), SectionType::text); // perform syscall
    //        emitRet();
        }

    void Compiler::optimize() {
        // if we have two movs, one "mov RegA, any" and "mov RegB, RegA" then we can combine to "mov RegB, any"
//        const size_t instrc = textSection.instructions.size();
//
//        for (size_t i = 0; i < instrc; i++) {
//            const Instruction* instr {textSection.instructions[i]};
//
//        }
    }

        // MARK: General compiliation
        void Compiler::compile(const File *file) {
            reset(); // in case the function is called more than once
            
            analyzer.analyze(file); // perform static analysis to gain control flow and type information
            if (analyzer.hasErrors()) {
                std::cout << "Cannot compile due to static analysis resulting in error(s)\n";
                for (auto index = 0; index < analyzer.errors().size(); index++) {
                    std::cout << index + 1 << ". ";
                    analyzer.errors()[index].print(_src);
                }
                std::cout << '\n';
                return;
            }
            // Compile
            Function* main = nullptr;
            if (file->main()) {
                main = file->main();
            }
            
            for (auto node: file->nodes()) {
                if (auto decl = dynamic_cast<Declaration*>(node))
                emitDeclaration(decl);
            else if (auto stm = dynamic_cast<Statement*>(node))
                emitStatement(stm);
            else {
                report(
                       Error::compileDomain,
                       "An expression cannot be a top-level declaration",
                       node->_loc,
                       { node->_loc.pos, 0 }
                );
                return;
            }
        }
        
        if (main) {
            emitFunction(main);
            generateEntryPoint(main); // if this is a file with `func main(): Int` then emit the assembly for an entry point
        }
            
            if (optimization) {
                optimize();
            }
        
        Floral::write(outputDest, result());
    }
    const std::string Compiler::result() const {
        std::string joined = textSection.str() + '\n' + bssSection.str() + '\n' + rodataSection.str() + '\n' + dataSection.str(); // join all sections into one string
        while (joined.back() == '\n') joined.pop_back();
        return joined;
    }

    void Compiler::setSource(const std::string& src) {
        _src = src;
    }
}}
