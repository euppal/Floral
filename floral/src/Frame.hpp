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
        rax, rbx, rcx, rdx, rdi, rsi, rbp, rsp, r8, r9, r10, r11, r12, r13, r14, r15, eax
    };
    const std::string registerNames[] {
        "rax", "rbc", "rcx", "rdx", "rdi", "rsi", "rbp", "rsp", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "eax"
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
        
        int avaliableScratch();
        void returnScratchRegister(Register r);
        void addData(Location loc, size_t size, const std::string& name);
        

        std::pair<Variable, bool> localLookup(const std::string& name) const;
    };
}


#endif /* Frame_hpp */
