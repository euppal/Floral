//
//  Frame.cpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 9/1/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Frame.hpp"
#include <algorithm>
#define max(x,y) ((x)>(y)?(x):(y))

namespace Floral {
    bool isScratch(Register reg) {
        return reg == Register::rax || reg == Register::rcx || reg == Register::rdx || reg == Register::rsi || reg == Register::rdi || reg == Register::r8 || reg == Register::r9 || reg == Register::r10 || reg == Register::r11;
    }

    long Frame::nextOffset() const {
        if (data.empty()) return 0;
        long maxOffset = 0;
        
        for (auto iter = data.rbegin(); iter != data.rend(); iter++) {
            if ((*iter).loc.reg == LOC_IS_NOT_REG) {
                maxOffset = max(maxOffset, (*iter).loc.stack + (*iter).size);
            }
        }
        return maxOffset;
    }
    int Frame::avaliableScratch() {
        for (int r{}; r < static_cast<int>(Register::r15); r++) {
            const Register reg {static_cast<Register>(r)};
            if (std::find(registersInUse.begin(), registersInUse.end(), reg) == registersInUse.end() && isScratch(reg)) {
                registersInUse.push_back(reg);
                return r;
            }
        }
        return -1;
    }
    void Frame::returnScratchRegister(Register r) {
        for (int i {}; i < registersInUse.size(); i++) {
            if (registersInUse[i] == r) {
                registersInUse.erase(registersInUse.begin() + i);
            }
        }
    }
    void Frame::addData(Location loc, size_t size, const std::string& name) {
        data.push_back({loc, size, name});
    }

    std::pair<Variable, bool> Frame::localLookup(const std::string& name) const {
        std::pair<Variable, bool> result {
            {0, 0},
            false
        };
        
        const auto iter = std::find_if(data.begin(), data.end(), [name](Variable v){
            return v.name == name;
        });
        if (iter != data.end()) {
            result.first = *iter;
            result.second = true;
        }
        
        return result;
    }



}
