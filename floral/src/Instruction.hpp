//
//  Instruction.hpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 9/3/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Instruction_hpp
#define Instruction_hpp

constexpr bool NO_COMMENTS = false;

#include <string>
#include <vector>
#include <cinttypes>

#define LOC_IS_NOT_REG (-1)
#define RETURN_VALUE_LOC (RegisterLocation(Register::rax))
#define RETURN_VALUE_LOC_32b (RegisterLocation(Register::eax))
#define INDENT "  "
#define prefixed(lbl) (FLORAL_ID_PREFIX + lbl)
#define ADD_COMMENT_IF_EXISTS ((NO_COMMENTS || comment.empty()) ? "" : (" ; " + comment))
#define OPSIZE_FROM_NUM(n) ((n) == 1 ? SizeType::byte : ((n) == 2 ? SizeType::word : ((n) == 4 ? SizeType::dword : SizeType::qword)))
#define MOVE_OPSIZE_STR_IF_NECESSARY (((src.isLiteral && dest.isDereference) ? sizeTypeNames[static_cast<int>(opsize)] + ' ' : ""))
#define IS_RBPOFFSET(loc) ((loc).reg == static_cast<int>(Register::rbp) && (loc).isDereference)
#define IS_REG(loc) ((loc).reg != LOC_IS_NOT_REG && !(loc).isDereference && !(loc).offset)
#define ARE_BOTH_LIT(leftop, rightop) ((leftop)->src.isLiteral && (rightop)->src.isLiteral)
#define NO_OPTM(op) (!(op)->comment.empty() && (op)->comment.front() == '@')
namespace Floral {
    const std::string join(const std::vector<std::string>& vector, const std::string& sep);

    struct Instruction {
        virtual ~Instruction() {}
        virtual const std::string str() const = 0;
    };
    const std::string join(const std::vector<Instruction*>& instructions, const std::string& sep, bool spaceOutLabels);

    struct RawText: public Instruction {
        RawText(const std::string& _rawText): rawText(_rawText) {}
        ~RawText() override {}
        std::string rawText;
        
        virtual const std::string str() const override;
    };

    enum class SectionType {
        text, data, rodata, bss
    };
    struct Section: public Instruction {
        Section(SectionType _type): type(_type) {}
        ~Section() override { for (auto instr: instructions) delete instr; }
        
        const SectionType type;
        std::vector<Instruction*> instructions;
        bool spaceOutLabels = false;
        void add(Instruction* instr);
        
        const std::string str() const override;
    };
    struct Label: public Instruction {
        Label(const std::string& _lbl, bool _isGlobal): lbl(_lbl), isGlobal(_isGlobal) {}
        ~Label() override {}
        
        std::string lbl;
        bool isGlobal;
                
        const std::string str() const override;
    };
    struct Extern: public Instruction {
        Extern(const std::string& _lbl): lbl(_lbl) {}
        ~Extern() override {}
        
        std::string lbl;
        
        const std::string str() const override;
    };
    struct Global: public Instruction {
        Global(const std::string& _lbl): lbl(_lbl) {}
        ~Global() override {}
        
        std::string lbl;
        
        const std::string str() const override;
    };
    enum class SizeType {
        byte, word, dword, qword
    };
    union SignedUnsigned {
        uint64_t u;
        int64_t s;
    };
    union SignedUnsigned SignedUnsigned(uint64_t u);
    union SignedUnsigned SignedUnsigned(int64_t s);
    #define SU(n) SignedUnsigned(n)

    struct Data: public Instruction {
        Data(const std::string& _label, SizeType _sizeType, bool _isSigned): label(_label), sizeType(_sizeType), isSigned(_isSigned) {}
        ~Data() override {}
        
        std::string label;
        SizeType sizeType;
        bool isSigned;
        std::vector<union SignedUnsigned> values;
        
        const std::string str() const override;
    };
    const std::string join(const std::vector<union SignedUnsigned>& data, bool isSigned, const std::string& sep);

    struct ZeroData: public Instruction {
        ZeroData(const std::string& _label, SizeType _sizeType): label(_label), sizeType(_sizeType) {}
        ~ZeroData() override {}
        
        std::string label;
        SizeType sizeType;
        
        const std::string str() const override;
    };
    struct StringData: public Instruction {
        StringData(const std::string& _label, const std::string& _contents): label(_label), contents(_contents) {}
        ~StringData() override {}
        
        std::string label;
        std::string contents;
        
        const std::string str() const override;
    };
    struct LengthOf: public Instruction {
        LengthOf(const std::string& _lbl, const std::string _src): lbl(_lbl), src(_src) {}
        ~LengthOf() override {}
        
        std::string lbl;
        std::string src;
        
        const std::string str() const override;
    };
    enum class OpType {
        mov, add, sub, xor_, imul, idiv, and_, or_, not_, push, pop, call, leave, ret
    };
    struct Operation: public Instruction {
        OpType opType;
        virtual ~Operation() {}
        
        virtual const std::string str() const override = 0;
    };
    struct Location {
        int reg;
        long long offset;
        bool isLiteral;
        bool isSigned;
        union SignedUnsigned value;
        bool isLbl;
        std::string lbl;
        bool isDereference;
        
        const std::string str() const;
        
        friend bool operator ==(const Location& lhs, const Location& rhs);
    };
    enum class Register;
    Location RegisterLocation(Register reg);
    Location ValueAtRegisterLocation(Register reg);
    Location ValueAtOffsetRegisterLocation(Register reg, long long offset);
    Location RBPOffsetLocation(long offset);
    Location NumberLiteralLocation(bool isSigned, union SignedUnsigned value);
    Location RelativeLabelLocation(const std::string& lbl);
    #define NumLL(isSigned, value) NumberLiteralLocation(isSigned, value)
    #define RelLabelL(lbl) RelativeLabelLocation(lbl)

    struct MoveOperation: public Operation {
        MoveOperation(Location _dest, Location _src, SizeType _opsize, const std::string& _comment = ""): src(_src), dest(_dest), opsize(_opsize), comment(_comment) {}
        ~MoveOperation() override {}
        
        Location src;
        Location dest;
        SizeType opsize;
        std::string comment;
        
        const std::string str() const override;
    };
    struct LoadAddressOperation: public Operation {
        LoadAddressOperation(Location _dest, Location _src, SizeType _opsize, const std::string& _comment = ""): src(_src), dest(_dest), opsize(_opsize), comment(_comment) {}
        ~LoadAddressOperation() override {}
        
        Location src;
        Location dest;
        SizeType opsize;
        std::string comment;
        
        const std::string str() const override;
    };
    struct AddOperation: public Operation {
        AddOperation(Location _dest, Location _src, const std::string& _comment = ""): src(_src), dest(_dest), comment(_comment) {}
        ~AddOperation() override {}
        
        Location src;
        Location dest;
        std::string comment;
        
        const std::string str() const override;
    };
    struct SubOperation: public Operation {
        SubOperation(Location _dest, Location _src, const std::string& _comment = ""): src(_src), dest(_dest), comment(_comment) {}
        ~SubOperation() override {}
        
        Location src;
        Location dest;
        std::string comment;
        
        const std::string str() const override;
    };
    struct XorOperation: public Operation {
        XorOperation(Location _dest, Location _src, const std::string& _comment = ""): src(_src), dest(_dest), comment(_comment) {}
        ~XorOperation() override {}
        
        Location src;
        Location dest;
        std::string comment;
        
        const std::string str() const override;
    };
    struct AndOperation: public Operation {
        AndOperation(Location _dest, Location _src, const std::string& _comment = ""): src(_src), dest(_dest), comment(_comment) {}
        ~AndOperation() override {}
        
        Location src;
        Location dest;
        std::string comment;
        
        const std::string str() const override;
    };
    struct OrOperation: public Operation {
        OrOperation(Location _dest, Location _src, const std::string& _comment = ""): src(_src), dest(_dest), comment(_comment) {}
        ~OrOperation() override {}
        
        Location src;
        Location dest;
        std::string comment;
        
        const std::string str() const override;
    };
    struct MulOperation: public Operation {
        MulOperation(Location _dest, Location _src, const std::string& _comment = ""): src(_src), dest(_dest), comment(_comment) {}
        ~MulOperation() override {}
        
        Location src;
        Location dest;
        std::string comment;
        
        const std::string str() const override;
    };
    struct DivOperation: public Operation {
        DivOperation(Location _src, const std::string& _comment = ""): src(_src), comment(_comment) {}
        ~DivOperation() override {}
        
        Location src;
        std::string comment;
        
        const std::string str() const override;
    };
    struct PushOperation: public Operation {
        PushOperation(Location _src, const std::string& _comment = ""): src(_src), comment(_comment) {}
        ~PushOperation() override {}
        
        Location src;
        std::string comment;
        
        const std::string str() const override;
    };
    struct PopOperation: public Operation {
        PopOperation(Location _dest, const std::string& _comment = ""): dest(_dest), comment(_comment) {}
        ~PopOperation() override {}
        
        Location dest;
        std::string comment;
        
        const std::string str() const override;
    };
    struct CallOperation: public Operation {
        CallOperation(std::string _lbl, const std::string& _comment = ""): lbl(_lbl), comment(_comment) {}
        ~CallOperation() override {}
        
        std::string lbl;
        std::string comment;
        
        const std::string str() const override;
    };
    struct LeaveOperation: public Operation {
        LeaveOperation(const std::string& _comment = ""): comment(_comment) {}
        ~LeaveOperation() override {}
        
        std::string comment;
        
        const std::string str() const override;
    };
    struct ReturnOperation: public Operation {
        ReturnOperation(const std::string& _comment = ""): comment(_comment) {}
        ~ReturnOperation() override {}
        
        std::string comment;
        
        const std::string str() const override;
    };
    struct Syscall: public Instruction {
        Syscall(const std::string& _comment = ""): comment(_comment) {}
        ~Syscall() override {}
        
        std::string comment;
        
        const std::string str() const override;
    };
    struct CmpOperation: public Operation {
        CmpOperation(Location _dest, Location _src, const std::string& _comment = ""): src(_src), dest(_dest), comment(_comment) {}
        ~CmpOperation() override {}
        
        Location src;
        Location dest;
        std::string comment;
        
        const std::string str() const override;
    };
    struct JumpOperation: public Operation {
        enum class JType {
            normal,
            zero,
            nonzero
        };
        const std::string jtypemap[3] {
            "jmp", "jz", "jnz"
        };
        JumpOperation(JType _type, std::string _lbl, const std::string& _comment = ""): type(_type), lbl(_lbl), comment(_comment) {}
        ~JumpOperation() override {}
        
        JType type;
        std::string lbl;
        std::string comment;
        
        const std::string str() const override;
    };
}


#endif /* Instruction_hpp */
