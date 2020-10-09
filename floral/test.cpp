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
    
    ColoredStream out;
    
    out << Color::magenta << Color::bold << "floralc - The Floral Compiler" << Color::reset << '\n';
    
    // Extract args
    CommandParser commandParser (
        { argc, argv }
    );
    if (commandParser.hasErrors()) {
        for (auto &error: commandParser.errors()) {
            error.print();
        }
        return;
    }
    
    const std::string outfile { commandParser.outfile().first };

    for (auto &file: commandParser.infiles()) {
        out << Color::cyan << "Compiling: " << Color::reset << file.first << '\n';
    }
    out << Color::cyan << "Target: " << Color::reset << outfile << '\n';
    
    Timer t;
    t.reset();
    
    _setup();
    // Lexing
    std::string result {'\n'};
    read(commandParser.infiles().front().first, result);
    if (result.empty()) {
        std::cout << "Unable to locate file at path\n";
        return;
    }
    
    Lexer lexer { result };
    
    // Tokens into vector
    std::vector<Token> tokens {lexer.lex()};
    //for (const auto &tkn: tokens) tkn.print();
    result = lexer.code;
    if (commandParser.catSource()) {
        std::cout << result << '\n';
    }
    
    if (lexer.hasErrors()) {
        for (auto &error: lexer.errors()) {
            error.print(result);
        }
        return;
    }
    
    // Parsing
    Parser parser { tokens };
    parser.setPath(commandParser.infiles().front().first);
    File *file { parser.parse() };
    if (!file) return;
    
    if (parser.hasErrors()) {
        for (auto &error: parser.errors()) {
            error.print(result);
        }
        return;
    }
    file->dump();
    
    v2::Compiler compiler;
    
    // Config
    for (auto &use: parser.use()) {
        switch (use) {
            case Use::C:
                compiler.usingCFunctions = true;
                break;
            case Use::syscalls:
                break;
            default:
                break;
        }
    }
    
    compiler.optimization = commandParser.optimization();
    compiler.usingCFunctions |= commandParser.usingCFunctions();
    compiler.notUsingStdlibHeader = commandParser.notUsingStdlibHeader();
    compiler.setOutputDestination(outfile + ".nasm");
    compiler.showTypeTrace(commandParser.typeTrace());
    
    // Compiling
    compiler.setSource(result);
    compiler.setPath(commandParser.infiles().front().first);
    compiler.compile(file);

    if (compiler.hasErrors()) {
        for (auto &error: compiler.errors()) {
            error.print(result);
        }
    } else {
        const std::string cmd {"open -a /Applications/Atom.app " + outfile + ".nasm"};
        system(cmd.c_str());
    }
    
    if (commandParser.logDebugInfo()) {
        // View file node
         out << Color::blue << Color::bold << "AST Info" << Color::reset << "\n--------\n" << Color::cyan << "";
        file->print();
        file->dump();
    }
    
    // Delete dynamically allocated memory
    delete file;
        
    out << Color::cyan << "Compiliation " + (compiler.hasErrors() ? "failed" : "finished in " + std::to_string(t.elapsed()) + " seconds") << Color::reset << '\n';
    
    if (!compiler.hasErrors()) {
        out << Color::blue << Color::bold << "\nModifiers: " << Color::reset << (compiler.notUsingStdlibHeader ? "" : "[link standard lib] ") << (compiler.usingCFunctions ? "[link C bridge] " : "") << '\n';

        std::vector<std::string> objfiles {
            "~/Programming/floral-src/stdlib/obj/init.o"
        };
        
        const std::string make_obj_cmd {
            "/usr/local/bin/nasm -f macho64 -o " + outfile + ".o " + outfile + ".nasm"
        };
        for (auto file: commandParser.infiles()) {
            if (file.second == CmdFileExt::c) {
                const std::string compile_c_cmd {
                    "gcc -c " + file.first + " -O" + std::to_string(compiler.optimization)
                };
                out << Color::cyan << "Executing: " << Color::white << compile_c_cmd << Color::reset << '\n';
                system(compile_c_cmd.c_str());
                
                file.first.erase(file.first.end() - 2, file.first.end());
                objfiles.push_back(file.first + ".o");
            }
        }
        if (!compiler.notUsingStdlibHeader) {
            objfiles.push_back("~/Programming/floral-src/stdlib/obj/std.o");
        }
        if (compiler.usingCFunctions) {
            objfiles.push_back("~/Programming/floral-src/stdlib/obj/cbridge.o");
        }
        std::string link_exec_cmd {
            "ld -o " + outfile + ' ' + outfile + ".o"
        };
        for (auto objf: objfiles) {
            link_exec_cmd += ' ' + objf;
        }
        link_exec_cmd += " -lSystem";
        out << Color::cyan << "Executing: " << Color::white << make_obj_cmd << Color::reset << '\n';
        system(make_obj_cmd.c_str());
        out << Color::cyan << "Executing: " << Color::white << link_exec_cmd << Color::reset << '\n';
        system(link_exec_cmd.c_str());
    }
}
