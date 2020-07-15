//
//  Type.cpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Type.hpp"

namespace Floral {
    Type::Type(Token* value): _tknValue(value), _arrType(nullptr), _ptrType(nullptr), _tupleType{}, _functionType{} {}
    Type::Type(Type* value, bool isPtr): _tknValue(nullptr), _arrType(isPtr ? nullptr : value), _ptrType(isPtr ? value : nullptr), _tupleType{}, _functionType{} {}
    Type::Type(Type* tuple[MAX_TUPLE_SIZE], const size_t size): _tknValue(nullptr), _arrType(nullptr), _ptrType(nullptr), _functionType{} {
        for (size_t i {}; i < size; ++i)
            _tupleType[i] = tuple[i];
        _tupleLen = size;
    }
    Type::Type(Type* params, Type* ret): _tknValue(nullptr), _arrType(nullptr), _ptrType(nullptr), _tupleType{}, _functionType{ params, ret }, _isFunctionType(true) {}
    Type::~Type() {
        if (isToken())
            delete _tknValue;
        else if (isArray())
            delete _arrType;
        else if (isPointer())
            delete _ptrType;
        else if (isTuple())
            for (size_t i {}; i < _tupleLen; ++i)
                delete _tupleType[i];
        else if (isFunction())
            static_cast<void>(delete _functionType[0]), delete _functionType[1];
    }

    const Type Type::void_ { new Token({0, 0}, TokenType::invalid, "") };
    const void* Type::value() const {
        if (isArray()) return _arrType;
        if (isPointer()) return _ptrType;
        if (isTuple()) return _tupleType;
        return _tknValue;
    }
    void Type::print() const {
        if (isToken())
            std::cout << _tknValue->contents;
        else if (isArray()) {
            std::cout << '[';
            _arrType->print();
            std::cout << ']';
        } else if (isPointer()) {
            std::cout << '&';
            _ptrType->print();
        } else if (isTuple()) {
            std::cout << '(';
            for (size_t i {}; i < _tupleLen; ++i)
                _tupleType[i]->print();
            std::cout << ')';
        } else if (isFunction()) {
            _functionType[0]->print();
            std::cout << ": ";
            _functionType[1]->print();
        }
    }

    bool Type::isIncomplete() const {
        return _tknValue && _tknValue->type == TokenType::invalid;
    }
    bool Type::isInt() const {
        return _tknValue && _tknValue->type == TokenType::int64Type;
    }
    bool Type::isUInt() const {
        return _tknValue && _tknValue->type == TokenType::uint64Type;
    }
    bool Type::isVoid() const {
        return _tknValue && _tknValue->type == TokenType::voidType;
    }

    bool Type::isToken() const {
        return _tknValue;
    }
    bool Type::isPointer() const {
        return _ptrType;
    }
    bool Type::isFunction() const {
        return _isFunctionType;
    }
    bool Type::isArray() const {
        return _arrType;
    }
    bool Type::isTuple() const {
        return _tupleLen > 0;
    }
}
