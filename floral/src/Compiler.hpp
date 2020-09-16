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
#include <string>
#include "SPA.hpp"
#include "Error.hpp"
#include <map>
#include <bitset>
#include "Frame.hpp"
#include "Instruction.hpp"

#define FLORAL_ID_PREFIX "_floralid_"
#define ALIGN_COMMENTS
#define max(x, y) ((x) > (y) ? (x) : (y))

namespace Floral {
    inline namespace v1 {
        class Compiler: public ErrorReporting {
            std::vector<Error> _errors;
            void report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc);
            
            std::string outputDest;
            std::string textSection;
            std::string dataSection;
            std::string rodataSection;
            std::string bssSection;
            
            std::vector<Frame> frames;
            
            std::unordered_map<std::string, GlobalDeclaration*> definedGlobalsTable;
            std::unordered_map<std::string, GlobalForwardDeclaration*> declaredGlobalsTable;
            
            StaticAnalyzer analyzer;
            
            void reset();
            const std::pair<std::string, bool> staticEvalulate(Expression* staticEvalExpr, bool wantsFinalSymbol = false);
            
            void mainFunction(Function* main);
            
            void declaration(Declaration* decl);
            void function(Function* func);
            void global(GlobalDeclaration* gbl);
            void forwardGlobal(GlobalForwardDeclaration* fgbl);
            void let(LetDeclaration* l);
            void var(VarDeclaration* v);
            
            void statement(Statement* stm);
            void callStm(CallStatement* callStm);
            void returnStm(ReturnStatement* rtnStm);
            
            void returnRegister(const std::string& reg);
            const std::string emitBinaryExpr(Expression* left, Expression* right, const std::string& opName, const std::string& opDes);
            void push(const std::vector<Register>& registers);
            void pop(const std::vector<Register>& registers);
            
            std::string expression(Expression* expr);
            size_t arguments(const std::vector<Expression*>& args);
            void call(Call* call);
            
        public:
            int optimization;
            int usingCFunctions;
            int notUsingStdlibHeader;
            
            void setOutputDestination(const std::string &dest);
            bool hasErrors() const;
            const std::vector<Error>& errors() const;
            
            void compile(const File *file);
        };
    }
    namespace v2 {
        class Compiler: public ErrorReporting {
            std::vector<Error> _errors;
            void report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "");
            std::string _src;
            
            std::string outputDest;
            Section textSection;
            Section dataSection;
            Section rodataSection;
            Section bssSection;
            
            std::vector<Frame> frames; // a stack of frames
            
            StaticAnalyzer analyzer; // the static analyzer

            void emit(Instruction* instr, SectionType section);
            
            void reset();
            const std::pair<std::string, bool> staticEvalulate(Expression* staticEvalExpr, bool wantsFinalSymbol = false);
            
            
            // Function related
            void emitFunction(Function* func);
            void emitExternFunc(FunctionForwardDeclaration* ffunc);
            void emitEnter(void);
            void emitLeave(void);
            void emitRet(void);
            void loadParametersIntoFrame(const Function::Parameters& params);

            // Declaration related
            void emitDeclaration(Declaration* decl);
            void emitGlobal(GlobalDeclaration* gbl);
            void emitExternGlobal(GlobalForwardDeclaration* fgbl);
            void emitLocalConst(LetDeclaration* l);
            void emitLocalVar(VarDeclaration* v);
            
            // Statement related
            void emitStatement(Statement* stm);
            void emitCallStatement(CallStatement* callStm);
            void emitReturnStatement(ReturnStatement* rtnStm);
            
            // Misc
            void emitReturnSpecificValue(Location src);
            Location emitBinaryExpr(Expression* left, Expression* right, const OpType op);
            void emitSaveRegisters(const std::vector<Register>& registers);
            void emitRestoreRegisters(const std::vector<Register>& registers);
            void enterFrame(void);
            void leaveFrame(void);
            Frame& currentFrame(void);
            void returnRegister(const Register r);
            std::pair<Variable, bool> lookup(const std::string& name);
            
            // Expression related
            Location emitExpression(Expression* expr);
            long long emitCallArguments(const std::vector<Expression*>& args);
            void emitCall(Call* call);
            
            void optimize();
            
        public:
            Compiler();
            ~Compiler();
            
            void generateEntryPoint(Function* main);

            int optimization;
            int usingCFunctions;
            int notUsingStdlibHeader;
            
            void setOutputDestination(const std::string &dest);
            bool hasErrors() const;
            const std::vector<Error>& errors() const;
            
            void compile(const File *file);
            const std::string result() const;
            
            void setSource(const std::string& src);
        };
    }
}

#endif /* Compiler_hpp */
