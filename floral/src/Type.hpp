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
    class Type {
        class StaticAnalyzer;
        friend class StaticAnalyzer;
        
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
        const void* value() const;
        bool isIncomplete() const;
        void print() const;
        const std::string des(void) const;
        
        // Helpers
        bool isString() const;
        bool isBool() const;
        bool isInt() const;
        bool isUInt() const;
        bool isInt32() const;
        bool isUInt32() const;
        bool isShort() const;
        bool isUShort() const;
        bool isChar() const;
        bool isUChar() const;
        bool isVoid() const;
        
        bool isToken() const;
        bool isPointer() const;
        bool isFunction() const;
        bool isArray() const;
        bool isTuple() const;
        
        bool isNumber() const;
        bool isInteger() const;
        
        bool isConst() const;
        
        bool canBeImplicitlyUnconst() const;
        
        size_t size() const;
        size_t alignment() const;
        
        const std::string shortID() const;
        
        void _setConst(bool isConst);
    };
}

#endif /* Type_hpp */
