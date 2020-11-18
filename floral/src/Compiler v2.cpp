//
//  Compiler_v2.cpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 9/9/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//
// ld -r -o std.o colored256io.o coloredio.o dynamic.o intmap.o itoa.o print_buffered.o print.o read.o strtowstr.o swap8.o sys.o upperlowerascii.o util.o wideio.o wreadc.o wstring.o
#include "Compiler.hpp"
#include "File IO.hpp"
#include <cassert>
#include "Colors.hpp"
#include "floral_cdef.h"

#define CAT_COMMENTS(opleft, opright) if (!((opleft)->comment.empty() || (opright)->comment.empty())) (opleft)->comment += " && " + (opright)->comment
#define INSRT_COMMENTS(opleft, opright) if (!((opleft)->comment.empty() || (opright)->comment.empty())) (opleft)->comment = (opright)->comment + " && " + (opleft)->comment

namespace Floral {
    // MARK: Constructor/Deinitializer
    Compiler::Compiler(): textSection(SectionType::text), bssSection(SectionType::bss), rodataSection(SectionType::rodata), dataSection(SectionType::data) {}
    Compiler::~Compiler() {}

    // MARK: Error and utility
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
        if (!staticEvalExpr) return "";
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
                case TokenType::minus:
                    if (left && right) return std::to_string(atoll(staticEvalulate(left).c_str()) + atoll(staticEvalulate(right).c_str()));
                    else if (!left && right) return std::to_string(-atoll(staticEvalulate(left).c_str()));
                case TokenType::multiply:
                    if (left && right) return std::to_string(atoll(staticEvalulate(left).c_str()) * atoll(staticEvalulate(right).c_str()));
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
        } else if (auto cast = dynamic_cast<UnsafeCast*>(staticEvalExpr)) {
            return staticEvalulate(cast->expr());
        } else if (auto arraylit = dynamic_cast<ArrayLiteralExpression*>(staticEvalExpr)) {
            std::string acc;
            for (auto expr: arraylit->values()) {
                acc += staticEvalulate(expr);
                acc += ", ";
            }
            if (!arraylit->values().empty()) {
                acc.pop_back();
                acc.pop_back();
            }
            return acc;
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
        emitSGEpilogue();
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
//        if (auto layer1 = dynamic_cast<BinaryExpression*>(assignStm->lval())) {
//            if (layer1->op() && layer1->op()->tkntype() == TokenType::multiply && !layer1->left()) {
//                PointerAssignment ptrAssign { layer1->right()->_loc, layer1->right(), assignStm->rval() };
//                emitPointerAssignmentStatement(&ptrAssign);
//                ptrAssign.makenull();
//                return;
//            } else if (layer1->op() && layer1->op()->tkntype() == TokenType::leftBracket) {
//                const Token addOp { {layer1->op()->_loc.pos, layer1->op()->_loc.startLine}, TokenType::plus, "+" };
//                Expression* pointer = new BinaryExpression(layer1->_loc, layer1->left(), new OperatorComponentExpression(addOp), layer1->right());
//                PointerAssignment ptrAssign { assignStm->_loc, pointer, assignStm->rval() };
//                emitPointerAssignmentStatement(&ptrAssign);
//                ptrAssign.makenull();
//                return;
//            }
//        }
//        const Token referenceOp { {assignStm->lval()->_loc.pos, assignStm->lval()->_loc.startLine}, TokenType::andOp, "&" };
//        Expression* pointer = new BinaryExpression(assignStm->lval()->_loc, nullptr, new OperatorComponentExpression(referenceOp), assignStm->lval());
//        PointerAssignment ptrAssign { assignStm->_loc, pointer, assignStm->rval() };
//        emitPointerAssignmentStatement(&ptrAssign);
//        ptrAssign.makenull();
        const auto lhsloc = emitExpression(assignStm->lval(), true);
        const auto rhsloc = emitExpression(assignStm->rval());
        if (IS_REG(lhsloc)) {
            emit(new MoveOperation(ValueAtRegisterLocation(static_cast<Register>(lhsloc.reg)), rhsloc, SizeType::qword, "assignment"), SectionType::text);
            returnRegister(static_cast<Register>(lhsloc.reg));
        } else if (IS_RBPOFFSET(lhsloc)) {
            const Register temp = static_cast<Register>(currentFrame().avaliableScratch());
            Location lhslocd = lhsloc;
            lhslocd.isDereference = true;
            if (lhsloc.isDereference) {
                emit(new MoveOperation(RegisterLocation(temp), lhslocd, SizeType::qword, "put lval in temp reg"), SectionType::text);
            } else {
                emit(new LoadAddressOperation(RegisterLocation(temp), lhslocd, SizeType::qword, "put lval in temp reg"), SectionType::text);
            }
            emit(new MoveOperation(ValueAtRegisterLocation(temp), rhsloc, SizeType::qword, "assignment into dereference"), SectionType::text);
            returnRegister(temp);
        }
        if (IS_REG(rhsloc)) {
            returnRegister(static_cast<Register>(rhsloc.reg));
        }
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
        
        const std::string name = currentFrame().id + "_#if_skip_" + std::to_string(counter++);
        const auto [loc, cond] = emitCondition(ifStm->condition(), true, true); // calculate condition

        if (loc.isLiteral) {
            if (loc.value.u) {
                emitBlock(ifStm->body()); // unconditionally emit block
            } else {
                return;
            }
        } else {
            emit(new JumpOperation(cond, name), SectionType::text);
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
        emit(new Label(name, false, false), SectionType::text); // label to loop to
        const auto [loc, cond] = emitCondition(whileStm->condition(), true, true); // calculate condition
        emit(new JumpOperation(cond, skipName), SectionType::text);
        if (IS_REG(loc)) {
            returnRegister(static_cast<Register>(loc.reg));
        }
        emitBlock(whileStm->body());

        if (loc.isLiteral) {
            if (loc.value.u) {
                emit(new JumpOperation(JumpOperation::JType::normal, name), SectionType::text); // unconditionally loop block
            } else {
                return;
            }
        } else {
            emit(new JumpOperation(JumpOperation::JType::normal, name), SectionType::text); // nonzero difference = false
            emit(new Label(skipName, false, false), SectionType::text); // label to loop to
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
        } else if (auto strct = dynamic_cast<StructDeclaration*>(decl)) {
            emitStruct(strct);
        } else if (auto nmspace = dynamic_cast<NamespaceDeclaration*>(decl)) {
            emitNamespace(nmspace);
        }
    }

    // MARK: Emit namespace declaration
    void Compiler::emitNamespace(NamespaceDeclaration* nmspace) {
        for (auto node: nmspace->nodes()) {
            if (auto decl = dynamic_cast<Declaration*>(node)) {
                emitDeclaration(decl);
            }
        }
    }

    // MARK: Emit global constant
    void Compiler::emitGlobal(GlobalDeclaration *gbl) {
        Initializer* init = gbl->initializer();
        if (init->type == Initializer::zero) {
            emit(new ZeroData(gbl->name.contents, OPSIZE_FROM_NUM(gbl->type->alignment()), 1), SectionType::bss);
        } else if (auto direct = dynamic_cast<const DirectInitializer*>(init)) {
            emit(new RawText(INDENT + prefixed(gbl->name.contents) + ": " + ((direct->expr()->type->isPointer() && GET_PTRTYYPE(direct->expr()->type)->size() == 1) ? "db " : "dq ") + staticEvalulate(direct->expr())), SectionType::rodata);
        } else if (auto copy = dynamic_cast<const CopyInitializer*>(init)) {
            emit(new RawText(INDENT + prefixed(gbl->name.contents) + ": " + ((copy->expr()->type->isPointer() && GET_PTRTYYPE(copy->expr()->type)->size() == 1) ? "db " : "dq ") + staticEvalulate(copy->expr())), SectionType::rodata);
        }
    }

    // MARK: Emit forward-declared global constant
    void Compiler::emitExternGlobal(GlobalForwardDeclaration *fgbl) {
        emit(new Extern(fgbl->name().contents, "@ global " + fgbl->name().contents + ": " + fgbl->type()->des()), SectionType::text);
    }

    // MARK: Emit struct constructor
    // Struct passed alongside constructor for neecssary context
    void Compiler::emitStructConstructor(StructDeclaration* strct, StructConstructor* constr) {
        const std::string str = analyzer.strFromFunctionSignature({ strct->name().contents + "._CONSTR", constr->params });
        emit(new Label(str, false), SectionType::text);
        const long structStart = currentFrame().nextOffset();
        const long dif = currentFrame().size + 8;
        emitEnter();
        loadParametersIntoFrame(constr->params);
        for (auto init: constr->inits) {
            Location result = emitExpression(init.second);
            const long localizedStructStart = dif - structStart;
            const long offset = strct->offsetOf(init.first.contents);
            const long memberOffset = localizedStructStart - offset;
            
            if (result.isDereference) {
                const Register reg = GET_REG(currentFrame());
                emit(new MoveOperation(RegisterLocation(reg), result, SizeType::qword, "move result into temp reg"), SectionType::text);
                result = RegisterLocation(reg);

                returnRegister(reg);
            }
            
            emit(new MoveOperation(RBPOffsetLocation(memberOffset), result, SizeType::qword, "member init"), SectionType::text);
        }
        emitStatement(constr->after);
        leaveFrame();
        emitLeave();
        emitRet();
    }

    // MARK: Emit struct
    void Compiler::emitStruct(StructDeclaration* strct) {
        frames.push_back({});
        auto t = new Type(0, strct->name().contents);
        currentFrame().addData(RegisterLocation(Register::rdi), 8, "this");
        for (auto fnmem: strct->functionMembers()) {
            fnmem->_name.contents.insert(0, strct->name().contents + ".");
            emitFunction(fnmem, true);
        }
        for (auto constr: strct->constructors()) {
            emitStructConstructor(strct, constr);
        }
        leaveFrame();
        delete t;
    }

    // If array literal, sequentially initialize the elements
   #define ARRAY_BRANCH(d, n) if ((d)->type()->isArray()) {\
       if (auto arraylit = dynamic_cast<ArrayLiteralExpression*>(init->expr())) {\
           size_t i = arraylit->values().size();\
           for (auto iter = arraylit->values().rbegin(); iter != arraylit->values().rend(); iter++) {\
               auto val = *iter;\
               Location result = emitExpression(val);\
               if (result.isDereference) {\
                   const Register reg = GET_REG(currentFrame());\
                   emit(new MoveOperation(RegisterLocation(reg), result, SizeType::qword, "store result in temp reg"), SectionType::text);\
                   result = RegisterLocation(reg);\
                   returnRegister(reg);\
               }\
                emit(new MoveOperation(RBPOffsetLocation(currentFrame().nextOffset()), result, OPSIZE_FROM_NUM(val->type->size()), #n " " + name + '[' + std::to_string(--i) + "];"), SectionType::text);\
                if (!i) {\
                    currentFrame().addData(RBPOffsetLocation(currentFrame().nextOffset()), val->type->size(), name);\
                } else {\
                    currentFrame().addData(RBPOffsetLocation(currentFrame().nextOffset()), val->type->size(), name + " + " + std::to_string(i));\
                }\
               if (IS_REG(result)) {\
                   returnRegister(static_cast<Register>(result.reg));\
               }\
           }\
           break;\
       }\
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
                
                 if (v->type()->isArray()) {
                     const std::string lbl = name + "#zeroarray";
                     const size_t elementSize = GET_PTRTYYPE(v->type())->size();
                     const size_t arrayCount = v->type()->_staticArray->second;
                     currentFrame().addData(RelLabelL(lbl), elementSize * arrayCount, name);
                     //currentFrame().data.back().loc.isDereference = false;
                     // If array literal, sequentially initialize the elements
                     emit(new ZeroData(lbl, OPSIZE_FROM_NUM(elementSize), arrayCount), SectionType::bss);
                 } else {
                    emit(
                         new MoveOperation(
                            RBPOffsetLocation(currentFrame().nextOffset()),
                            NumLL(false, SU(0ULL)),
                            SizeType::qword, //OPSIZE_FROM_NUM(size),
                            "@ var " + name + " = 0"
                        ),
                        SectionType::text
                    );
                    currentFrame().addData(RBPOffsetLocation(currentFrame().nextOffset()), size, name);
                }
                break;
            }
            case Initializer::direct: {
                const auto init {static_cast<const DirectInitializer*>(v->initializer())};
                auto exprtype = init->expr()->type;
                const size_t size = v->type()->size();
                const std::string name = v->name().contents;
                ARRAY_BRANCH(v, var)
                const Location result = emitExpression(init->expr(), false, exprtype->isPointer() ? !GET_PTRTYYPE(exprtype)->isConst() : !exprtype->isConst());
                emit(
                    new MoveOperation(
                        RBPOffsetLocation(currentFrame().nextOffset()),
                        result,
                        SizeType::qword, //OPSIZE_FROM_NUM(size),
                        "@ var " + name + ": " + v->type()->des()
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
                auto exprtype = init->expr()->type;
                const size_t size = v->type()->size();
                const std::string name = v->name().contents;
                ARRAY_BRANCH(v, var)
                const Location result = emitExpression(init->expr(), false, exprtype->isPointer() ? !GET_PTRTYYPE(exprtype)->isConst() : !exprtype->isConst());
                if (dynamic_cast<ConstructExpression*>(init->expr())) {
                    currentFrame().addData(result, size, name);
                    return;
                }
                emit(
                    new MoveOperation(
                        RBPOffsetLocation(currentFrame().nextOffset()),
                        result,
                        SizeType::qword, //OPSIZE_FROM_NUM(size),
                        "@ var " + name + ": " + v->type()->des()
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
                
                if (l->type()->isArray()) {
                    const std::string lbl = name + "#zeroarray";
                    const size_t elementSize = GET_PTRTYYPE(l->type())->size();
                    const size_t arrayCount = l->type()->_staticArray->second;
                    currentFrame().addData(RelLabelL(lbl), elementSize * arrayCount, name);
                    //currentFrame().data.back().loc.isDereference = false;
                        // If array literal, sequentially initialize the elements
                    emit(new ZeroData(lbl, OPSIZE_FROM_NUM(elementSize), arrayCount), SectionType::bss);
                } else {
                    emit(
                        new MoveOperation(
                            RBPOffsetLocation(currentFrame().nextOffset()),
                            NumLL(false, SU(0ULL)),
                            SizeType::qword, //OPSIZE_FROM_NUM(size),
                            "@ let " + name + " = 0"
                        ),
                        SectionType::text
                    );
                    currentFrame().addData(RBPOffsetLocation(currentFrame().nextOffset()), size, name);
                }
                break;
            }
            case Initializer::direct: {
                const auto init {static_cast<const DirectInitializer*>(l->initializer())};
                auto exprtype = init->expr()->type;
                const size_t size = l->type()->size();
                const std::string name = l->name().contents;
                ARRAY_BRANCH(l, let)
                const Location result = emitExpression(init->expr(), false, exprtype->isPointer() ? !GET_PTRTYYPE(exprtype)->isConst() : !exprtype->isConst());
                emit(
                    new LoadAddressOperation(
                        RBPOffsetLocation(currentFrame().nextOffset()),
                        result,
                        SizeType::qword, //OPSIZE_FROM_NUM(size),
                        "@ let " + name + ": " + l->type()->des()
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
                auto exprtype = init->expr()->type;
                const size_t size = l->type()->size();
                const std::string name = l->name().contents;
                ARRAY_BRANCH(l, let)
                const Location result = emitExpression(init->expr(), false, exprtype->isPointer() ? !GET_PTRTYYPE(exprtype)->isConst() : !exprtype->isConst());
                emit(
                    new MoveOperation(
                        RBPOffsetLocation(currentFrame().nextOffset()),
                        result,
                        SizeType::qword, //OPSIZE_FROM_NUM(size),
                        "@ let " + name + ": " + l->type()->des()
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
    void Compiler::emitFunction(Function *func, bool isFunctionMember) {
        const FunctionSignature funsig {func->name().contents, func->parameters()};
        const auto flbl {analyzer.strFromFunctionSignature(funsig)};
        emit(new Label(flbl, !func->isStatic()), SectionType::text); // label this code
        emitEnter(); currentFrame().id = flbl; // create new frame
                
        if (func->body().size() == 1) {
            if (auto ret = dynamic_cast<ReturnStatement*>(func->body().front())) {
                loadParametersIntoFrame(func->parameters(), false, isFunctionMember);
                emitReturnStatement(ret);
                leaveFrame();
                return;
            }
        }
        
        // [rbp-N] = first function parameter, where N is the
        //           sum of the sizes of the function parameter
        //           aligned to 16 bytes
        // [rbp] = old stack frame
        // [rbp-8] = first local variable
        auto sz = (long long)(func->staticAllocationSize + func->parameters().size() * 8);
        long long staticAllocationSize { (sz + 15) & (-16) };
        if (_stackGuard) {
            staticAllocationSize += 16;
        }

        if (staticAllocationSize && (!(staticAllocationSize > 128 && func->isLeaf()) || _stackGuard)) {
            emit(new SubOperation(RegisterLocation(Register::rsp), NumLL(true, SU(staticAllocationSize)), "allocate space on the stack for local variables"), SectionType::text);
            currentFrame().size = staticAllocationSize;
        }
        
        emitSGPrologue();

        loadParametersIntoFrame(func->parameters(), true);
        
        size_t i = 0;
        for (auto node: func->body()) {
            if (auto stm = dynamic_cast<Statement*>(node)) {
                if (i + 2 == func->body().size() && !_stackGuard && func->returnType()->isVoid()) if (auto callStm = dynamic_cast<CallStatement*>(stm)) {
                    emitCall(callStm->call, true);
                    leaveFrame();
                    return;
                }
                emitStatement(stm);
            }
            if (auto decl = dynamic_cast<Declaration*>(node)) emitDeclaration(decl);
            i++;
        }
        
        // Integer return values up to 64 bits in size are stored in RAX while values up to 128 bit are stored in RAX and RDX
        leaveFrame(); // pop_back the current Frame from std::vector frames
    }

    void Compiler::emitExternFunc(FunctionForwardDeclaration* ffunc) {
        const FunctionSignature funsig {ffunc->name().contents, ffunc->parameters()};
        std::string acc = "@ " + ffunc->name().contents + '(';
        for (auto param: ffunc->parameters()) {
            acc += param.type->des();
            acc += ", ";
        }
        acc.pop_back(); acc.pop_back();
        acc.push_back(')');
        if (ffunc->returnType()->isIncomplete()) {
            acc += ": const Void";
        } else {
            acc += ": ";
            acc += ffunc->returnType()->des();
        }
        emit(new Extern(analyzer.strFromFunctionSignature(funsig), acc), SectionType::text);
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
    void Compiler::emitSGPrologue() {
        if (_stackGuard) {
            currentFrame().addData(RBPOffsetLocation(-8), 8, "Stack Guard");
            currentFrame().addData(RBPOffsetLocation(-16), 8, "Null Qword");
            emit(new MoveOperation(RETURN_VALUE_LOC, RBPOffsetLocation(0), SizeType::qword, "load old rbp"), SectionType::text);
            emit(new XorOperation(RETURN_VALUE_LOC, RBPOffsetLocation(8), "xor with return address"), SectionType::text);
            emit(new MoveOperation(RBPOffsetLocation(-8), RETURN_VALUE_LOC, SizeType::qword, "stack guard"), SectionType::text);
            emit(new MoveOperation(RBPOffsetLocation(-16), NumLL(false, SU(0ULL)), SizeType::qword, "null bytes"), SectionType::text);
        }
    }
    void Compiler::emitSGEpilogue() {
        if (_stackGuard) {
            const Register reg = GET_REG(currentFrame());
            emit(new MoveOperation(RegisterLocation(reg), RBPOffsetLocation(0), SizeType::qword, "load old rbp "), SectionType::text);
            emit(new XorOperation(RegisterLocation(reg), RBPOffsetLocation(8), "xor with return address"), SectionType::text);
            emit(new CmpOperation(RegisterLocation(reg), RBPOffsetLocation(-8), "check if different than stack guard"), SectionType::text);
            static int skipper = 0;
            const std::string skiplbl = '#' + currentFrame().id + "#check_skip_" + std::to_string(skipper++);
            emit(new JumpOperation(JumpOperation::JType::equal, skiplbl, "no failure"), SectionType::text);
            emit(new MoveOperation(RegisterLocation(Register::rdi), RBPOffsetLocation(8), SizeType::qword, "return address"), SectionType::text);
            emit(new MoveOperation(RegisterLocation(Register::rsi), RegisterLocation(Register::rbp), SizeType::qword, "base pointer"), SectionType::text);
            emit(new CallOperation("stkgrd_fail_u_u", "handle if failure"), SectionType::text);
            emit(new Label(skiplbl, false, false), SectionType::text);
            returnRegister(reg);
        }
    }
        void Compiler::loadParametersIntoFrame(const Function::Parameters& params, bool storeAsLocalVars, bool isFunctionMember) {
            const size_t size = params.size();
                if (size == 0) return;
                
                int integers = isFunctionMember ? 1 : 0;
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
                            if (storeAsLocalVars) {
                                const Location dest = RBPOffsetLocation(currentFrame().nextOffset());
                                emit(new MoveOperation(dest, RegisterLocation(integerRegs[integers++]), SizeType::qword, "@ load register parameter to local var"), SectionType::text);
                                currentFrame().addData(dest, param.type->alignment(), param.name.contents);
                            } else {
                                currentFrame().addData(RegisterLocation(integerRegs[integers++]), param.type->alignment(), param.name.contents);
                            }
                        }
                    }
        //            else {
        //                floats++;
        //            }
                }
        }

        // MARK: Variable lookup


        std::pair<Variable, bool> Compiler::lookup(const std::string& name) {
            long framesOffset = 0;
            if (auto gbl = analyzer.lookupGlobal(name)) {
                Variable v;
                v.loc = RelLabelL(name);
                v.size = gbl->type->size();
                v.name = name;
                return { v, true };
            }
            for (std::vector<Frame>::const_reverse_iterator i = frames.rbegin(); i != frames.rend(); ++i) {
                auto result = (*i).localLookup(name);
                if (result.second) {
                    result.first.loc.offset += framesOffset;
                    return result;
                }
                framesOffset += 16;
            }
            return { {0, 0}, false };
        }

        // MARK: Emit general expression (COMPLEX)
        Location Compiler::emitExpression(Expression* expr, bool wantsAddressResult, bool mut) {
            if (auto binary = dynamic_cast<BinaryExpression*>(expr)) {
                // assume true binary expression for now
                Expression* left {binary->left()};
                const OperatorComponentExpression* op {binary->op()};
                Expression* right {binary->right()};
                
                switch (op->tkntype()) {
                    case TokenType::plus: {
                        if (!left && right) return emitExpression(right);
                        if (left->type->isPointer()) {
                            const int r = currentFrame().avaliableScratch();
                            const Location leftloc = emitExpression(left);
                            const Location rightloc = emitExpression(right);
                            if (rightloc.isLiteral) {
                                if (rightloc.isSigned) {
                                    emit(new RawText(INDENT "lea " + registerNames[r] + ", [" + leftloc.str() + '+' + std::to_string(GET_PTRTYYPE(left->type)->size() * rightloc.value.s) + "] ; pointer arithmetic (" + left->type->des() + " + " + right->prettystr() + ")"), SectionType::text);
                                } else {
                                    emit(new RawText(INDENT "lea " + registerNames[r] + ", [" + leftloc.str() + '+' + std::to_string(GET_PTRTYYPE(left->type)->size() * rightloc.value.u) + "] ; pointer arithmetic (" + left->type->des() + " + " + right->prettystr() + ")"), SectionType::text);
                                }
                            } else {
                                auto size = GET_PTRTYYPE(left->type)->size();
                                std::string mult = size == 1 ? "" : ('*' + std::to_string(size));
                                emit(new RawText(INDENT "lea " + registerNames[r] + ", [" + leftloc.str() + '+' + rightloc.str() + mult + "] ; pointer arithmetic (" + left->type->des() + " + " + right->prettystr() + ")"), SectionType::text);

                            }
                            if (IS_REG(leftloc)) returnRegister(static_cast<const Register>(leftloc.reg));
                            if (IS_REG(rightloc)) returnRegister(static_cast<const Register>(rightloc.reg));
                            return RegisterLocation(static_cast<Register>(r));
                        } else {
                            return emitBinaryExpr(left, right, OpType::add);
                        }
                    }
                    case TokenType::minus: {
                        if (left && right) return emitBinaryExpr(left, right, OpType::sub);
                        else if (!left && right) {
                            const Location result = emitExpression(right);
                            if (result.isLiteral) {
                                if (result.isSigned) {
                                    return NumLL(true, SU(-result.value.s));
                                } else {
                                    return NumLL(false, SU(-result.value.u));
                                }
                            }
                            emit(new NegationOperation(result, "two's complement negation"), SectionType::text);
                            return result;
                        }
                    }
                    case TokenType::multiply: {
                        if (left && right) return emitBinaryExpr(left, right, OpType::imul);
                        else if (!left && right) {
                            if (wantsAddressResult) {
                                return emitExpression(right, true);
                            }
                            Location loc = emitExpression(right, true);
                            if (IS_REG(loc)) {
                                const Register reg = static_cast<Register>(loc.reg);
                                emit(new MoveOperation(RegisterLocation(reg), ValueAtRegisterLocation(reg), SizeType::qword, "dereference"), SectionType::text);
                                return RegisterLocation(reg);
                            } 
                        }
                    }
                    case TokenType::bit_and: {
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
                        const Location lhs = emitExpression(left);
                        const Location rhs = emitExpression(right);
                        
                        emitSaveRegisters({ Register::rax, Register::rdx });
                        
                        emit(new XorOperation(RegisterLocation(Register::edx), RegisterLocation(Register::edx), "clear rdx"), SectionType::text);
                        emit(new MoveOperation(RETURN_VALUE_LOC, lhs, SizeType::qword, "lhs of division into rax"), SectionType::text);
                        emit(new DivOperation(rhs), SectionType::text);

                        if (IS_REG(lhs) && lhs.reg) {
                            returnRegister(static_cast<Register>(lhs.reg));
                        }
                        if (IS_REG(rhs)) {
                            returnRegister(static_cast<Register>(rhs.reg));
                        }
                        
                        Location result = RETURN_VALUE_LOC;
                        
                        emitRestoreRegisters({ Register::rax, Register::rdx });
                        const Register reg = static_cast<Register>(currentFrame().avaliableScratch());
                        emit(new MoveOperation(RegisterLocation(reg), RETURN_VALUE_LOC, SizeType::qword, "save result of div"), SectionType::text);
                        emit(new PopOperation(RegisterLocation(Register::rax)), SectionType::text);
                        emit(new PopOperation(RegisterLocation(Register::rdx)), SectionType::text);
                        return result;
                    }
                    case TokenType::plusEqu: {
                        Location pointer = emitExpression(left, true);
                        if (pointer.isLbl) {
                            const Register temp = GET_REG(currentFrame());
                            emit(new LoadAddressOperation(RegisterLocation(temp), pointer, SizeType::qword, "load pointer to temp reg"), SectionType::text);
                            pointer = RegisterLocation(temp);
                        } else if (left->type->isPointer()) {
                            Location cpy = pointer; cpy.isDereference = true;
                             if (!IS_REG(pointer)) {
                                const Register temp = GET_REG(currentFrame());
                                emit(new MoveOperation(RegisterLocation(temp), pointer, SizeType::qword, "load pointer to temp reg"), SectionType::text);
                                pointer = RegisterLocation(temp);
                            }
                            emit(new MoveOperation(pointer, cpy, SizeType::qword, "prevent pointer-to-pointer indirection"), SectionType::text);
                        }
                        const Location increment = emitExpression(right);
                        pointer.isDereference = true;
                        emit(new AddOperation(pointer, increment, SizeType::qword, "add then assign"), SectionType::text);
                        if (wantsAddressResult) {
                            pointer.isDereference = false;
                            return pointer;
                        } else {
                            return pointer;
                        }
                    }
                    case TokenType::leftBracket: {
                        Location pointer = emitExpression(left, true);
                        if (pointer.isLbl) {
                            const Register temp = GET_REG(currentFrame());
                            emit(new LoadAddressOperation(RegisterLocation(temp), pointer, SizeType::qword, "load pointer to temp reg"), SectionType::text);
                            pointer = RegisterLocation(temp);
                        } else if (left->type->isPointer()) {
                             if (!IS_REG(pointer)) {
                                const Register temp = GET_REG(currentFrame());
                                emit(new MoveOperation(RegisterLocation(temp), pointer, SizeType::qword, "load pointer to temp reg"), SectionType::text);
                                pointer = RegisterLocation(temp);
                            }
                            if (pointer.isDereference) {
                                Location cpy = pointer; cpy.isDereference = true;
                                emit(new MoveOperation(pointer, cpy, SizeType::qword, "prevent pointer-to-pointer indirection"), SectionType::text);
                            }
                        }
                        const Location index = emitExpression(right);
                        
                        if (index.isLiteral && IS_RBPOFFSET(pointer)) {
                            if (wantsAddressResult) {
                                const auto size = GET_PTRTYYPE(left->type)->size();
                                pointer.offset += index.isSigned ? (index.value.s * size) : (index.value.u * size);
                                return pointer;
                            }
                        } else if (index.isLiteral && IS_REG(pointer)) {
                            const auto size = GET_PTRTYYPE(left->type)->size();
                            const auto opsize = OPSIZE_FROM_NUM(size);
                            const Register pointerReg = static_cast<Register>(pointer.reg);
                            const int resultint = currentFrame().avaliableScratch();
                            const Register resultreg = static_cast<Register>(resultint);
                            if (opsize != SizeType::qword) emit(new XorOperation(RegisterLocation(resultreg), RegisterLocation(resultreg), "zero out result"), SectionType::text);
                            if (wantsAddressResult) {
                                emit(new LoadAddressOperation(RegisterLocation(resultreg), ValueAtOffsetRegisterLocation(pointerReg, index.value.s * size), opsize, "subscript into result"), SectionType::text);
                            } else {
                                emit(new MoveOperation(RegisterLocation(static_cast<Register>(resultint + REG_IMPL_OFFSET_FOR_SIZE(opsize))), ValueAtOffsetRegisterLocation(pointerReg, index.value.s * size), opsize, "subscript into result"), SectionType::text);

                            }
                            returnRegister(pointerReg);
                            return RegisterLocation(resultreg);
                        } else if (IS_REG(index) && IS_REG(pointer)) {
                            const auto size = GET_PTRTYYPE(left->type)->size();
                            const auto opsize = OPSIZE_FROM_NUM(size);
                            returnRegister(static_cast<Register>(index.reg));
                            if (wantsAddressResult) {
                                
                            } else {
                                if (size-1) emit(new MulOperation(index, NumLL(false, SU((uint64_t)size)), "calculate subscript offset"), SectionType::text);
                                if (opsize == SizeType::qword) {
                                    emit(new RawText(
                                    INDENT "mov " + pointer.str() + ", [" + pointer.str() + "+" + index.str() + "] ; offset"
                                    ), SectionType::text);
                                    return pointer;
                                }
                                //const int resultint = pointer.reg;
                                const static std::string sizeTypeNames[] {
                                    "byte", "word", "dword"
                                };
                                emit(new RawText(
                                                 INDENT "movzx " +
                                                 /*RegisterLocation(static_cast<Register>(resultint + REG_IMPL_OFFSET_FOR_SIZE(opsize)))*/pointer.str() +
                                                 ", " + sizeTypeNames[static_cast<int>(opsize)] +
                                                 " [" + pointer.str() + "+" + index.str() + "] ; offset"
                                                 ), SectionType::text);
//                                if (opsize == SizeType::byte) {
//                                    emit(new AndOperation(RegisterLocation(static_cast<Register>(pointer.reg)), NumLL(false, SU(0xFFULL)), "clear high bytes"), SectionType::text);
//                                } else if (opsize == SizeType::word) {
//                                    emit(new AndOperation(RegisterLocation(static_cast<Register>(pointer.reg)), NumLL(false, SU(0xFFFFULL)), "clear high bytes"), SectionType::text);
//                                }
                                return pointer;
                            }
                        }
                        break;
                    }
                    case TokenType::invert: {
                        const Location loc = emitExpression(right);
                        emit(new NotOperation(loc, "bitwise not"), SectionType::text);
                        return loc;
                    }
                    case TokenType::bool_not: {
                        return emitCondition(expr).first;
                    }
                    case TokenType::equal: {
                        return emitCondition(expr).first;
                    }
                    case TokenType::unequal: {
                        return emitCondition(expr).first;
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
                        
                        emit(new MoveOperation(resultloc, ZeroLL, SizeType::qword), SectionType::text);
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
                    case TokenType::dot: {
                        Location lhsloc = emitExpression(left, true);
                        if (!IS_RBPOFFSET(lhsloc)) {
                            lhsloc.offset = 0;
                            lhsloc.isDereference = false;
                        }
                        if (auto member = dynamic_cast<SymbolExpression*>(right)) {
                            const long offset = -left->type->structValue()->offsetOf(member->value().contents);
                            const Register temp = static_cast<Register>(currentFrame().avaliableScratch());
                            if (lhsloc.isDereference || !wantsAddressResult) {
                                lhsloc.isDereference = true;
                                emit(new MoveOperation(RegisterLocation(temp), lhsloc, SizeType::qword, "move struct into temp reg"), SectionType::text);
                            } else {
                                lhsloc.isDereference = true;
                                emit(new LoadAddressOperation(RegisterLocation(temp), lhsloc, SizeType::qword, "move struct into temp reg"), SectionType::text);
                            }
                            if (offset) {
                                emit(new SubOperation(RegisterLocation(temp), NumLL(true, SU((long long)offset)), "member offset in struct"), SectionType::text);
                            }
                            if (wantsAddressResult) {
                                if (IS_REG(lhsloc)) {
                                    returnRegister(static_cast<Register>(lhsloc.reg));
                                }
                                return RegisterLocation(temp);
                            }
                            emit(new MoveOperation(RegisterLocation(temp), ValueAtRegisterLocation(temp), SizeType::qword, "member access"), SectionType::text);
    //                        emit(new MoveOperation(RegisterLocation(result), ValueAtOffsetRegisterLocation(result, offset), SizeType::qword, "member access"), SectionType::text);
                            return RegisterLocation(temp);
                        } else if (auto call = dynamic_cast<Call*>(right)) {
                            lhsloc.isDereference = true;
                            emit(new LoadAddressOperation(RegisterLocation(Register::rdi), lhsloc, SizeType::qword, "this pointer = first arg"), SectionType::text);
                            call->args.insert(call->args.begin(), nullptr);
                            call->name.contents.insert(0, left->type->structValue()->name().contents + '.');
                            return emitCall(call);
                        }
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
                        emit(new StringData(lbl, stringLiteral), mut ? SectionType::data : SectionType::rodata); // add the labeled string as bytes in section .rodata
                        
                        const Register resultr = static_cast<Register>(frames.back().avaliableScratch()); // get a new register
                        emit(new LoadAddressOperation(RegisterLocation(resultr), RelLabelL(lbl), SizeType::qword, "string literal"), SectionType::text); // dump the string into this new register
                        return RegisterLocation(resultr);
                    }
                    case Literal::LType::wideString: {
                        static long wstrlitCount = 0;
                        const std::string lbl {"#wstr_literal_" + std::to_string(wstrlitCount++)}; // create the label
                        auto wstrData = new Data(lbl, SizeType::dword, false);
                        const auto wchars = literal->value()._wstr;
                        for (auto codepoint: wchars) {
                            wstrData->values.push_back(SU((long long)codepoint));
                        }
                        wstrData->values.push_back(SU(0ULL));
                        emit(wstrData, mut ? SectionType::data : SectionType::rodata);
                        const Register resultr = static_cast<Register>(frames.back().avaliableScratch()); // get a new register
                        emit(new LoadAddressOperation(RegisterLocation(resultr), RelLabelL(lbl), SizeType::qword, "wide string literal"), SectionType::text); // dump the string into this new register
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
                        return NumLL(true, SU(atoll(literal->value().contents.c_str()))); // simply return the integer value
                    }
                    case Literal::LType::decimalUInteger:
                    case Literal::LType::decimalUByte:
                    case Literal::LType::decimalWideUChar:
                    case Literal::LType::decimalUShort:
                    case Literal::LType::decimalUInt32: {
                        return NumLL(false, SU((uint64_t)strtoul(literal->value().contents.c_str(), NULL, 10))); // simply return the integer value
                    }
                    case Literal::LType::hexadecimalInteger: {
                        return NumLL(false, SU((uint64_t)strtoul(literal->value().contents.c_str(), NULL, 16))); // simply return the hex integer value
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
                auto loc = result.first.loc;
                auto d = loc.isDereference;
                if (symbol->type->isPointer()) {
                    loc.isDereference = true;
                } else {
                    loc.isDereference = false;
                }
                if (wantsAddressResult) {
                    loc.isDereference = false;
                    return loc;
                }
                loc.isDereference = d;
                const Register resultr = static_cast<Register>(frames.back().avaliableScratch());
                if (loc.isLbl) {
                    emit(new LoadAddressOperation(RegisterLocation(resultr), loc, OPSIZE_FROM_NUM(result.first.size), "store " + result.first.name + " in " + registerNames[static_cast<int>(resultr)]), SectionType::text);
//                    if (wantsAddressResult) {
//                        
//                    } else {
//                        emit(new MoveOperation(RegisterLocation(resultr), loc, OPSIZE_FROM_NUM(result.first.size), "store " + result.first.name + " in " + registerNames[static_cast<int>(resultr)]), SectionType::text);
//                    }
                } else {
                    emit(new MoveOperation(RegisterLocation(resultr), loc, OPSIZE_FROM_NUM(result.first.size), "store " + result.first.name + " in " + registerNames[static_cast<int>(resultr)]), SectionType::text);
                }
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
            else if (auto constructor = dynamic_cast<ConstructExpression*>(expr)) {
                auto strct = constructor->type()->structValue();
                long offset = 0;
                size_t index = 0;
                long start = currentFrame().nextOffset();
                for (auto datam: strct->dataMembers()) {
                    if (auto var = dynamic_cast<VarStatement*>(datam)) {
                        offset += var->type()->alignment();
                        if (!start) start = offset;
                        Location argresult = emitExpression(constructor->args()[index++]);
                        if (argresult.isDereference) {
                            const Register temp = static_cast<Register>(currentFrame().avaliableScratch());
                            emit(new MoveOperation(RegisterLocation(temp), argresult, SizeType::qword, "store arg in temp reg"), SectionType::text);
                            argresult = RegisterLocation(temp);
                            returnRegister(temp);
                        }
                        emit(new MoveOperation(RBPOffsetLocation(-offset), argresult, SizeType::qword, "@ initialize data member"), SectionType::text);
                        if (IS_REG(argresult)) {
                            returnRegister(static_cast<Register>(argresult.reg));
                        }
                    }
                }
                return RBPOffsetLocation(start);
            }
            else if (auto arrayliteralexpr = dynamic_cast<ArrayLiteralExpression*>(expr)) {
                
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
            Register resultr;
            Register rhsr;
            
            if (IS_REG(lhs)) {
                resultr = static_cast<Register>(lhs.reg);
            } else {
                resultr = static_cast<Register>(frames.back().avaliableScratch());
                emit(new MoveOperation(RegisterLocation(resultr), lhs, SizeType::qword), SectionType::text);
            }
            if (IS_REG(rhs)) {
                rhsr = static_cast<Register>(rhs.reg);
            } else {
                rhsr = static_cast<Register>(frames.back().avaliableScratch());
                emit(new MoveOperation(RegisterLocation(rhsr), rhs, SizeType::qword), SectionType::text);
            }
            
            // move the expression results into the new temp registers
            
            // Perform the operation
            switch (op) {
                case OpType::add: {
                    emit(new AddOperation(RegisterLocation(resultr), RegisterLocation(rhsr), SizeType::qword, "add"), SectionType::text);
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
                case OpType::or_: {
                    emit(new OrOperation(RegisterLocation(resultr), RegisterLocation(rhsr), "bitwise or"), SectionType::text);
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

    // MARK: Emit condition to set specified flags
    std::pair<Location, Condition> Compiler::emitCondition(Expression* expr, bool inverted, bool justFlags) {
        if (auto binary = dynamic_cast<BinaryExpression*>(expr)) {
            Expression* left = binary->left();
            Expression* right = binary->right();
            
            switch (binary->op()->tkntype()) {
                case TokenType::bool_not: {
                    if (left && !right) {
                        assert(false && "Improper overloading test in SPA");
                    }
                    const Location resultloc = emitExpression(right);
                    if (resultloc.isLiteral) {
                        if (resultloc.value.u) {
                            const Register reg = static_cast<Register>(currentFrame().avaliableScratch());
                            emit(new XorOperation(RegisterLocation(reg), RegisterLocation(reg), "set the zero flag"), SectionType::text);
                            returnRegister(reg);
                            return { TrueLL, inverted ? Condition::nonzero : Condition::zero };
                        } else {
                            const Register reg = static_cast<Register>(currentFrame().avaliableScratch());
                            emit(new XorOperation(RegisterLocation(reg), RegisterLocation(reg), "set the zero flag"), SectionType::text);
                            return { FalseLL, inverted ? Condition::zero : Condition::nonzero };
                        }
                    }
                    
                    emit(new CmpOperation(resultloc, ZeroLL, "set zero flag if false"), SectionType::text); // zero flag set = false
                    emit(new MoveOperation(resultloc, FalseLL, SizeType::qword, "zero result"), SectionType::text); // assume false
                    const Register reg = static_cast<Register>(currentFrame().avaliableScratch());
                    emit(new MoveOperation(RegisterLocation(reg), TrueLL, SizeType::qword, "temp reg for conditional move"), SectionType::text);
                    emit(new RawText(INDENT "cmovz " + RegisterLocation(static_cast<Register>(resultloc.reg + 8)).str() + ", " + registerNames[static_cast<int>(reg) + 8] + " ; set true if zero flag"), SectionType::text); // if false set to true
                    returnRegister(reg);
                    return { resultloc, inverted ? Condition::nonzero : Condition::zero };
                }
                case TokenType::equal: {
                    const Location resultloc = emitExpression(left);
                    const Location rhsloc = emitExpression(right);
                    
                    if (resultloc.isLiteral && rhsloc.isLiteral) {
                        if (justFlags) {
                            return { NumLL(false, SU((uint64_t)(resultloc.value.u == rhsloc.value.u))), inverted ? Condition::unequal : Condition::equal };
                        }
                        const Register r = GET_REG(currentFrame());
                        if (resultloc.value.u != rhsloc.value.u) {
                            emit(new XorOperation(RegisterLocation(r), RegisterLocation(r)), SectionType::text);
                        } else {
                            emit(new XorOperation(RegisterLocation(r), RegisterLocation(r)), SectionType::text);
                            emit(new CmpOperation(RegisterLocation(r), OneLL), SectionType::text);
                        }
                        return { RegisterLocation(r), inverted ? Condition::nonzero : Condition::zero };
                    }
                    
                    emit(new XorOperation(resultloc, resultloc), SectionType::text);
                    emit(new CmpOperation(resultloc, rhsloc), SectionType::text);
                    
                    if (justFlags) {
                        return { RelLabelL("N/A"), inverted ? Condition::zero : Condition::nonzero };
                    }
                    
                    emit(new RawText(INDENT "cmovnz " + RegisterLocation(static_cast<Register>(static_cast<int>(resultloc.reg) + 8)).str() + " ; equal condition = nonzero for true"), SectionType::text);
                    
                    if (IS_REG(rhsloc)) {
                        returnRegister(static_cast<Register>(rhsloc.reg));
                    }
                    
                    return { resultloc, inverted ? Condition::zero : Condition::nonzero };
                }
                case TokenType::unequal: {
                    const Location resultloc = emitExpression(left);
                    const Location rhsloc = emitExpression(right);
                    
                    if (resultloc.isLiteral && rhsloc.isLiteral) {
                        const Register r = static_cast<Register>(currentFrame().avaliableScratch());
                        if (resultloc.value.u != rhsloc.value.u) {
                            emit(new XorOperation(RegisterLocation(r), RegisterLocation(r)), SectionType::text);
                        } else {
                            emit(new MoveOperation(RegisterLocation(r), NumLL(false, SU(1ULL)), SizeType::qword), SectionType::text);
                        }
                        return { RegisterLocation(r), inverted ? Condition::nonzero : Condition::zero };
                    }
                    
                    emit(new SubOperation(resultloc, rhsloc, resultloc.str() + " becomes 0 if equal"), SectionType::text);
                    
                    if (IS_REG(rhsloc)) {
                        returnRegister(static_cast<Register>(rhsloc.reg));
                    }
                    
                    return { resultloc, inverted ? Condition::nonzero : Condition::zero };
                }
                case TokenType::less: {
                    const Location resultloc = emitExpression(left);
                    const Location rhsloc = emitExpression(right);
                    
                    if (resultloc.isLiteral && rhsloc.isLiteral) {
                        const Register r = GET_REG(currentFrame());
                        if (resultloc.value.s < rhsloc.value.s) {
                            emit(new XorOperation(RegisterLocation(r), RegisterLocation(r)), SectionType::text);
                        } else {
                            emit(new MoveOperation(RegisterLocation(r), NumLL(false, SU(1ULL)), SizeType::qword), SectionType::text);
                        }
                        return { RegisterLocation(r), inverted ? Condition::nonzero : Condition::zero };
                    }
                    
                    emit(new CmpOperation(resultloc, rhsloc), SectionType::text);
                    
                    if (IS_REG(resultloc)) {
                        returnRegister(static_cast<Register>(resultloc.reg));
                    }
                    if (IS_REG(rhsloc)) {
                        returnRegister(static_cast<Register>(rhsloc.reg));
                    }
                    
                    if (justFlags) {
                        return { RelLabelL("N/A"), inverted ? Condition::greaterEqual : Condition::less };
                    }
                    
                    const Register r = GET_REG(currentFrame());
                    emit(new MoveOperation(RegisterLocation(r), ZeroLL, SizeType::qword), SectionType::text);
                    const Register r8bit = static_cast<Register>(static_cast<int>(r) + 32);
                    emit(new RawText(INDENT "setl " + RegisterLocation(r8bit).str()), SectionType::text);
                    
                    return { RegisterLocation(r), inverted ? Condition::greaterEqual : Condition::less };
                }
                default:
                    break;
            }
        } else if (auto literal = dynamic_cast<Literal*>(expr)) {
            switch (literal->type()) {
                case Literal::LType::boolean: {
                    const Register r = GET_REG(currentFrame());
                    if (literal->value().type == TokenType::boolTrue) {
                        emit(new XorOperation(RegisterLocation(r), RegisterLocation(r)), SectionType::text);
                    } else {
                        emit(new MoveOperation(RegisterLocation(r), NumLL(false, SU(1ULL)), SizeType::qword), SectionType::text);
                    }
                    emit(new CmpOperation(RegisterLocation(r), FalseLL), SectionType::text);
                    returnRegister(r);
                    if (literal->value().type == TokenType::boolTrue) {
                        return { TrueLL, inverted ? Condition::zero : Condition::nonzero };
                    } else {
                        return { FalseLL, inverted ? Condition::nonzero : Condition::zero };
                    }
                }
                default:
                    break;
            }
        } else if (auto symbol = dynamic_cast<SymbolExpression*>(expr)) {
            // MARK: literally looks for defined stuff in this frame will fix later
            const auto result = lookup(symbol->value().contents); // PROBLEM: only valid in reference to that frame. Add member to struct variable called TOTAL OFFSET or ABSOLUTE OFFSET which contains the offset FROM THE START and hence make the calculation of the offset from the current rbp possible
            if (!result.second) {
                assert(false && "Static analyzer should catch this");
            }
            auto loc = result.first.loc;
            const Register resultr = static_cast<Register>(frames.back().avaliableScratch());
            emit(new MoveOperation(RegisterLocation(resultr), loc, OPSIZE_FROM_NUM(result.first.size), "store " + result.first.name + " in " + registerNames[static_cast<int>(resultr)]), SectionType::text);
            emit(new CmpOperation(RegisterLocation(resultr), NumLL(false, SU(0ULL))), SectionType::text);
            return { RegisterLocation(resultr), inverted ? Condition::zero : Condition::nonzero };
        }
        assert(false && "Unimplemented condition");
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
        }
    }

    // MARK: Load arguments pre-call
    std::pair<long long, std::vector<Register>> Compiler::emitCallArguments(const std::vector<Expression*>& args) {
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
                    if (!arg)
                        continue;
                    const Location result = emitExpression(arg);
                    if (result.reg != static_cast<int>(argreg)) emit(new MoveOperation(RegisterLocation(argreg), result, SizeType::qword, "argument " + std::to_string(index)), SectionType::text);
                    if (IS_REG(result)) returnRegister(static_cast<Register>(result.reg));
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
    Location Compiler::emitCall(Call *call, bool isTailCall) {
        const FunctionSignature funsig {call->name.contents, call->_spa_params};
        
        if (isTailCall) {
            emitLeave();
            emitCallArguments(call->args); // load the arguments
            emit(new JumpOperation(JumpOperation::JType::normal, analyzer.strFromFunctionSignature(funsig), call->generateTypeDescription() + " && tail call optimization"), SectionType::text);
            return RETURN_VALUE_LOC;
        }
        
        std::vector<Register> registersInUse;
        if (!frames.empty()) registersInUse = currentFrame().registersInUse;
        if (!registersInUse.empty() && (registersInUse.size() & 1)) registersInUse.push_back(registersInUse.back());
        emitSaveRegisters(registersInUse); // push all registers in use onto the stack, as the callee may modify them
        
        auto loadresult {emitCallArguments(call->args)}; // load the arguments
        const long long stackArgC = loadresult.first;
        
        emit(new CallOperation(analyzer.strFromFunctionSignature(funsig), call->generateTypeDescription()), SectionType::text); // call the function
        
        bool savedrax = false;
        for (size_t i = 0; i < registersInUse.size(); i++) {
            if (registersInUse[i] == Register::rax) {
                savedrax = true;
            }
        }
        if (stackArgC) emit(new AddOperation(RegisterLocation(Register::rsp), NumLL(false, SU(stackArgC * 8)), SizeType::qword, "remove args"), SectionType::text); // remove arguments
        
        Location r = RETURN_VALUE_LOC;
        
        emitRestoreRegisters(registersInUse); // restore all the saved registers to original state
        
        if (savedrax) {
            const Register result = static_cast<Register>(currentFrame().avaliableScratch());
            if (!(RegisterLocation(result) == RETURN_VALUE_LOC)) {
                emit(new MoveOperation(RegisterLocation(result), RETURN_VALUE_LOC, SizeType::qword, "save return value"), SectionType::text);
                r = RegisterLocation(result);
            }
        }
        
        for (auto reg: registersInUse) {
            emit(new PopOperation(RegisterLocation(reg)), SectionType::text);
        }
                
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
                         "\nextern _init_floral ; initialization procedure\n"
                         "global _main ; _main is the entry point in macOS nasm\n" // make entry point visible to linker
                         "_main:" // _main is the entry point in macOS nasm
                         ), SectionType::text);
        
        const std::string nameOfMain = analyzer.strFromFunctionSignature({main->name().contents, main->parameters()});
        if (!(nameOfMain == "main" || nameOfMain == "main_i32_u")) {
            report(Error::resolutionDomain, "Cannot find function main(Int32, &&Char)", main->_loc.path, main->_loc, { main->_name.pos(), main->_name.contents.size() });
        }
        
        emit(new SubOperation(RegisterLocation(Register::rsp), NumLL(false, SU(8LLU)), "@ so stack is aligned upon calls"), SectionType::text); // align stack to 16 bytes
        emit(new RawText(INDENT "call _init_floral ; @ call initialization procedure"), SectionType::text);
        emit(new CallOperation(nameOfMain, "@ call the floral main function"), SectionType::text); // call the Floral main function
        emit(new AddOperation(RegisterLocation(Register::rsp), NumLL(false, SU(8LLU)), SizeType::qword, "@ restore stack pointer"), SectionType::text); // restore stack pointer
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
        for (auto riter = textSection.instructions.rbegin(); riter != textSection.instructions.rend(); riter++) {
            if (auto extern_ = dynamic_cast<Extern*>(*riter)) {
                int refcount {};
                for (auto instr: textSection.instructions) {
                    if (auto call = dynamic_cast<CallOperation*>(instr)) {
                        if (call->lbl == extern_->lbl) {
                            refcount++;
                        }
                    } else if (auto jmp = dynamic_cast<JumpOperation*>(instr)) {
                        if (jmp->lbl == extern_->lbl) {
                            refcount++;
                        }
                    } else if (auto lea = dynamic_cast<LoadAddressOperation*>(instr)) {
                        if (lea->src.lbl == extern_->lbl) {
                            refcount++;
                        }
                    } else if (auto mov = dynamic_cast<MoveOperation*>(instr)) {
                        if (mov->src.lbl == extern_->lbl) {
                            refcount++;
                        }
                        if (mov->dest.lbl == extern_->lbl) {
                            refcount++;
                        }
                    }
                }
                if (!refcount) {
                    textSection.instructions.erase(--(riter.base()));
                }
            }
        }
        if (passes > 0) {            
            while (
                optimizeMatch2(textSection.instructions.size())
            ) {
                optimizeMatch3(textSection.instructions.size());
            }
            optimizeMatch3(textSection.instructions.size());
            while (
                optimizeMatch2(textSection.instructions.size())
            ) {
                optimizeMatch3(textSection.instructions.size());
            }
            optimizeMatch1(textSection.instructions.size());
            optimizeMatch2(textSection.instructions.size());
        }
        if (passes > 1) {
            //optimizeOutRedundancy(textSection.instructions.size());
        }
    }

    // MARK: Match patterns one long
    void Compiler::optimizeMatch1(size_t instrc) {
        for (auto iter = textSection.instructions.rbegin(); iter != textSection.instructions.rend(); iter++) {
            if (auto mov = dynamic_cast<MoveOperation*>(*iter)) {
                if (IS_REG(mov->dest) && mov->dest.reg < 8 && mov->src.isLiteral && mov->src.value.u < 0b100000000) {
                    mov->dest.reg += 8;
                } else if (mov->src == mov->dest) {
                    textSection.instructions.erase(--(iter.base()));
                }
            } else if (auto xor_ = dynamic_cast<XorOperation*>(*iter)) {
                if (IS_REG(xor_->src) && xor_->src == xor_->dest) {
                    int regint = xor_->src.reg;
                    if (regint < 8) {
                        regint += 8;
                    }
                    xor_->src.reg = regint;
                    xor_->dest.reg = regint;
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
                        } else if (amov->dest.reg == bmov->dest.reg && IS_REG(amov->dest) && IS_REG(bmov->dest) && bmov->dest.isDereference && (bmov->src.isLiteral || IS_REG(bmov->src)) && IS_REG(amov->src)) {
                            bmov->dest = amov->src;
                            bmov->dest.isDereference = true;
                            INSRT_COMMENTS(bmov, amov);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        } else if (amov->dest == bmov->dest && amov->dest.reg != bmov->src.reg && IS_REG(amov->dest)) {
                            INSRT_COMMENTS(bmov, amov);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        }
                        else if (amov->dest == bmov->src && !amov->src.isDereference && !bmov->dest.isDereference) {
                            bmov->src = amov->src;
                            bmov->comment += " (with " + amov->dest.str() + " = " + amov->src.str() + ')';
                        }
                    }
                    else if (auto badd = dynamic_cast<AddOperation*>(b)) {
                        if (amov->dest == badd->src && IS_REG(amov->dest)) {
                            badd->src = amov->src;
                            INSRT_COMMENTS(badd, amov);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        } else if (IS_REG(amov->dest) && badd->dest == amov->dest && ARE_BOTH_LIT(amov, badd)) {
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
                    else if (auto bcmp = dynamic_cast<CmpOperation*>(b)) {
                        if (IS_REG(amov->dest) && amov->dest == bcmp->dest && !bcmp->src.isDereference && IS_RBPOFFSET(amov->src)) {
                            bcmp->dest = amov->src;
                            INSRT_COMMENTS(bcmp, amov);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        }
                    }
                } else if (auto alea = dynamic_cast<LoadAddressOperation*>(a)) {
                    if (auto bmov = dynamic_cast<MoveOperation*>(b)) {
                        if (alea->dest == bmov->src && IS_REG(alea->dest) && IS_REG(bmov->dest)) {
                            alea->dest = bmov->dest;
                            CAT_COMMENTS(alea, bmov);
                            textSection.instructions.erase(textSection.instructions.begin() + i + 1);
                            i--;
                        } else if (IS_REG(alea->dest) && bmov->dest.reg == alea->dest.reg && !bmov->dest.isDereference && bmov->dest.isDereference && !bmov->src.isDereference) {
                            bmov->dest = alea->src;
                            INSRT_COMMENTS(bmov, alea);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        } else if (alea->dest == bmov->dest && bmov->dest.reg == bmov->src.reg && IS_REG(alea->dest)) {
                            INSRT_COMMENTS(bmov, alea);
                            bmov->src = alea->src;
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        } else if (alea->dest.reg == bmov->dest.reg && IS_REG(alea->dest) && bmov->dest.isDereference && !bmov->src.isDereference) {
                            bmov->dest = alea->src;
                            INSRT_COMMENTS(bmov, alea);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i--;
                        }
                    } else if (auto bsub = dynamic_cast<SubOperation*>(b)) {
                        if (bsub->dest == alea->dest && bsub->src.isLiteral && IS_RBPOFFSET(alea->src)) {
                            if (bsub->src.isSigned) {
                                alea->src.offset -= bsub->src.value.s;
                            } else {
                                alea->src.offset -= (long long)bsub->src.value.u;
                            }
                            CAT_COMMENTS(alea, bsub);
                            textSection.instructions.erase(textSection.instructions.begin() + i + 1);
                            i--;
                        }
                    } else if (auto blea = dynamic_cast<LoadAddressOperation*>(b)) {
                        if (alea->dest.reg == blea->src.reg && IS_REG(alea->dest) && IS_REG(blea->dest)) {
                            alea->dest = blea->dest;
                            CAT_COMMENTS(alea, blea);
                            textSection.instructions.erase(textSection.instructions.begin() + i + 1);
                            i--;
                        }
                    }
                } else if (auto apush = dynamic_cast<PushOperation*>(a)) {
                    if (auto bpop = dynamic_cast<PopOperation*>(b)) {
                        if (apush->src == bpop->dest) {
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            textSection.instructions.erase(textSection.instructions.begin() + i);
                            i -= 2;
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
                            } else if (IS_RBPOFFSET(amov->src) && IS_REG(amov->dest) && badd->dest == amov->dest && !badd->src.isDereference && cmov->dest == amov->src && cmov->src == badd->dest) {
                                badd->dest = amov->src;
                                badd->opsize = amov->opsize;
                                CAT_COMMENTS(amov, badd);
                                INSRT_COMMENTS(badd, amov);
                                textSection.instructions.erase(textSection.instructions.begin() + i);
                                textSection.instructions.erase(textSection.instructions.begin() + i + 1);
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
                    } else if (auto bmov = dynamic_cast<MoveOperation*>(b)) {
                        if (auto cmul = dynamic_cast<MulOperation*>(c)) {
                            if (amov->src == bmov->src && cmul->src == amov->dest && cmul->dest == bmov->dest && IS_REG(amov->src)) {
                                textSection.instructions.insert(
                                    textSection.instructions.begin() + i + 3,
                                    new MoveOperation(bmov->dest, amov->src, SizeType::qword)
                                );
                                cmul->src = amov->src;
                                cmul->dest = amov->src;
                                textSection.instructions.erase(textSection.instructions.begin() + i);
                                textSection.instructions.erase(textSection.instructions.begin() + i);
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
        
        ColoredStream out(std::cerr);
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
        
        if (_stackGuard) {
            emit(new Extern("stkgrd_fail_u_u"), SectionType::text);
        }
        
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
                       node->_loc.path,
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
        
        optimize(optimization);

        Floral::write(outputDest, result());
    }
    const std::string Compiler::result() const {
        // join all sections into one string
        std::string joined = textSection.str() + '\n';
        if (!bssSection.str().empty()) {
            joined.push_back('\n');
            joined += bssSection.str();
            joined.push_back('\n');
        }
        if (!rodataSection.str().empty()) {
            joined.push_back('\n');
            joined += rodataSection.str();
            joined.push_back('\n');
        }
        if (!dataSection.str().empty()) {
            joined.push_back('\n');
            joined += dataSection.str();
            joined.push_back('\n');
        }
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
}
