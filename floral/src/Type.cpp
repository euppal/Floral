//
//  Type.cpp
//  floral
//
//  Created by Ethan Uppal on 7/13/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Type.hpp"
#include "AST.hpp"
#include <cassert>
#include <numeric>

namespace Floral {
    Type::Type(Token* value, bool isConst): _tknValue(value), _stdlib_arrType(nullptr), _structValue(nullptr), _ptrType(nullptr), _tupleType{}, _functionType{}, _isConst(isConst) {}
    Type::Type(int, const std::string& name, bool isConst): _tknValue(nullptr), _stdlib_arrType(nullptr), _ptrType(nullptr), _tupleType{}, _functionType{}, _isConst(isConst) {
        auto iter = std::find_if(structs.begin(), structs.end(), [name](StructDeclaration* struct_) -> bool {
            return struct_->name().contents == name;
        });
        if (iter != structs.end()) {
            _structValue = *iter;
        }
    }
    Type::Type(Type* value, bool isPtr, bool isConst): _tknValue(nullptr), _stdlib_arrType(isPtr ? nullptr : value), _structValue(nullptr), _ptrType(isPtr ? value : nullptr), _tupleType{}, _functionType{}, _isConst(isConst) {}
    Type::Type(Type* elementType, size_t length, bool isConst): _tknValue(nullptr), _stdlib_arrType(nullptr), _structValue(nullptr), _staticArray({elementType, length}), _ptrType(nullptr), _tupleType{}, _functionType{}, _isConst(isConst) {}
    Type::Type(Type* tuple[MAX_TUPLE_SIZE], const size_t size, bool isConst): _tknValue(nullptr), _stdlib_arrType(nullptr), _structValue(nullptr), _ptrType(nullptr), _functionType{}, _isConst(isConst) {
        for (size_t i {}; i < size; ++i)
            _tupleType[i] = tuple[i];
        _tupleLen = size;
    }
    Type::Type(Type* params, Type* ret, bool isConst): _tknValue(nullptr), _stdlib_arrType(nullptr), _structValue(nullptr), _ptrType(nullptr), _tupleType{}, _functionType{ params, ret }, _isFunctionType(true), _isConst(isConst) {}
    Type::~Type() {
        if (isToken()) {
            dealloc(_tknValue);
        } else if (isArray()) {
            dealloc(_staticArray->first);
        } else if (isStdArray()) {
            dealloc(_stdlib_arrType);
        } else if (isPointer()) {
            dealloc(_ptrType);
        }  else if (isTuple()) {
            for (size_t i {}; i < _tupleLen; ++i)
                dealloc(_tupleType[i]);
        } else if (isFunction()) {
            dealloc(_functionType[0]);
            dealloc(_functionType[1]);
        }
    }

    const Type Type::void_ { new Token({0, 0, 0, ""}, TokenType::voidType, "") };
    const void* Type::value() const {
        if (isStdArray()) return _stdlib_arrType;
        if (isPointer()) return _ptrType;
        if (isTuple()) return _tupleType;
        return _tknValue;
    }
    void Type::print() const {
        std::cout << des();
    }
    const std::string Type::des() const {
        std::string str;
        if (_isConst && !_staticArray) str += "const ";
        if (isToken()) {
            str += _tknValue->contents;
        } else if (isStruct()) {
            str += "struct " + _structValue->name().contents;
        } else if (isStdArray()) {
            str.push_back('[');
            str += _stdlib_arrType->des();
            str.push_back(']');
        }  else if (isArray()) {
            str += _staticArray->first->des();
            str.push_back('[');
            str += std::to_string(_staticArray->second);
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
    bool Type::isStruct() const {
        return _structValue;
    }
    StructDeclaration* Type::structValue() {
        return _structValue;
    }
    bool Type::isPointer() const {
        return _ptrType || isArray();
    }
    bool Type::isCString() const {
        return _ptrType && _ptrType->isChar();
    }
    bool Type::isFunction() const {
        return _isFunctionType;
    }
    bool Type::isArray() const {
        return _staticArray != std::nullopt;
    }
    bool Type::isStdArray() const {
        return _stdlib_arrType;
    }
    bool Type::isTuple() const {
        return _tupleLen > 0;
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
        return _tknValue && (
            _tknValue->type == TokenType::int64Type ||
            _tknValue->type == TokenType::uint64Type ||
            _tknValue->type == TokenType::int32Type ||
            _tknValue->type == TokenType::uint32Type ||
            _tknValue->type == TokenType::shortType ||
            _tknValue->type == TokenType::ushortType ||
            _tknValue->type == TokenType::charType ||
            _tknValue->type == TokenType::ucharType ||
            _tknValue->type == TokenType::wideCharType ||
            _tknValue->type == TokenType::wideUCharType
        );
    }

    bool Type::isSigned() const {
        return _tknValue && (
            _tknValue->type == TokenType::int64Type ||
            _tknValue->type == TokenType::int32Type ||
            _tknValue->type == TokenType::shortType ||
            _tknValue->type == TokenType::charType ||
            _tknValue->type == TokenType::wideCharType
        );
    }

    bool Type::isConst() const {
        return _isConst;
    }

    size_t Type::size() const {
        if (isStruct()) {
            size_t s {};
            auto dataMembers = _structValue->dataMembers();
            for (auto &dataMember: dataMembers) {
                if (auto var = dynamic_cast<VarStatement*>(dataMember)) {
                    s += (var->type()->size() + 7) & -8;
                }
            }
            return s;
        }
        if (isArray()) return _staticArray->first->size() * _staticArray->second;
        if (isPointer() || isFunction()) return 8;
        if (isStdArray()) return 24;
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
        if (isStruct() || isArray())
            return (size() + 7) & -8;
        if (isPointer() || isFunction()) return 8;
        if (isStdArray()) return 24;
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
        if (lhs.isToken() && rhs.isToken())
            return lhs._tknValue->type == rhs._tknValue->type;
        if (lhs.isStruct() && rhs.isStruct())
            return lhs._structValue == rhs._structValue;
        if (lhs.isPointer() && rhs.isPointer()) {
            if (lhs.isArray()) {
                if (rhs.isArray()) {
                    return lhs._staticArray->first->isVoid() || rhs._staticArray->first->isVoid() || ((*lhs._staticArray->first == *rhs._staticArray->first) && (lhs._staticArray->second == rhs._staticArray->second));
                } else {
                    return lhs._staticArray->first->isVoid() || rhs._ptrType->isVoid() || (*lhs._staticArray->first == *rhs._ptrType);
                }
            } else {
                if (rhs.isArray()) {
                    return lhs._ptrType->isVoid() || rhs._staticArray->first->isVoid() || (*lhs._ptrType == *rhs._staticArray->first);
                } else {
                    return lhs._ptrType->isVoid() || rhs._ptrType->isVoid() || (*lhs._ptrType == *rhs._ptrType);
                }
            }
        }
        return false;
    }

    const std::string Type::shortID() const {
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
        
        if (isStruct()) return _structValue->name().contents + "struct";
//        if (isPointer()) return _ptrType->shortID() + "ptr";
        if (isStdArray()) return _stdlib_arrType->shortID() + "arr";
        if (isFunction()) return _functionType[0]->shortID() + "to" + _functionType[1]->shortID() + "fptr";
        
        return "";
    }

    void Type::_setConst(bool isConst) {
        _isConst = isConst;
    }


    std::vector<StructDeclaration*> Type::structs;
    std::unordered_map<std::string, Type*> Type::typealiases;
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
