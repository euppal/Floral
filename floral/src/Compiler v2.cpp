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
#include "Colors.hpp"

#define CAT_COMMENTS(opleft, opright) if (!((opleft)->comment.empty() || (opright)->comment.empty())) (opleft)->comment += " && " + (opright)->comment
#define INSRT_COMMENTS(opleft, opright) if (!((opleft)->comment.empty() || (opright)->comment.empty())) (opleft)->comment = (opright)->comment + " && " + (opleft)->comment

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
    void Compiler::warn(const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix) {
        Error err {Error::warning, text, loc, errloc};
        err.fix = fix;
        err.isWarning = true;
        _errors.push_back(err);
    }
    bool Compiler::hasErrors() const {
        return !_errors.empty() || analyzer.hasErrors();
    }
    const std::vector<Error>& Compiler::errors() const {
        return _errors;
    }
    bool Compiler::hasWarnings() const {
        return !_warnings.empty();
    }
    const std::vector<Error>& Compiler::warnings() const {
        return _warnings;
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
        analyzer.reset();
        _errors.clear();
        _warnings.clear();
    }

    std::string Compiler::staticEvalulate(Expression* staticEvalExpr) {
        assert(staticEvalExpr->info.isStaticEval && "Expression was not static eval");
        if (auto literal = dynamic_cast<Literal*>(staticEvalExpr)) {
            auto descr = literal->description();
            if (descr == TYPE_STRING_INDICATOR) {
                descr = literal->value().contents;
                this->_strprocess(descr);
                StringData sd("", descr);
                descr = sd.str();
                descr.erase(descr.begin(), descr.begin() + 16);
                return descr;
            }
            return descr;
        } else if (auto binary = dynamic_cast<BinaryExpression*>(staticEvalExpr)) {
            Expression* left = binary->left();
            Expression* right = binary->right();
            
            switch (binary->op()->tkntype()) {
                case TokenType::plus:
                    return std::to_string(atoll(staticEvalulate(left).c_str()) + atoll(staticEvalulate(right).c_str()));                    
                default:
                    break;
            }
        } else if (auto symbol = dynamic_cast<SymbolExpression*>(staticEvalExpr)) {
            const auto gbl = analyzer.lookupGlobal(symbol->value().contents);
            const auto init = gbl->initializer();
            if (init->type == Initializer::InitializerType::zero) {
                return "0";
            } else if (auto direct = dynamic_cast<DirectInitializer*>(init)) {
                return staticEvalulate(direct->expr());
            } else if (auto copy = dynamic_cast<CopyInitializer*>(init)) {
                return staticEvalulate(copy->expr());
            }
        }
        assert(false && "Should not reach here");
    }

    void Compiler::_processPotentialStackOperation(Instruction* instr) {
        if (auto add = dynamic_cast<AddOperation*>(instr)) {
            if (add->dest.reg == static_cast<int>(Register::rsp) && add->src.isLiteral) {
                _stackFromMain -= add->src.value.s;
            }
        } else if (auto sub = dynamic_cast<SubOperation*>(instr)) {
            if (sub->dest.reg == static_cast<int>(Register::rsp) && sub->src.isLiteral) {
                _stackFromMain += sub->src.value.s;
            }
        } else if (auto push = dynamic_cast<PushOperation*>(instr)) {
            _stackFromMain += 8;
        } else if (auto pop = dynamic_cast<PopOperation*>(instr)) {
            _stackFromMain -= 8;
        } else if (auto call = dynamic_cast<CallOperation*>(instr)) {
            _stackFromMain += 8;
        } else if (auto ret = dynamic_cast<ReturnOperation*>(instr)) {
            _stackFromMain -= 8;
        }
    }

    // MARK: General emission
    void Compiler::emit(Instruction* instr, SectionType section) {
        switch (section) {
            case SectionType::text: {
                textSection.add(instr);
                _processPotentialStackOperation(instr);
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
        if (auto letStm = dynamic_cast<LetStatement*>(stm)) {
            emitLocalConst(letStm);
        } else if (auto varStm = dynamic_cast<VarStatement*>(stm)) {
            emitLocalVar(varStm);
        } if (auto callStm = dynamic_cast<CallStatement*>(stm)) {
            emitCallStatement(callStm); // perform call with necessary setup/teardown
        } else if (auto rtnStm = dynamic_cast<ReturnStatement*>(stm)) {
            emitReturnStatement(rtnStm); // get a specific value into RETURN_VALUE_LOC (Register::rax)
        } else if (auto exprStm = dynamic_cast<ExpressionStatement*>(stm)) {
            emitExpressionStatement(exprStm); // simply execute the expression
        } else if (auto ptrAssignStm = dynamic_cast<PointerAssignment*>(stm)) {
            emitPointerAssignmentStatement(ptrAssignStm); // put value into pointer
        } else if (auto assignStm = dynamic_cast<Assignment*>(stm)) {
            emitAssignmentStatement(assignStm); // assign new value
        } else if (auto ifStm = dynamic_cast<IfStatement*>(stm)) {
            emitIfStatement(ifStm); // conditional branching
        } else if (auto whileStm = dynamic_cast<WhileStatement*>(stm)) {
            emitWhileStatement(whileStm); // conditional looping
        } else if (auto forStm = dynamic_cast<ForStatement*>(stm)) {
            emitForStatement(forStm); // conditional looping but nicer
        } else if (auto block = dynamic_cast<Block*>(stm)) {
            emitBlock(block); // block of code
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

    // MARK: Expression statement
    void Compiler::emitExpressionStatement(ExpressionStatement* exprStm) {
        const Location loc = emitExpression(exprStm->expr()); // execute expression
        if (IS_REG(loc)) {
            returnRegister(static_cast<Register>(loc.reg)); // discard result
        }
    }

    // MARK: Assignment statement
    void Compiler::emitAssignmentStatement(Assignment* assignStm) {
        if (auto layer1 = dynamic_cast<BinaryExpression*>(assignStm->lval())) {
            if (layer1->op() && layer1->op()->tkntype() == TokenType::multiply && !layer1->left()) {
                PointerAssignment ptrAssign { layer1->right()->_loc, layer1->right(), assignStm->rval() };
                emitPointerAssignmentStatement(&ptrAssign);
                ptrAssign.makenull();
                return;
            } else if (layer1->op() && layer1->op()->tkntype() == TokenType::leftBracket) {
                const Token addOp { {layer1->op()->_loc.pos, layer1->op()->_loc.startLine}, TokenType::plus, "+" };
                Expression* pointer = new BinaryExpression(layer1->_loc, layer1->left(), new OperatorComponentExpression(addOp), layer1->right());
                PointerAssignment ptrAssign { assignStm->_loc, pointer, assignStm->rval() };
                emitPointerAssignmentStatement(&ptrAssign);
                ptrAssign.makenull();
                return;
            }
        }
        const Token referenceOp { {assignStm->lval()->_loc.pos, assignStm->lval()->_loc.startLine}, TokenType::andOp, "&" };
        Expression* pointer = new BinaryExpression(assignStm->lval()->_loc, nullptr, new OperatorComponentExpression(referenceOp), assignStm->lval());
        PointerAssignment ptrAssign { assignStm->_loc, pointer, assignStm->rval() };
        emitPointerAssignmentStatement(&ptrAssign);
        ptrAssign.makenull();
    }

    // MARK: Pointer assignment statement
    void Compiler::emitPointerAssignmentStatement(PointerAssignment* ptrAssign) {
        const Location ptr = emitExpression(ptrAssign->ptrExpr()); // dump the pointer into a register
        const Location newval = emitExpression(ptrAssign->newValue()); // dump the new value into a register
        
        if (IS_REG(ptr)) {
            const Register destreg = static_cast<Register>(ptr.reg);
            const Location dest = _wasRegisterParameter ? RegisterLocation(destreg) : ValueAtRegisterLocation(destreg);
            _wasRegisterParameter = false;
            
            emit(new MoveOperation(dest, newval, SizeType::qword, "assignment"), SectionType::text); // move src into [dest]
            
            returnRegister(destreg);
            if (IS_REG(newval)) {
                const Register srcreg = static_cast<Register>(newval.reg);
                returnRegister(srcreg);
            }
        }
    }

    // MARK: If statement
    void Compiler::emitIfStatement(IfStatement* ifStm) {
        static int counter {};
        
        const std::string name =  currentFrame().id + "_#if_skip_" + std::to_string(counter++);
        const Location loc = emitExpression(ifStm->condition()); // calculate condition
        
        if (loc.isLiteral) {
            if (loc.value.u) {
                emitBlock(ifStm->body()); // unconditionally emit block
            } else {
                return;
            }
        } else {
            emit(new CmpOperation(loc, NumLL(false, SU(0ULL))), SectionType::text); // subtract from itself
            emit(new JumpOperation(JumpOperation::JType::zero, name), SectionType::text); // nonzero difference = false
            if (IS_REG(loc)) {
                returnRegister(static_cast<Register>(loc.reg));
            }
            emitBlock(ifStm->body()); // skipped if condition is false
            emit(new Label(name, false), SectionType::text); // label to skip to
        }
    }

    // MARK: While statement
    void Compiler::emitWhileStatement(WhileStatement* whileStm) {
        static int counter {};
        static int counter2 {};
        
        const std::string name =  currentFrame().id + "_#while_loop_" + std::to_string(counter++);
        const std::string skipName =  currentFrame().id + "_#while_skip_" + std::to_string(counter2++);
        emit(new Label(name, false), SectionType::text); // label to loop to
        const Location loc = emitExpression(whileStm->condition()); // calculate condition
        emit(new JumpOperation(JumpOperation::JType::zero, skipName), SectionType::text);
        emitBlock(whileStm->body());

        if (loc.isLiteral) {
            if (loc.value.u) {
                emit(new JumpOperation(JumpOperation::JType::normal, name), SectionType::text); // unconditionally loop block
            } else {
                return;
            }
        } else {
            emit(new CmpOperation(loc, NumLL(false, SU(0ULL))), SectionType::text); // subtract from itself
            emit(new JumpOperation(JumpOperation::JType::nonzero, name), SectionType::text); // nonzero difference = false
            emit(new Label(skipName, false), SectionType::text); // label to loop to
            if (IS_REG(loc)) {
                returnRegister(static_cast<Register>(loc.reg));
            }
        }
    }

    // MARK: Emit for statement
    void Compiler::emitForStatement(ForStatement *forStm) {
        
        emitStatement(forStm->init());
        
//        static int counter {};
//
//        const std::string name =  currentFrame().id + "_#for_loop_" + std::to_string(counter++);
//        emit(new Label(name, false), SectionType::text); // label to loop to
//
//        const Location loc = emitExpression(forStm->check()); // calculate condition
//        emitBlock(forStm->body());
//
//        if (loc.isLiteral) {
//            if (loc.value.u) {
//                emitStatement(forStm->modify());
//                emit(new JumpOperation(JumpOperation::JType::normal, name), SectionType::text); // unconditionally loop block
//            } else {
//                leaveFrame();
//                return;
//            }
//        } else {
//            emitStatement(forStm->modify());
//            emit(new CmpOperation(loc, NumLL(false, SU(0ULL))), SectionType::text); // subtract from itself
//            emit(new JumpOperation(JumpOperation::JType::nonzero, name), SectionType::text); // nonzero difference = false
//            if (IS_REG(loc)) {
//                returnRegister(static_cast<Register>(loc.reg));
//            }
//        }
        
        forStm->body()->insert(forStm->modify());
        emitWhileStatement(new WhileStatement(forStm->_loc, forStm->check(), forStm->body()));
        
    }

    // MARK: Block statement
    void Compiler::emitBlock(Block* block) {
        // MARK: BAD BUT WILL DO FOR NOW
        for (auto node: block->body()) {
            if (auto stm = dynamic_cast<Statement*>(node)) {
                emitStatement(stm);
            } else if (auto decl = dynamic_cast<Declaration*>(node)) {
                emitDeclaration(decl);
            }
        }
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
        }
    }

    // MARK: Emit global constant
    void Compiler::emitGlobal(GlobalDeclaration *gbl) {
        Initializer* init = gbl->initializer();
        if (init->type == Initializer::zero) {
            emit(new ZeroData(gbl->name.contents, OPSIZE_FROM_NUM(gbl->type->alignment())), SectionType::bss);
        } else if (auto direct = dynamic_cast<const DirectInitializer*>(init)) {
            emit(new RawText(INDENT + prefixed(gbl->name.contents) + ": " + (direct->expr()->type->isCString() ? "db" : "dq ") + staticEvalulate(direct->expr())), SectionType::rodata);
        } else if (auto copy = dynamic_cast<const CopyInitializer*>(init)) {
            emit(new RawText(INDENT + prefixed(gbl->name.contents) + ": " + (copy->expr()->type->isCString() ? "db" : "dq ") + staticEvalulate(copy->expr())), SectionType::rodata);
        }
    }

    // MARK: Emit forward-declared global constant
    void Compiler::emitExternGlobal(GlobalForwardDeclaration *fgbl) {
        emit(new Extern(fgbl->name().contents), SectionType::text);
    }

    // MARK: Emit local variable
    void Compiler::emitLocalVar(VarStatement *v) {
        if (!v->initializer()) {
            const std::string name = v->name().contents;
            currentFrame().addData(RBPOffsetLocation(currentFrame().nextOffset()), v->type()->alignment(), name);
            return;
        }
        switch (v->initializer()->type) {
            case Initializer::zero: {
                const size_t size = v->type()->size();
                const std::string name = v->name().contents;
                
                emit(
                    new MoveOperation(
                        RBPOffsetLocation(currentFrame().nextOffset()),
                        NumLL(false, SU(0ULL)),
                        OPSIZE_FROM_NUM(v->type()->alignment()),
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
                        RBPOffsetLocation(currentFrame().nextOffset()),
                        result,
                        SizeType::qword, //OPSIZE_FROM_NUM(size),
                        "var " + name + ""
                    ),
                    SectionType::text
                );
                if (IS_REG(result)) {
                    returnRegister(static_cast<Register>(result.reg));
                }
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
                        RBPOffsetLocation(currentFrame().nextOffset()),
                        result,
                        SizeType::qword, //OPSIZE_FROM_NUM(size),
                        "var " + name + ""
                    ),
                    SectionType::text
                );
                if (IS_REG(result)) {
                    returnRegister(static_cast<Register>(result.reg));
                }
                currentFrame().addData(RBPOffsetLocation(currentFrame().nextOffset()), size, name);
                break;
            }
        }
    }
    
    // MARK: Emit local constant
    void Compiler::emitLocalConst(LetStatement *l) {
        switch (l->initializer()->type) {
            case Initializer::zero: {
                const size_t size = l->type()->size();
                const std::string name = l->name().contents;
                
                emit(
                    new MoveOperation(
                        RBPOffsetLocation(currentFrame().nextOffset()),
                        NumLL(false, SU(0ULL)),
                        SizeType::qword, //OPSIZE_FROM_NUM(size),
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
                        RBPOffsetLocation(currentFrame().nextOffset()),
                        result,
                        SizeType::qword, //OPSIZE_FROM_NUM(size),
                        "let " + name + ""
                    ),
                    SectionType::text
                );
                if (IS_REG(result)) {
                    returnRegister(static_cast<Register>(result.reg));
                }
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
                        RBPOffsetLocation(currentFrame().nextOffset()),
                        result,
                        SizeType::qword, //OPSIZE_FROM_NUM(size),
                        "let " + name + ""
                    ),
                    SectionType::text
                );
                if (IS_REG(result)) {
                    returnRegister(static_cast<Register>(result.reg));
                }
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
        emitEnter(); currentFrame().id = flbl; // create new frame
        
        // [rbp-N] = first function parameter, where N is the
        //           sum of the sizes of the function parameter
        //           aligned to 16 bytes
        // [rbp] = old stack frame
        // [rbp-8] = first local variable
        auto sz = (long long)(func->staticAllocationSize + func->parameters().size() * 8);
        const long long staticAllocationSize { (sz + 15) & (-16) };

        if (staticAllocationSize) emit(new SubOperation(RegisterLocation(Register::rsp), NumLL(true, SU(staticAllocationSize)), "allocate space on the stack for local variables"), SectionType::text);
        loadParametersIntoFrame(func->parameters());
        for (auto node: func->body()) {
            if (auto stm = dynamic_cast<Statement*>(node)) emitStatement(stm);
            if (auto decl = dynamic_cast<Declaration*>(node)) emitDeclaration(decl);
        }
        
        // Integer return values up to 64 bits in size are stored in RAX while values up to 128 bit are stored in RAX and RDX
        
        leaveFrame(); // pop_back the current Frame from std::vector frames
    }

    void Compiler::emitExternFunc(FunctionForwardDeclaration* ffunc) {
        const FunctionSignature funsig {ffunc->name().contents, ffunc->parameters()};
        emit(new Extern(analyzer.strFromFunctionSignature(funsig)), SectionType::text);
    }

    // MARK: Various subroutine components
    void Compiler::emitEnter() {
        enterFrame(); // create a new Frame and push_back it to the std::vector frames
        emit(new PushOperation(RegisterLocation(Register::rbp), "store old frame"), SectionType::text); // save old stack frame pointer
        emit(new MoveOperation(RegisterLocation(Register::rbp), RegisterLocation(Register::rsp), SizeType::qword, "push new frame"), SectionType::text); // create new stack frame pointer
        currentFrame().addData(RBPOffsetLocation(0), 8, "old_rbp"); // the old rbp is pushed and thus stored at offset 0 in this frame
    }
        void Compiler::emitLeave() {
            //emit(new LeaveOperation("restore old frame"), SectionType::text); // leave current stack frame
            emit(new MoveOperation(RegisterLocation(Register::rsp), RegisterLocation(Register::rbp), SizeType::qword, "pop this frame"), SectionType::text);
            emit(new PopOperation(RegisterLocation(Register::rbp), "restore old frame"), SectionType::text);
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
                            const Location dest = RBPOffsetLocation(currentFrame().nextOffset());
                            emit(new MoveOperation(dest, RegisterLocation(integerRegs[integers++]), SizeType::qword, "@ load register parameter to local var"), SectionType::text);
                            currentFrame().addData(dest, param.type->alignment(), param.name.contents);
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
                        if (left->type->isPointer()) {
                            const int r = currentFrame().avaliableScratch();
                            const Location leftloc = emitExpression(left);
                            const Location rightloc = emitExpression(right);
                            emit(new RawText(INDENT "lea " + registerNames[r] + ", [" + leftloc.str() + '+' + rightloc.str() + '*' + std::to_string(left->type->_ptrType->size()) + "] ; pointer arithmetic (" + left->type->des() + " + " + right->prettystr() + ")"), SectionType::text);
                            if (IS_REG(leftloc)) returnRegister(static_cast<const Register>(leftloc.reg));
                            if (IS_REG(rightloc)) returnRegister(static_cast<const Register>(rightloc.reg));
                            return RegisterLocation(static_cast<Register>(r));
                        } else {
                            return emitBinaryExpr(left, right, OpType::add);
                        }
                    }
                    case TokenType::minus: {
                        return emitBinaryExpr(left, right, OpType::sub);
                    }
                    case TokenType::multiply: {
                        if (left && right) return emitBinaryExpr(left, right, OpType::imul);
                        else if (!left && right) {
                            const Location loc = emitExpression(right);
                            if (IS_REG(loc)) {
                                const Register reg = static_cast<Register>(loc.reg);
                                emit(new MoveOperation(RegisterLocation(reg), ValueAtRegisterLocation(reg), SizeType::qword, "dereference"), SectionType::text);
                                return RegisterLocation(reg);
                            }
                        }
                    }
                    case TokenType::andOp: {
                        if (left && right) emitBinaryExpr(left, right, OpType::and_);
                        else if (!left && right) {
                            if (auto symbol = dynamic_cast<SymbolExpression*>(right)) {
                                const auto result = lookup(symbol->value().contents);
                                if (result.second && IS_RBPOFFSET(result.first.loc)) {
                                    const auto reg = RegisterLocation(static_cast<Register>(currentFrame().avaliableScratch()));
                                    emit(new LoadAddressOperation(reg, RBPOffsetLocation(result.first.loc.offset), SizeType::qword, "address of"), SectionType::text);
                                    return reg;
                                } else {
                                    _wasRegisterParameter = true;
                                    return result.first.loc;
                                }
                            } else {
                                Expression* r = right;
                                if (auto binary = dynamic_cast<BinaryExpression*>(right)) {
                                    if (binary->op()->tkntype() == TokenType::multiply && !binary->left() && binary->right()) {
                                        r = binary->right();
                                    }
                                }
                                const Location loc = emitExpression(r);
                                if (IS_REG(loc)) {
                                    const Register reg = static_cast<Register>(loc.reg);
                                    emit(new LoadAddressOperation(RegisterLocation(reg), ValueAtRegisterLocation(reg), SizeType::qword, "address of"), SectionType::text);
                                    return RegisterLocation(reg);
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
                    case TokenType::leftBracket: {
                        const Location pointer = emitExpression(left);
                        const Location index = emitExpression(right);
                        
                        if (index.isLiteral && IS_REG(pointer)) {
                            const auto size = left->type->_ptrType->size();
                            const auto opsize = OPSIZE_FROM_NUM(size);
                            const Register pointerReg = static_cast<Register>(pointer.reg);
                            const int resultint = currentFrame().avaliableScratch();
                            const Register resultreg = static_cast<Register>(resultint);
                            emit(new XorOperation(RegisterLocation(resultreg), RegisterLocation(resultreg), "zero out result"), SectionType::text);
                            emit(new MoveOperation(RegisterLocation(static_cast<Register>(resultint + REG_IMPL_OFFSET_FOR_SIZE(opsize))), ValueAtOffsetRegisterLocation(pointerReg, index.value.s * size), opsize, "subscript into result"), SectionType::text);
                            returnRegister(pointerReg);
                            return RegisterLocation(resultreg);
                        }
                    }
                    case TokenType::equal: {
                        const Location resultloc = emitExpression(left);
                        const Location rhsloc = emitExpression(right);
                        
                        if (resultloc.isLiteral && rhsloc.isLiteral) {
                            const Register r = static_cast<Register>(currentFrame().avaliableScratch());
                            if (resultloc.value.u == rhsloc.value.u) {
                                emit(new MoveOperation(RegisterLocation(r), NumLL(false, SU(1ULL)), SizeType::qword), SectionType::text);
                            } else {
                                emit(new XorOperation(RegisterLocation(r), RegisterLocation(r)), SectionType::text);
                            }
                            return RegisterLocation(r);
                        }
                        
                        emit(new CmpOperation(resultloc, rhsloc), SectionType::text);
                        emit(new MoveOperation(resultloc, NumLL(false, SU(0ULL)), SizeType::qword, "assume unequal by default"), SectionType::text);
                        emit(new RawText(INDENT "sete " + RegisterLocation(static_cast<Register>(static_cast<int>(resultloc.reg) + (static_cast<int>(Register::al) - static_cast<int>(Register::rax)))).str()), SectionType::text);

                        if (IS_REG(rhsloc)) {
                            returnRegister(static_cast<Register>(rhsloc.reg));
                        }
                        
                        return resultloc;
                    }
                    case TokenType::unequal: {
                        const Location resultloc = emitExpression(left);
                        const Location rhsloc = emitExpression(right);
                        
                        if (resultloc.isLiteral && rhsloc.isLiteral) {
                            const Register r = static_cast<Register>(currentFrame().avaliableScratch());
                            if (resultloc.value.u != rhsloc.value.u) {
                                emit(new MoveOperation(RegisterLocation(r), NumLL(false, SU(1ULL)), SizeType::qword), SectionType::text);
                            } else {
                                emit(new XorOperation(RegisterLocation(r), RegisterLocation(r)), SectionType::text);
                            }
                            return RegisterLocation(r);
                        }
                        
                        emit(new SubOperation(resultloc, rhsloc, resultloc.str() + " becomes true if unequal"), SectionType::text);
                        
                        if (IS_REG(rhsloc)) {
                            returnRegister(static_cast<Register>(rhsloc.reg));
                        }
                        
                        return resultloc;
                    }
                    case TokenType::less: {
                        const Location lhsloc = emitExpression(left);
                        const Location rhsloc = emitExpression(right);
                        const Location resultloc = RegisterLocation(static_cast<Register>(currentFrame().avaliableScratch()));
                        
                        if (lhsloc.isLiteral && rhsloc.isLiteral) {
                            const Register r = static_cast<Register>(currentFrame().avaliableScratch());
                            if (lhsloc.isSigned) {
                                if (lhsloc.value.s < rhsloc.value.s) {
                                    emit(new MoveOperation(RegisterLocation(r), NumLL(false, SU(1ULL)), SizeType::qword), SectionType::text);
                                } else {
                                    emit(new XorOperation(RegisterLocation(r), RegisterLocation(r)), SectionType::text);
                                }
                            } else {
                                if (lhsloc.value.u < rhsloc.value.u) {
                                    emit(new MoveOperation(RegisterLocation(r), NumLL(false, SU(1ULL)), SizeType::qword), SectionType::text);
                                } else {
                                    emit(new XorOperation(RegisterLocation(r), RegisterLocation(r)), SectionType::text);
                                }
                            }
                            return RegisterLocation(r);
                        }
                        
                        emit(new XorOperation(resultloc, resultloc), SectionType::text);
                        emit(new CmpOperation(lhsloc, rhsloc), SectionType::text);
                        
                        Register reg = static_cast<Register>(currentFrame().avaliableScratch());
                        
                        emit(new MoveOperation(RegisterLocation(reg), NumLL(false, SU(1ULL)), SizeType::qword), SectionType::text);
                        emit(new RawText(INDENT "cmovb " + resultloc.str() + ", " + RegisterLocation(reg).str()), SectionType::text);
                        
                        returnRegister(reg);
                        
                        if (IS_REG(rhsloc)) {
                            returnRegister(static_cast<Register>(rhsloc.reg));
                        }
                        if (IS_REG(lhsloc)) {
                            returnRegister(static_cast<Register>(lhsloc.reg));
                        }
                        
                        return resultloc;
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
                    case Literal::LType::cString: {
                        static long strlitCount = 0;
                        const std::string lbl {"#str_literal_" + std::to_string(strlitCount++)}; // create the label
                        std::string stringLiteral {literal->value().contents};
                        _strprocess(stringLiteral);
                        emit(new StringData(lbl, stringLiteral), SectionType::rodata); // add the labeled string as bytes in section .rodata
                        
                        const Register resultr = static_cast<Register>(frames.back().avaliableScratch()); // get a new register
                        emit(new LoadAddressOperation(RegisterLocation(resultr), RelLabelL(lbl), SizeType::qword, "string literal"), SectionType::text); // dump the string into this new register
                        return RegisterLocation(resultr);
                    }
                    case Literal::LType::boolean: {
                        return literal->value().type == TokenType::boolTrue ? NumLL(false, SU(1ULL)) : NumLL(false, SU(0ULL)); // 1 == true, 0 == false
                    }
                    case Literal::LType::decimalInteger:
                    case Literal::LType::decimalByte:
                    case Literal::LType::decimalWideChar:
                    case Literal::LType::decimalShort:
                    case Literal::LType::decimalInt32: {
                        return NumLL(false, SU(atoll(literal->value().contents.c_str()))); // simply return the integer value
                    }
                    case Literal::LType::decimalUInteger:
                    case Literal::LType::decimalUByte:
                    case Literal::LType::decimalWideUChar:
                    case Literal::LType::decimalUShort:
                    case Literal::LType::decimalUInt32: {
                        return NumLL(true, SU((uint64_t)strtoul(literal->value().contents.c_str(), NULL, 10))); // simply return the integer value
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
                emit(new MoveOperation(RegisterLocation(resultr), loc, OPSIZE_FROM_NUM(result.first.size), "store " + result.first.name + " in " + registerNames[static_cast<int>(resultr)]), SectionType::text);
                return RegisterLocation(resultr);
            }
            else if (auto call = dynamic_cast<Call*>(expr)) {
                return emitCall(call);
            }
            else if (auto sizeofexpr = dynamic_cast<SizeOfType*>(expr)) {
                return NumLL(false, SU((unsigned long long)sizeofexpr->size()));
            }
            else if (auto unsafecast = dynamic_cast<UnsafeCast*>(expr)) {
                return emitExpression(unsafecast->expr());
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
                    emit(new AddOperation(RegisterLocation(resultr), RegisterLocation(rhsr), "add"), SectionType::text);
                    break;
                }
                case OpType::sub: {
                    emit(new SubOperation(RegisterLocation(resultr), RegisterLocation(rhsr), "subtract"), SectionType::text);
                    break;
                }
                case OpType::imul: {
                    emit(new MulOperation(RegisterLocation(resultr), RegisterLocation(rhsr), "multiply"), SectionType::text);
                    break;
                }
                case OpType::and_: {
                    emit(new AndOperation(RegisterLocation(resultr), RegisterLocation(rhsr), "bitwise and"), SectionType::text);
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
                currentFrame().returnScratchRegister(*i);
                emit(new PushOperation(RegisterLocation(*i)), SectionType::text);
            }
        }
        void Compiler::emitRestoreRegisters(const std::vector<Register> &registers) {
            for (auto reg: registers) {
                currentFrame().registersInUse.push_back(reg);
                emit(new PopOperation(RegisterLocation(reg)), SectionType::text);
            }
        }

        // MARK: Load arguments pre-call
        std::pair<long long, std::vector<Register>> Compiler::emitCallArguments(const std::vector<Expression *> &args) {
            // https://en.wikipedia.org/wiki/X86_calling_conventions#System_V_AMD64_ABI -
            // The first six integer or pointer arguments are passed in registers RDI, RSI, RDX, RCX, R8, R9
            // while XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6 and XMM7 are used for the first floating point arguments. As in the Microsoft x64 calling convention, additional arguments are passed on the stack.
            const size_t size = args.size();
            if (size == 0) return {0, {}};
            
            int integers{};
    //        int floats{};
            
            static const Register integerRegs[] {
                Register::rdi, Register::rsi, Register::rdx, Register::rcx, Register::r8, Register::r9
            };
    //        static const Register floatRegs[] {
    //
    //        };
                    
            std::vector<Expression*> stackArgs;
            std::vector<Register> overwrittenRegs;
            stackArgs.reserve(max(6, size) - 6);
                    
            for (size_t index = 0; index < size; index++) {
                Expression* arg {args[index]};
                if ((true) || arg->type->isInteger()) {
                    if (integers >= 6) { // only 6 registers for arguments
                        stackArgs.push_back(arg);
                    } else {
                        const Register argreg = integerRegs[integers++];
                        if (std::find_if(currentFrame().data.begin(), currentFrame().data.end(), [argreg](Variable v) -> bool {
                            return v.loc == RegisterLocation(argreg);
                        }) != currentFrame().data.end()) {
                            emit(new PushOperation(RegisterLocation(argreg)), SectionType::text);
                            overwrittenRegs.push_back(argreg);
                        }
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
            return { stackArgC, overwrittenRegs };
        }

        // MARK: Emit call
        Location Compiler::emitCall(Call *call) {
            const FunctionSignature funsig {call->name.contents, call->_spa_params};

            std::vector<Register> registersInUse;
            if (!frames.empty()) registersInUse = currentFrame().registersInUse;
            emitSaveRegisters(registersInUse); // pop all registers in use onto the stack, as the callee may modify them
            
            auto loadresult {emitCallArguments(call->args)}; // load the arguments
            const long long stackArgC = loadresult.first;
                        
            emit(new CallOperation(analyzer.strFromFunctionSignature(funsig), call->generateTypeDescription()), SectionType::text); // call the function
            
            bool savedrax = false;
            for (size_t i = 0; i < registersInUse.size(); i++) {
                if (registersInUse[i] == Register::rax) {
                    savedrax = true;
                }
            }
            if (stackArgC) emit(new AddOperation(RegisterLocation(Register::rsp), NumLL(false, SU(stackArgC * 8)), "remove args"), SectionType::text); // remove arguments
            
            Location r = RETURN_VALUE_LOC;
            
            for (auto reg: loadresult.second) {
                emit(new PopOperation(RegisterLocation(reg)), SectionType::text);
            }
            
            if (savedrax) {
                const Register result = static_cast<Register>(currentFrame().avaliableScratch());
                if (!(RegisterLocation(result) == RETURN_VALUE_LOC)) {
                    emit(new MoveOperation(RegisterLocation(result), RETURN_VALUE_LOC, SizeType::qword, "save return value"), SectionType::text);
                    r = RegisterLocation(result);
                }
            }
            
            emitRestoreRegisters(registersInUse); // restore all the saved registers to original state
            
            return r;
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
                "\nextern _init_floral\n"
                "global _main ; _main is the entry point in macOS nasm\n" // make entry point visible to linker
                "_main:" // _main is the entry point in macOS nasm
            ), SectionType::text);
            
            const std::string nameOfMain = analyzer.strFromFunctionSignature({main->name().contents, main->parameters()});
            if (!(nameOfMain == "main" || nameOfMain == "main_i32_u")) {
                report(Error::resolutionDomain, "Cannot find function main(Int32, &&Char)", main->_loc, { main->_name.pos(), main->_name.contents.size() });
            }
            
            emit(new SubOperation(RegisterLocation(Register::rsp), NumLL(false, SU(8LLU)), "@ so stack is aligned upon calls"), SectionType::text); // align stack to 16 bytes
            emit(new RawText(INDENT "call _init_floral"), SectionType::text);
            emit(new CallOperation(nameOfMain, "@ call the floral main function"), SectionType::text); // call the Floral main function
            emit(new AddOperation(RegisterLocation(Register::rsp), NumLL(false, SU(8LLU)), "@ restore stack pointer"), SectionType::text); // restore stack pointer
            emit(
                new MoveOperation(RegisterLocation(Register::rdi), RETURN_VALUE_LOC, SizeType::qword, "@ exit code"),
                SectionType::text
            ); // store return value (rax) as exit code in rdi (first syscall argment)
            emit(
                //new MoveOperation(RegisterLocation(Register::rax), NumLL(false, SU(0x2000001ULL)), SizeType::qword, "0x200001 = exit"),
                new RawText("  mov eax, 0x2000001 ; @ exit syscall"),
                SectionType::text
            ); // set rax to indicate the exit syscall
            emit(new Syscall(), SectionType::text); // perform syscall
    //        emitRet();
        }


    // MARK: General optimiziation
    void Compiler::optimize(int passes) {
        if (passes > 0) {
            while (
                optimizeMatch2(textSection.instructions.size())
            ) {
                optimizeMatch3(textSection.instructions.size());
            }
            optimizeMatch1(textSection.instructions.size());
        }
        if (passes > 1) {
            //optimizeOutRedundancy(textSection.instructions.size());
        }
    }

    // MARK: Match patterns one long
    void Compiler::optimizeMatch1(size_t instrc) {
        for (auto iter = textSection.instructions.rbegin(); iter != textSection.instructions.rend(); iter++) {
            if (auto mov = dynamic_cast<MoveOperation*>(*iter)) {
                if (IS_REG(mov->dest) && mov->dest.reg < 8 && mov->src.isLiteral && mov->src.value.u < 0b11111111) {
                    mov->dest.reg += 8;
                } else if (mov->src == mov->dest) {
                    textSection.instructions.erase(--(iter.base()));
                }
            }
        }
    }

    // MARK: Match patterns two long
    bool Compiler::optimizeMatch2(size_t instrc) {
        int opcount {};
        if (instrc > 1) {
            instrc--;
            for (size_t i = 0; i < instrc; i++) {
                const size_t old_i = i;
                Instruction* a {textSection.instructions[i]};
                Instruction* b {textSection.instructions[i + 1]};
                if (auto amov = dynamic_cast<MoveOperation*>(a)) {
                    if (auto bmov = dynamic_cast<MoveOperation*>(b)) {
                        if (NO_OPTM(amov) || NO_OPTM(bmov)) {
                            continue;
                        }
                        if (amov->dest == bmov->src && IS_REG(amov->dest)) {
                            amov->dest = bmov->dest;
                            CAT_COMMENTS(amov, bmov);
                            textSection.instructions.erase(textSection.instructions.begin() + i + 1);
                            i--;
                        } else if (amov->dest == bmov->src && amov->src.isDereference && IS_REG(bmov->dest) && IS_REG(bmov->src)) {
                            amov->dest = bmov->dest;
                            CAT_COMMENTS(amov, bmov);
                            textSection.instructions.erase(textSection.instructions.begin() + i + 1);
                            i--;
                        } else if (amov->dest.reg == bmov->dest.reg && bmov->dest.isDereference && (bmov->src.isLiteral || IS_REG(bmov->src)) && IS_REG(amov->src)) {
                            bmov->dest = amov->src;
                            bmov->dest.isDereference = true;
                            INSRT_COMMENTS(bmov, amov);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        } else if (amov->dest == bmov->dest && amov->dest.reg != bmov->src.reg) {
                            INSRT_COMMENTS(bmov, amov);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        } else if (amov->dest == bmov->src && !amov->src.isDereference && !bmov->dest.isDereference) {
                            bmov->src = amov->src;
                            bmov->comment += " (with " + amov->dest.str() + " = " + amov->src.str() + ')';
                        }
                    }
                    else if (auto badd = dynamic_cast<AddOperation*>(b)) {
                        if (amov->dest == badd->src && amov->dest.reg != LOC_IS_NOT_REG) {
                            badd->src = amov->src;
                            INSRT_COMMENTS(badd, amov);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        } else if (IS_REG(amov->dest) && badd->dest == amov->dest && !badd->src.isDereference) {
                            if (badd->src.isSigned) {
                                amov->src.value.s += badd->src.value.s;
                            } else {
                                amov->src.value.u += badd->src.value.u;
                            }
                            CAT_COMMENTS(amov, badd);
                            textSection.instructions.erase(textSection.instructions.begin() + i + 1);
                            i--;
                        }
                    }
                    else if (auto bsub = dynamic_cast<SubOperation*>(b)) {
                        if (amov->dest == bsub->src && IS_REG(amov->dest) && IS_REG(amov->src) && !bsub->src.isLiteral) {
                            bsub->src = amov->src;
                            INSRT_COMMENTS(bsub, amov);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        } else if (IS_REG(amov->dest) && bsub->dest == amov->dest && !bsub->src.isDereference && !bsub->src.isLiteral) {
                            if (bsub->src.isSigned) {
                                amov->src.value.s -= bsub->src.value.s;
                            } else {
                                amov->src.value.u -= bsub->src.value.u;
                            }
                            CAT_COMMENTS(amov, bsub);
                            textSection.instructions.erase(textSection.instructions.begin() + i + 1);
                            i--;
                        } else if (amov->dest == bsub->src && IS_REG(amov->dest) && amov->src.isLiteral) {
                            bsub->src = amov->src;
                            INSRT_COMMENTS(bsub, amov);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        }
                    }
                    else if (auto bmul = dynamic_cast<MulOperation*>(b)) {
                        if (amov->dest == bmul->src && IS_REG(amov->dest)) {
                            bmul->src = amov->src;
                            INSRT_COMMENTS(bmul, amov);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        } else if (amov->dest == bmul->dest && ARE_BOTH_LIT(amov, bmul)) {
                            if (bmul->src.isSigned) {
                                amov->src.value.s *= bmul->src.value.s;
                            } else {
                                amov->src.value.u *= bmul->src.value.u;
                            }
                            CAT_COMMENTS(amov, bmul);
                            textSection.instructions.erase(textSection.instructions.begin() + i + 1);
                            i--;
                        }
                    }
                    else if (auto blea  = dynamic_cast<LoadAddressOperation*>(b)) {
                        if (amov->dest == blea->dest && IS_REG(amov->src) && IS_REG(amov->dest) && blea->src.reg == blea->dest.reg) {
                            blea->src.reg = amov->src.reg;
                            blea->dest.reg = amov->src.reg;
                            INSRT_COMMENTS(blea, amov);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        }
                    }
                } else if (auto alea = dynamic_cast<LoadAddressOperation*>(a)) {
                    if (auto bmov = dynamic_cast<MoveOperation*>(b)) {
                        if (alea->dest == bmov->src && alea->dest.reg != LOC_IS_NOT_REG && IS_REG(bmov->dest)) {
                            alea->dest = bmov->dest;
                            CAT_COMMENTS(alea, bmov);
                            textSection.instructions.erase(textSection.instructions.begin() + i + 1);
                            i--;
                        } else if (IS_REG(alea->dest) && bmov->dest.reg == alea->dest.reg && !bmov->dest.isDereference && bmov->dest.isDereference && !bmov->src.isDereference) {
                            bmov->dest = alea->src;
                            INSRT_COMMENTS(bmov, alea);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        }
                    }
                } else if (auto apush = dynamic_cast<PushOperation*>(a)) {
                    if (auto bpop = dynamic_cast<PopOperation*>(b)) {
                        if (apush->src == bpop->dest) {
                            i -= 2;
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                        }
                    }
                }
                opcount += (old_i > i);
            }
        }
        return opcount;
    }

    // MARK: Match patterns 3 long
    void Compiler::optimizeMatch3(size_t instrc) {
        if (instrc > 2) {
            instrc -= 2;
            for (size_t i = 0; i < instrc; i++) {
                Instruction* a {textSection.instructions[i]};
                Instruction* b {textSection.instructions[i + 1]};
                Instruction* c {textSection.instructions[i + 2]};
                if (auto amov = dynamic_cast<MoveOperation*>(a)) {
                    if (auto badd = dynamic_cast<AddOperation*>(b)) {
                        if (auto cmov = dynamic_cast<MoveOperation*>(c)) {
                            if (IS_REG(badd->src) && IS_REG(badd->dest) && IS_REG(cmov->src) &&
                                amov->dest == badd->dest && badd->dest == cmov->src
                                ) {
                                badd->dest = badd->src;
                                badd->src = amov->src;
                                cmov->src = badd->dest;
                                badd->comment = amov->comment + " && " + badd->comment;
                                textSection.instructions.erase(textSection.instructions.begin() + i);
                                if (cmov->src == cmov->dest) {
                                    cmov->comment = badd->comment + " && " + cmov->comment;
                                    textSection.instructions.erase(textSection.instructions.begin() + i + 1); // not two since its all shifted
                                }
                            }
                        }
                    } else if (auto bsub = dynamic_cast<SubOperation*>(b)) {
                        if (auto cmov = dynamic_cast<MoveOperation*>(c)) {
                            if (IS_REG(bsub->src) && IS_REG(bsub->dest) && IS_REG(cmov->src) &&
                                amov->dest == bsub->dest && bsub->dest == cmov->src
                                ) {
                                bsub->dest = bsub->src;
                                bsub->src = amov->src;
                                cmov->src = bsub->dest;
                                INSRT_COMMENTS(bsub, amov);
                                textSection.instructions.erase(textSection.instructions.begin() + i);
                                if (cmov->src == cmov->dest) {
                                    INSRT_COMMENTS(cmov, bsub);
                                    textSection.instructions.erase(textSection.instructions.begin() + i + 1); // not two since its all shifted
                                }
                            } else if (amov->src == cmov->dest && amov->dest == cmov->src && bsub->dest == amov->dest) {
                                bsub->dest = amov->src;
                                INSRT_COMMENTS(bsub, amov);
                                CAT_COMMENTS(bsub, cmov);
                                textSection.instructions.erase(textSection.instructions.begin() + i);
                                textSection.instructions.erase(textSection.instructions.begin() + i + 1); // not two since its all shifted
                            }
                        }
                    } else if (auto bmul = dynamic_cast<MulOperation*>(b)) {
                        if (auto cmov = dynamic_cast<MoveOperation*>(c)) {
                            if (IS_REG(bmul->src) && IS_REG(bmul->dest) && IS_REG(cmov->src) &&
                                amov->dest == bmul->dest && bmul->dest == cmov->src
                                ) {
                                bmul->dest = bmul->src;
                                bmul->src = amov->src;
                                cmov->src = bmul->dest;
                                bmul->comment = amov->comment + " && " + bmul->comment;
                                textSection.instructions.erase(textSection.instructions.begin() + i);
                                if (cmov->src == cmov->dest) {
                                    cmov->comment = bmul->comment + " && " + cmov->comment;
                                    textSection.instructions.erase(textSection.instructions.begin() + i + 1); // not two since its all shifted
                                }
                            }
                        }
                    }
                } else if (auto apop = dynamic_cast<PopOperation*>(a)) {
                    if (auto bpush = dynamic_cast<PushOperation*>(b)) {
                        if (auto cpop = dynamic_cast<PopOperation*>(c)) {
                            if (apop->dest == bpush->src) {
                                textSection.instructions.insert(textSection.instructions.begin() + i + 3, new MoveOperation(cpop->dest, bpush->src, SizeType::qword, cpop->comment));
                                
                                textSection.instructions.erase(textSection.instructions.begin() + i);
                                textSection.instructions.erase(textSection.instructions.begin() + i);
                                textSection.instructions.erase(textSection.instructions.begin() + i);
                                i -= 3;
                            }
                        }
                    }
                }
            }
        }
    }

    // MARK: Remove redundant code
    void Compiler::optimizeOutRedundancy(size_t instrc) {
        if (instrc > 5) {
            instrc -= 5;
            for (size_t i = 0; i < instrc; i++) {
                if (dynamic_cast<Label*>(textSection.instructions[i])) { i++;
                Instruction* a {textSection.instructions[i]};
                Instruction* b {textSection.instructions[i + 1]};
                if (auto apush = dynamic_cast<PushOperation*>(a)) if (auto bmov = dynamic_cast<MoveOperation*>(b)) {
                    if (auto stackalloc = dynamic_cast<SubOperation*>(textSection.instructions[i + 2])) {
                    }
                    const size_t start_i = i;
                    i += 2;
                    std::vector<Instruction*> body;
                    bool exit {};
                    while (!exit) {
                        Instruction* c {textSection.instructions[i]};
                        Instruction* d {textSection.instructions[i + 1]};
                        if (auto movc = dynamic_cast<MoveOperation*>(c)) {if (auto popd = dynamic_cast<PopOperation*>(d)) {
                            if (std::find_if(body.begin(), body.end(), [](Instruction* instr) -> bool {
                                return dynamic_cast<CallOperation*>(instr);
                            }) == body.end()) {
                                textSection.instructions.erase(textSection.instructions.begin() + i);
                                textSection.instructions.erase(textSection.instructions.begin() + i);
                                textSection.instructions.erase(textSection.instructions.begin() + start_i);
                                textSection.instructions.erase(textSection.instructions.begin() + start_i);
                                if (auto stackalloc = dynamic_cast<SubOperation*>(textSection.instructions[start_i])) {
                                    if (stackalloc->dest.reg == static_cast<int>(Register::rsp)) {
                                        textSection.instructions.erase(textSection.instructions.begin() + start_i);
                                        i--;
                                    }
                                }
                                i -= 4;
                            }
                            exit = true;
                        } else {
                            body.push_back(textSection.instructions[i++]);
                        }} else {
                            body.push_back(textSection.instructions[i++]);
                        }
                    }
                }
            }}
        }
    }

    // MARK: General compiliation
    void Compiler::compile(const File *file) {
        reset(); // in case the function is called more than once
        
        analyzer.analyze(file); // perform static analysis to gain control flow and type information
        if (_showTypeTrace) analyzer.dumpTypeTrace();
        
        ColoredStream out;
        if (analyzer.hasWarnings() || analyzer.hasErrors()) {
            out << Color::blue << Color::bold << "Analyzer Output" << Color::reset << "\n---------------\n";
        }
        
        if (analyzer.hasWarnings()) {
            for (auto warning: analyzer.warnings()) {
                warning.path = _path;
                warning.print(_src);
            }
        }
        if (analyzer.hasErrors()) {
            for (auto err: analyzer.errors()) {
                err.path = _path;
                err.print(_src);
            }
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
            optimize(optimization);
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

    void Compiler::_debugInsert(Instruction* instr) {
        emit(instr, SectionType::text);
    }

    void Compiler::setPath(const std::string& path) {
        _path = path;
    }

    void Compiler::showTypeTrace(bool show) {
        _showTypeTrace = show;
    }

    void Compiler::_strprocess(std::string& str) {
        long long idx = str.size();
        while (idx > 0) {
            --idx;
            if (str[idx - 1] == '\\') {
                switch (str[idx]) {
                    case 't': {
                        str.erase(str.begin() + idx - 1, str.begin() + idx + 1);
                        const std::string insrt {"`, 0x9, `"};
                        str.insert(idx - 1, insrt);
                        idx -= 2;
                        break;
                    }
                    case 'r': {
                        str.erase(str.begin() + idx - 1, str.begin() + idx + 1);
                        const std::string insrt {"`, 0xD, `"};
                        str.insert(idx - 1, insrt);
                        idx -= 2;
                        break;
                    }
                    case 'e': {
                        str.erase(str.begin() + idx - 1, str.begin() + idx + 1);
                        const std::string insrt {"`, 0x1B, `"};
                        str.insert(idx - 1, insrt);
                        idx -= 2;
                        break;
                    }
                    case '\"': {
                        str.erase(str.begin() + idx - 1, str.begin() + idx + 1);
                        const std::string insrt {"`, 0x22, `"};
                        str.insert(idx - 1, insrt);
                        idx -= 2;
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
}}
