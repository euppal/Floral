//
//  Instruction.cpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 9/3/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Instruction.hpp"
#include "Frame.hpp"
#include <algorithm>
#include "Compiler.hpp"

namespace Floral {
    const std::string RawText::str() const {
        return rawText;
    }
    const std::string Section::str() const {
        if (instructions.empty()) return "";
        const static std::string sectionNames[] {
            "text", "data", "rodata", "bss"
        };
        return "section ." + sectionNames[static_cast<int>(type)] + "\n" + join(instructions, "\n", spaceOutLabels);
    }
    void Section::add(Instruction* instr) {
        instructions.push_back(instr);
    }
    const std::string Label::str() const {
        return (isGlobal ? ("global "  + prefixed(lbl) + '\n') : "") + prefixed(lbl) + ':';
    }
    const std::string Extern::str() const {
        return "extern " + prefixed(lbl) + '\n';
    }
    const std::string Global::str() const {
        return "global " + prefixed(lbl);
    }
    const std::string Data::str() const {
        const static std::string sizeTypeNames[] {
            "db", "dw", "dd", "dq"
        };
        
        return INDENT + prefixed(label) + ": " + sizeTypeNames[static_cast<int>(sizeType)] + join(values, isSigned, ", ");
    }
    const std::string ZeroData::str() const {
        const static std::string sizeTypeNames[] {
            "resb", "resw", "resd", "resq"
        };
        return INDENT + prefixed(label) + ": " + sizeTypeNames[static_cast<int>(sizeType)];
    }
    const std::string StringData::str() const {
        std::string_view view {'`' + contents};
        if (strncmp(view.data(), "``, ", 4) == 0) {
            view.remove_prefix(4);
        }
        std::string str {view};
        str.append("\\0`");
        return INDENT + prefixed(label) + ": db " + str;
    }
    const std::string LengthOf::str() const {
        return INDENT + prefixed(lbl) + ": equ $-" + src;
    }
    const std::string Location::str() const {
        if (isLiteral) return isSigned ? std::to_string(value.s) : std::to_string(value.u);
        else if (isLbl) return "[rel " + prefixed(lbl) + ']';
        const auto item = registerNames[reg] + (offset ? ((offset > 0 ? "+" : "") + std::to_string(offset)) : "");
        return isDereference ? ('[' + item + ']') : item;
    }
    const std::string MoveOperation::str() const {
        const static std::string sizeTypeNames[] {
            "byte", "word", "dword", "qword"
        };
        return INDENT "mov " + MOVE_OPSIZE_STR_IF_NECESSARY + dest.str() + ", " + src.str() + ADD_COMMENT_IF_EXISTS;
    }
    const std::string LoadAddressOperation::str() const {
        const static std::string sizeTypeNames[] {
            "byte", "word", "dword", "qword"
        };
        return INDENT "lea " + MOVE_OPSIZE_STR_IF_NECESSARY + dest.str() + ", " + src.str() + ADD_COMMENT_IF_EXISTS;
    }
    const std::string XorOperation::str() const {
        return INDENT "xor " + dest.str() + ", " + src.str() + ADD_COMMENT_IF_EXISTS;
    }
    const std::string AndOperation::str() const {
        return INDENT "and " + dest.str() + ", " + src.str() + ADD_COMMENT_IF_EXISTS;
    }
    const std::string CallOperation::str() const {
        return INDENT "call " + prefixed(lbl) + ADD_COMMENT_IF_EXISTS;
    }
    const std::string SubOperation::str() const {
        return INDENT "sub " + dest.str() + ", " + src.str() + ADD_COMMENT_IF_EXISTS;
    }
    const std::string AddOperation::str() const {
        return INDENT "add " + dest.str() + ", " + src.str() + ADD_COMMENT_IF_EXISTS;
    }
    const std::string MulOperation::str() const {
        return INDENT "imul " + dest.str() + ", " + src.str() + ADD_COMMENT_IF_EXISTS;
    }
    const std::string Syscall::str() const {
        return INDENT "syscall";
    }
    const std::string PushOperation::str() const {
        return INDENT "push " + src.str() + ADD_COMMENT_IF_EXISTS;
    }
    const std::string PopOperation::str() const {
        return INDENT "pop " + dest.str() + ADD_COMMENT_IF_EXISTS;
    }
    const std::string LeaveOperation::str() const {
        return INDENT "leave" + ADD_COMMENT_IF_EXISTS;
    }
    const std::string ReturnOperation::str() const {
        return INDENT "ret" + ADD_COMMENT_IF_EXISTS;
    }
    const std::string CmpOperation::str() const {
        return INDENT "cmp " + dest.str() + ", " + src.str() + ADD_COMMENT_IF_EXISTS;
    }
    const std::string JumpOperation::str() const {
        return INDENT + jtypemap[static_cast<int>(type)] + ' ' + prefixed(lbl) + ADD_COMMENT_IF_EXISTS;
    }

    const std::string join(const std::vector<Instruction*>& instructions, const std::string& sep, bool spaceOutLabels) {
        std::string result;
        bool isFirstLabel = false;
        for (size_t index = 0; index < instructions.size(); index++) {
            const auto instr = instructions[index];
            if (spaceOutLabels && dynamic_cast<Label*>(instr)) {
                if (!isFirstLabel) isFirstLabel = true;
                else {
                    result.push_back('\n');
                }
            }
            result += instr->str();
            if (index + 1 < instructions.size()) result += sep;
        }
        return result;
    }
    const std::string join(const std::vector<union SignedUnsigned>& data, bool isSigned, const std::string& sep) {
        std::string result;
        for (size_t index = 0; index < data.size(); index++) {
            result += std::to_string( isSigned ? data[index].s : data[index].u);
            if (index + 1 < data.size()) result += sep;
        }
        return result;
    }
    const std::string join(const std::vector<std::string>& vector, const std::string& sep) {
        std::string result;
        for (size_t index = 0; index < vector.size(); index++) {
            result += vector[index];
            if (index + 1 < vector.size()) result += sep;
        }
        return result;
    }

    union SignedUnsigned SignedUnsigned(uint64_t u) {
        union SignedUnsigned su;
        su.u = u;
        return su;
    }
    union SignedUnsigned SignedUnsigned(int64_t s) {
        union SignedUnsigned su;
        su.s = s;
        return su;
    }

    Location RegisterLocation(Register reg) {
        return {static_cast<int>(reg), 0, false, false, 0, false, "", false};
    }
    Location ValueAtRegisterLocation(Register reg) {
        return {static_cast<int>(reg), 0, false, false, 0, false, "", true};
    }
    Location ValueAtOffsetRegisterLocation(Register reg, long long offset) {
        return {static_cast<int>(reg), offset, false, false, 0, false, "", true};
    }
    Location RBPOffsetLocation(long offset) {
        return {static_cast<int>(Register::rbp), offset, false, false, 0, false, "", true};
    }
    Location NumberLiteralLocation(bool isSigned, union SignedUnsigned value) {
        return {LOC_IS_NOT_REG, 0, true, isSigned, value, false, "", false};
    }
    Location RelativeLabelLocation(const std::string& lbl) {
        return {LOC_IS_NOT_REG, 0, false, false, 0, true, lbl, true};
    }
    bool operator ==(const Location& lhs, const Location& rhs) {
        return (lhs.reg == rhs.reg) && (lhs.offset == rhs.offset) && (lhs.lbl == rhs.lbl) && (lhs.value.u == rhs.value.u) && (lhs.isDereference == rhs.isDereference);
    }
}
