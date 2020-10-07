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
    if (commandParser.hasErrors()) {
        for (auto error: commandParser.errors()) {
            error.print();
        }
        return;
    }
    
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
    
    if (lexer.hasErrors()) {
        for (auto error: lexer.errors()) {
            error.print(result);
        }
        return;
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
    compiler.setPath(commandParser.infiles().front());
    compiler.compile(file);

    if (compiler.hasErrors()) {
        for (auto error: compiler.errors()) {
            error.print(result);
        }
    } else {
        const std::string cmd {"open -a /Applications/Atom.app /Users/ethanuppal/Library/Developer/Xcode/DerivedData/floral-cfenahbtrrcouqajupekfvjcqxgw/Build/Products/Debug/" + commandParser.outfile()};
        system(cmd.c_str());
    }
    
    // View file node
    ColoredStream out; out << Color::blue << Color::bold << "AST Info" << Color::reset << "\n--------\n";
    file->print();
    file->dump();
    
    // Delete dynamically allocated memory
    delete file;
        
    std::cout << "Compiliation " + (compiler.hasErrors() ? "failed" : "finished in " + std::to_string(t.elapsed()) + " seconds") << '\n';
}
