//
//  Type.cpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Type.hpp"
#include <cassert>
#include <numeric>

namespace Floral {
    Type::Type(Token* value, bool isConst): _tknValue(value), _arrType(nullptr), _ptrType(nullptr), _tupleType{}, _functionType{}, _isConst(isConst) {}
    Type::Type(Type* value, bool isPtr, bool isConst): _tknValue(nullptr), _arrType(isPtr ? nullptr : value), _ptrType(isPtr ? value : nullptr), _tupleType{}, _functionType{}, _isConst(isConst) {}
    Type::Type(Type* tuple[MAX_TUPLE_SIZE], const size_t size, bool isConst): _tknValue(nullptr), _arrType(nullptr), _ptrType(nullptr), _functionType{}, _isConst(isConst) {
        for (size_t i {}; i < size; ++i)
            _tupleType[i] = tuple[i];
        _tupleLen = size;
    }
    Type::Type(Type* params, Type* ret, bool isConst): _tknValue(nullptr), _arrType(nullptr), _ptrType(nullptr), _tupleType{}, _functionType{ params, ret }, _isFunctionType(true), _isConst(isConst) {}
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
        std::cout << des();
    }
    const std::string Type::des() const {
        std::string str;
        if (_isConst) str += "const ";
        if (isToken())
            str += _tknValue->contents;
        else if (isArray()) {
            str.push_back('[');
            str += _arrType->des();
            str.push_back(']');
        } else if (isPointer()) {
            str.push_back('&');
            if (_ptrType->isConst()) {
                str += '(' + _ptrType->des() + ')';
            } else {
                str += _ptrType->des();
            }
        } else if (isTuple()) {
            std::cout << '(';
            for (size_t i {}; i < _tupleLen; ++i) {
                str += _tupleType[i]->des();
                if (i + 1 < _tupleLen) str += ", ";
            }
            std::cout << ')';
        } else if (isFunction()) {
            str += _functionType[0]->des();
            str += " -> ";
            str += _functionType[1]->des();
        }
        return str;
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
    bool Type::isInt32() const {
        return _tknValue && _tknValue->type == TokenType::int32Type;
    }
    bool Type::isUInt32() const {
        return _tknValue && _tknValue->type == TokenType::uint32Type;
    }
    bool Type::isShort() const {
        return _tknValue && _tknValue->type == TokenType::shortType;
    }
    bool Type::isUShort() const {
        return _tknValue && _tknValue->type == TokenType::ushortType;
    }
    bool Type::isChar() const {
        return _tknValue && _tknValue->type == TokenType::charType;
    }
    bool Type::isUChar() const {
        return _tknValue && _tknValue->type == TokenType::ucharType;
    }
    bool Type::isWideChar() const {
        return _tknValue && _tknValue->type == TokenType::wideCharType;
    }
    bool Type::isWideUChar() const {
        return _tknValue && _tknValue->type == TokenType::wideUCharType;
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
    bool Type::isCString() const {
        return _ptrType && _ptrType->isChar();
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

    bool Type::isString() const {
        return _tknValue && _tknValue->type == TokenType::stringType;
    }
    bool Type::isBool() const {
        return _tknValue && _tknValue->type == TokenType::boolType;
    }

    bool Type::isNumber() const {
        return _tknValue && (
            _tknValue->type == TokenType::int64Type ||
            _tknValue->type == TokenType::uint64Type ||
            _tknValue->type == TokenType::uint64Type ||
            _tknValue->type == TokenType::int32Type ||
            _tknValue->type == TokenType::uint32Type ||
            _tknValue->type == TokenType::shortType ||
            _tknValue->type == TokenType::ushortType ||
            _tknValue->type == TokenType::charType ||
            _tknValue->type == TokenType::ucharType ||
            _tknValue->type == TokenType::wideCharType ||
            _tknValue->type == TokenType::wideUCharType // || float types...
        );
    }

    bool Type::isInteger() const {
        return isPointer() || (_tknValue && (
            _tknValue->type == TokenType::int64Type ||
            _tknValue->type == TokenType::uint64Type ||
            _tknValue->type == TokenType::uint64Type ||
            _tknValue->type == TokenType::int32Type ||
            _tknValue->type == TokenType::uint32Type ||
            _tknValue->type == TokenType::shortType ||
            _tknValue->type == TokenType::ushortType ||
            _tknValue->type == TokenType::charType ||
            _tknValue->type == TokenType::ucharType ||
            _tknValue->type == TokenType::wideCharType ||
            _tknValue->type == TokenType::wideUCharType)
        );
    }

    bool Type::isConst() const {
        return _isConst;
    }

    size_t Type::size() const {
        if (isPointer() || isFunction()) return 8;
        if (isString()) return 24;
        if (isArray()) return 24;
        if (isBool()) return 1;
        if (isChar() || isUChar()) return 1;
        if (isShort() || isUShort()) return 2;
        if (isInt32() || isUInt32() || isWideChar() || isWideUChar()) return 4;
        if (isInt() || isUInt()) return 8;
        if (isVoid()) return 0;
        if (isTuple()) return std::reduce(_tupleType, _tupleType + (_tupleLen - 1), 0UL, [](unsigned long lhs, Type* rhs) -> unsigned long {
            return lhs + rhs->size();
        });
        assert(false && "Not implemented");
    }
    size_t Type::alignment() const {
        if (isPointer() || isFunction()) return 8;
        if (isString()) return 24;
        if (isArray()) return 24;
        if (isBool()) return 8;
        if (isChar() || isUChar()) return 8;
        if (isShort() || isUShort()) return 8;
        if (isInt32() || isUInt32() || isWideChar() || isWideUChar()) return 8;
        if (isInt() || isUInt()) return 8;
        if (isVoid()) return 0;
        assert(false && "Not implemented");
    }

    bool Type::canBeImplicitlyUnconst() const {
        return isNumber() || isBool();
    }

    bool operator ==(const Type& lhs, const Type& rhs) {
        if (lhs.isNumber() && rhs.isNumber()) // MARK: cheap and dirty solution to passing int literal to uints and the like
            return true;
        if (lhs.isToken() && rhs.isToken())
            return *lhs._tknValue == *rhs._tknValue;
        if (lhs.isPointer() && rhs.isPointer())
            return lhs._ptrType->isVoid() || rhs._ptrType->isVoid() || *lhs._ptrType == *rhs._ptrType;
        return false;
    }

    const std::string Type::shortID() const {
        if (isString()) return "str";
        if (isBool()) return "b";
        if (isInt()) return "i";
        if (isUInt() || isPointer()) return "u";
        if (isInt32()) return "i32";
        if (isUInt32()) return "u32";
        if (isShort()) return "i16";
        if (isUShort()) return "u16";
        if (isChar()) return "ch";
        if (isUChar()) return "uch";
        if (isWideChar()) return "wch";
        if (isWideUChar()) return "wuch";
        
        if (isPointer()) return _ptrType->shortID() + "ptr";
        if (isArray()) return _arrType->shortID() + "arr";
        if (isFunction()) return _functionType[0]->shortID() + "to" + _functionType[1]->shortID() + "fptr";
        
        return "";
    }

    void Type::_setConst(bool isConst) {
        _isConst = isConst;
    }

}
//bool isString() const;
//bool isBool() const;
//bool isInt() const;
//bool isUInt() const;
//bool isInt32() const;
//bool isUInt32() const;
//bool isShort() const;
//bool isUShort() const;
//bool isChar() const;
//bool isUChar() const;
//bool isVoid() const;
//
//bool isToken() const;
//bool isPointer() const;
//bool isFunction() const;
//bool isArray() const;
//bool isTuple() const;
//
//bool isNumber() const;
//bool isInteger() const;
