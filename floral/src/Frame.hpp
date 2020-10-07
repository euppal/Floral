//
//  Frame.hpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 9/1/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Frame_hpp
#define Frame_hpp

/*
 * https://wiki.cdot.senecacollege.ca/wiki/X86_64_Register_and_Instruction_Quick_Start
 *
 * Usage during syscall/function call:
 *
 * First six arguments are in rdi, rsi, rdx, rcx, r8d, r9d; remaining arguments are on the stack.
 * For syscalls, the syscall number is in rax.
 * Return value is in rax.
 * The called routine is expected to preserve rsp,rbp, rbx, r12, r13, r14, and r15 but may trample any other registers.
*/

#include <vector>
#include <string>
#include "Instruction.hpp"

namespace Floral {
    struct Location;
    enum class Register {
        rax, rbx, rcx, rdx, rdi, rsi, rbp, rsp,
        eax, ebx, ecx, edx, edi, esi, ebp, esp,
        r8, r9, r10, r11, r12, r13, r14, r15,
        xmm0, xmm1, xmm2, xmm3, xmm4, xmm5, xmm6, xmm7,
        al
    };
    const std::string registerNames[] {
        "rax", "rbx", "rcx", "rdx", "rdi", "rsi", "rbp", "rsp",
        "eax", "ebx", "ecx", "edx", "edi", "esi", "ebp", "esp",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7",
        "al"
    };
    bool isScratch(Register reg);
    struct Variable {
        Location loc;
        size_t size;
        std::string name;
    };
    struct Frame {
        std::vector<Register> registersInUse;
        std::vector<Variable> data;
        long nextOffset() const;
        std::string id;
        
        int avaliableScratch();
        void returnScratchRegister(Register r);
        void addData(Location loc, size_t size, const std::string& name);
        

        std::pair<Variable, bool> localLookup(const std::string& name) const;
    };
}


#endif /* Frame_hpp */
