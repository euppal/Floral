//
//  Type.hpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Type_hpp
#define Type_hpp

#define MAX_TUPLE_SIZE 64

#include "Token.hpp"

namespace Floral {
    class Type {
        Token* _tknValue;
        Type* _ptrType;
        Type* _arrType;
        Type* _functionType[2];
        bool _isFunctionType {};
        size_t _tupleLen {};
        Type* _tupleType[MAX_TUPLE_SIZE];
        
    public:
        Type(Token* value);                     // single type literal
        Type(Type* value, bool isPtr);          // array/ptr types
        Type(Type* tuple[MAX_TUPLE_SIZE], const size_t size); // tuple types
        Type(Type* params, Type* ret);          // function types
        ~Type();
        
        static const Type void_;
        const void* value() const;
        bool isIncomplete() const;
        void print() const;
        
        // Helpers
        bool isString() const;
        bool isBool() const;
        bool isInt() const;
        bool isUInt() const;
        bool isChar() const;
        bool isUChar() const;
        bool isVoid() const;
        
        bool isToken() const;
        bool isPointer() const;
        bool isFunction() const;
        bool isArray() const;
        bool isTuple() const;
    };

}

#endif /* Type_hpp */
