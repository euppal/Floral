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

void testCompile() {
    // Lexing
    std::string result;
    Floral::read("/Users/ethanuppal/Library/Mobile Documents/com~apple~CloudDocs/Xcode Projects/floral/floral/proj/main.floral", result);
    if (result.empty()) {
        std::cout << "Unable to locate file at path\n";
        return;
    }
    
    Floral::Lexer lexer { result };
    
    // Tokens into vector
    std::vector<Floral::Token> tokens;
    for (const auto &tkn: lexer.lex()) {
        tkn.print();
        tokens.push_back(tkn);
    }
    
    // Parsing
    Floral::Parser parser { tokens };
    Floral::File *file { parser.parse() };
    
    if (parser.hasErrors()) {
        for (auto error: parser.errors()) {
            error.print();
        }
        return;
    }
    
    file->setPath("/Users/ethanuppal/Library/Mobile Documents/com~apple~CloudDocs/Xcode Projects/floral/floral/proj/main.floral");
    file->print();
    file->dump();

    // Compiling
    Floral::Compiler compiler;
    compiler.setOutputDestination("asm_out.s");
    compiler.compile(file);
    
    // Delete dynamically allocated memory
    delete file;
}

void _main() {
    Floral::_setup();
    testCompile();
    Floral::_free();
}

int main() {
    Timer t;
    _main();
    std::cout << "Compiliation attempt finished in " << t.elapsed() << " seconds\n";
    return 0;
}
