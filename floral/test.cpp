//
//  test.cpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 9/2/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Sources.hpp"
using namespace Floral;

void test(int argc, const char* argv[]) {
    
    // Extract args
    CommandParser commandParser (
        {argc, argv}
    );
    
    Timer t;
    t.reset();
    
    _setup();
    // Lexing
    std::string result {'\n'};
    read(commandParser.infiles().front(), result);
    if (result.empty()) {
        std::cout << "Unable to locate file at path\n";
        return;
    }
    
    Lexer lexer { result };
    
    // Tokens into vector
    std::vector<Token> tokens;
    for (const auto &tkn: lexer.lex()) {
        tokens.push_back(tkn);
        tkn.print();
    }
    
    // Parsing
    Parser parser { tokens };
    parser.setPath(commandParser.infiles().front());
    File *file { parser.parse() };
    if (!file) return;
    
    if (parser.hasErrors()) {
        for (auto error: parser.errors()) {
            error.print(result);
        }
        return;
    }
    
    v2::Compiler compiler;
    
    // Config
    compiler.optimization = commandParser.optimization();
    compiler.usingCFunctions = commandParser.usingCFunctions();
    compiler.notUsingStdlibHeader = commandParser.notUsingStdlibHeader();
    compiler.setOutputDestination(commandParser.outfile());
    
    // Compiling
    compiler.setSource(result);
    compiler.compile(file);

    if (compiler.hasErrors()) {
        for (auto error: compiler.errors()) {
            error.print(result);
        }
    } else {
        const std::string cmd {"open /Users/ethanuppal/Library/Developer/Xcode/DerivedData/floral-cfenahbtrrcouqajupekfvjcqxgw/Build/Products/Debug/" + commandParser.outfile()};
        system(cmd.c_str());
    }
    
    // View file node
    file->print();
    file->dump();
    
    // Delete dynamically allocated memory
    delete file;
        
    std::cout << "Compiliation finished in " << t.elapsed() << " seconds\n";
}
