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
            
            void statement(Statement* stm);
            void let(LetStatement* l);
            void var(VarStatement* v);
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
            void report(Error::Domain domain, const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "");
            void warn(const std::string& text, TextRegion loc, ErrorLoc errloc, const std::string& fix = "");
            std::string _src;
            
            std::string outputDest;
            Section textSection;
            Section dataSection;
            Section rodataSection;
            Section bssSection;
            
            std::vector<Frame> frames; // a stack of frames
            
            StaticAnalyzer analyzer; // the static analyzer

            size_t _stackFromMain {};
            void _processPotentialStackOperation(Instruction* instr);
            
            void emit(Instruction* instr, SectionType section);
            
            void reset();
            std::string staticEvalulate(Expression* staticEvalExpr);
            
            
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
            
            // Statement related
            void emitStatement(Statement* stm);
            void emitLocalConst(LetStatement* l);
            void emitLocalVar(VarStatement* v);
            void emitCallStatement(CallStatement* callStm);
            void emitReturnStatement(ReturnStatement* rtnStm);
            void emitExpressionStatement(ExpressionStatement* exprStm);
            void emitAssignmentStatement(Assignment* assignStm);
            void emitPointerAssignmentStatement(PointerAssignment* ptrAssign);
            void emitIfStatement(IfStatement* ifStm);
            void emitWhileStatement(WhileStatement* whileStm);
            void emitForStatement(ForStatement* forStm);
            void emitBlock(Block* block);
            
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
            Location emitExpression(Expression* expr, bool wantsAddressResult = false);
            std::pair<long long, std::vector<Register>> emitCallArguments(const std::vector<Expression*>& args);
            Location emitCall(Call* call);
            
            // Optimization
            void optimizeMatch1(size_t instrc);
            bool optimizeMatch2(size_t instrc);
            void optimizeMatch3(size_t instrc);
            void optimizeOutRedundancy(size_t instrc);
            void optimize(int passes);
            
            // Other
            std::string _path;
            bool _showTypeTrace = false;
            bool _wasRegisterParameter = false;
            void _strprocess(std::string& str);
            
        public:
            Compiler();
            ~Compiler();
            
            void generateEntryPoint(Function* main);

            int optimization;
            int usingCFunctions;
            int notUsingStdlib;
            
            void setOutputDestination(const std::string &dest);
            void showTypeTrace(bool show);
            bool hasErrors() const;
            const std::vector<Error>& errors() const;
            bool hasWarnings() const;
            const std::vector<Error>& warnings() const;
            
            void compile(const File *file);
            const std::string result() const;
            
            void setSource(const std::string& src);
            
            void _debugInsert(Instruction* instr);
            void setPath(const std::string& path);
        };
    }
}

#endif /* Compiler_hpp */
