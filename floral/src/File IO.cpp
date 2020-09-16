//
//  File IO.cpp
//  floral
//
//  Created by Ethan Uppal on 6/29/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "File IO.hpp"
#include <string>
#include <iostream>
#include <fstream>

namespace Floral {
    void read(const std::string path, std::string &result) {
//        FILE *file { fopen(path.c_str(), "r") };
//        fseek(file, 0, SEEK_END);
//        long length { ftell(file) };
//        rewind(file);
//        char ptr[length];
//        if (fread(ptr, sizeof(char), length, file) != length) {
//            std::cout << "Error in reading file\n";
//            std::cout << length << '\n';
//        }
//        result = ptr;
//        fclose(file);
        if (auto fileStream = std::ifstream { path }) {
            std::string next;
            while (getline(fileStream, next)) {
                result.append(next);
                result.push_back('\n');
            }
        }
    }
    void write(const std::string path, const std::string &contents) {
//        FILE *file { fopen(path.c_str(), "wb") };
//        fwrite(contents.c_str(), sizeof(char), contents.size(), file);
//        fclose(file);
        
        if (auto fileStream = std::ofstream { path })
            fileStream << contents.data();
    }
}
