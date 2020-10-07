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
#include "Token.hpp"

#define MAX_TUPLE_SIZE 64

#define ZERO_INIT_CHAR "resb 1"
#define ZERO_INIT_SHORT "resw 1"
#define ZERO_INIT_INT32 "resd 1"
#define ZERO_INIT_INT64 "resq 1"

namespace Floral {
    namespace v2 { class Compiler; }
    class StaticAnalyzer;
    class Type {
        friend class StaticAnalyzer;
        friend class v2::Compiler;
        friend class Operator;
        
        Token* _tknValue;
        Type* _ptrType;
        Type* _arrType;
        Type* _functionType[2];
        bool _isFunctionType {};
        bool _isConst;
        size_t _tupleLen {};
        Type* _tupleType[MAX_TUPLE_SIZE];
        
    public:
        Type(Token* value, bool isConst = false);                     // single type literal
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
        bool isString(void) const;
        bool isBool(void) const;
        bool isInt(void) const;
        bool isUInt(void) const;
        bool isInt32(void) const;
        bool isUInt32(void) const;
        bool isShort(void) const;
        bool isUShort(void) const;
        bool isChar(void) const;
        bool isUChar(void) const;
        bool isVoid(void) const;
        
        bool isToken(void) const;
        bool isPointer(void) const;
        bool isFunction(void) const;
        bool isArray(void) const;
        bool isTuple(void) const;
        
        bool isNumber(void) const;
        bool isInteger(void) const;
        
        bool isConst(void) const;
        
        bool canBeImplicitlyUnconst(void) const;
        
        size_t size(void) const;
        size_t alignment(void) const;
        
        const std::string shortID(void) const;
        
        void _setConst(bool isConst);
    };
}

#endif /* Type_hpp */
