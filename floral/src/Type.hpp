//
//  Type.hpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Type_hpp
#define Type_hpp

#include <string>
#include <optional>
#include "Token.hpp"

#define dealloc(ptr) (delete (ptr), ptr = nullptr)

#define MAX_TUPLE_SIZE 64

#define ZERO_INIT_CHAR "resb 1"
#define ZERO_INIT_SHORT "resw 1"
#define ZERO_INIT_INT32 "resd 1"
#define ZERO_INIT_INT64 "resq 1"

#define GET_PTRTYYPE(type) ((type)->isArray() ? (type)->_staticArray->first : (type)->_ptrType)

namespace Floral {
    namespace v2 { class Compiler; }
    class StaticAnalyzer;
    class StructDeclaration;
    class Type {
        friend class StaticAnalyzer;
        friend class v2::Compiler;
        friend class Operator;
                
        Token* _tknValue;
        StructDeclaration* _structValue;
        std::optional<std::pair<Type*, size_t>> _staticArray;
        Type* _ptrType;
        Type* _stdlib_arrType;
        Type* _functionType[2];
        bool _isFunctionType {};
        bool _isConst;
        size_t _tupleLen {};
        Type* _tupleType[MAX_TUPLE_SIZE];
        
    public:
        static std::vector<StructDeclaration*> structs;
        static std::unordered_map<std::string, Type*> typealiases;

        Type(Token* value, bool isConst = false);                     // single type literal
        Type(int, const std::string& name, bool isConst = false);     // struct type
        Type(Type* elementType, size_t length, bool isConst = false); // static arr type
        Type(Type* value, bool isPtr, bool isConst = false);          // array/ptr types
        Type(Type* tuple[MAX_TUPLE_SIZE], const size_t size, bool isConst = false); // tuple types
        Type(Type* params, Type* ret, bool isConst = false);          // function types
        ~Type();
        
        friend bool operator ==(const Type& lhs, const Type& rhs);
        
        static const Type void_;
        const void* value(void) const;
        bool isIncomplete(void) const;
        void print(void) const;
        const std::string des(void) const;
        
        // Helpers
        bool isBool(void) const;
        bool isInt(void) const;
        bool isUInt(void) const;
        bool isInt32(void) const;
        bool isUInt32(void) const;
        bool isShort(void) const;
        bool isUShort(void) const;
        bool isChar(void) const;
        bool isUChar(void) const;
        bool isWideChar(void) const;
        bool isWideUChar(void) const;
        bool isVoid(void) const;
        
        bool isToken(void) const;
        bool isStruct(void) const;
        StructDeclaration* structValue();
        bool isPointer(void) const;
        bool isCString(void) const;
        bool isFunction(void) const;
        bool isArray(void) const;
        bool isStdArray(void) const;
        bool isTuple(void) const;
        
        bool isNumber(void) const;
        bool isInteger(void) const;
        bool isSigned(void) const;
        
        bool isConst(void) const;
        
        bool canBeImplicitlyUnconst(void) const;
        
        size_t size(void) const;
        size_t alignment(void) const;
        
        const std::string shortID(void) const;
        
        void _setConst(bool isConst);
    };
}

#endif /* Type_hpp */
