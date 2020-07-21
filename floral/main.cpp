//
//  main.cpp
//  floral
//
//  Created by Ethan Uppal on 6/28/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include <iostream>
#include <string>
#include <vector>

#include "Sources.hpp"

double testCompile() {
    Timer t;
    
    // Lexing
    std::string result;
    Floral::read("/Users/ethanuppal/Library/Mobile Documents/com~apple~CloudDocs/Xcode Projects/floral/floral/proj/main.floral", result);
    if (result.empty()) {
        double elapsed { t.elapsed() };
        std::cout << "Unable to locate file at path\n";
        return elapsed;
    }
    
    Floral::Lexer lexer { result };
    
    // Tokens into vector
    std::vector<Floral::Token> tokens;
    for (const auto &tkn: lexer.lex()) {
        tokens.push_back(tkn);
    }
    
    // Parsing
    Floral::Parser parser { tokens };
    Floral::File *file { parser.parse() };
    
    if (parser.hasErrors()) {
        double elapsed { t.elapsed() };
        for (auto error: parser.errors()) {
            error.print();
        }
        return elapsed;
    }
    
    file->setPath("/Users/ethanuppal/Library/Mobile Documents/com~apple~CloudDocs/Xcode Projects/floral/floral/proj/main.floral");

    // Compiling
    Floral::Compiler compiler;
    compiler.setOutputDestination("asm_out.s");
    compiler.compile(file);
    double elapsed { t.elapsed() };
    
    // View file node
    file->print();
    file->dump();
    
    // Delete dynamically allocated memory
    delete file;
    
    return elapsed;
}

int main() {
    Floral::_setup();
    auto elapsed { testCompile() };
    std::cout << "Compiliation attempt finished in " << elapsed << " seconds\n";
    Floral::_free();

    return 0;
}
