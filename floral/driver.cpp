//
//  test.cpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 9/2/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "driver.hpp"
#include "Sources.hpp"

void driver(CommandParser& commandParser) {
    title("floralc - The Floral Compiler");
    
    if (commandParser.hasErrors()) {
        for (auto &error: commandParser.errors()) {
            error.print();
        }
        return;
    }
    
    const std::string outfile { commandParser.outfile().first };

    for (auto &file: commandParser.infiles()) {
        annotated("Compiling", file.first);
    }
    annotated("Target", outfile);

    v2::Compiler compiler;

    Timer t;
    t.reset();
    
    _setup();
    
    // Config
    compiler.optimization = commandParser.optimization();
    compiler.usingCFunctions = commandParser.usingCFunctions();
    compiler.notUsingStdlib = commandParser.notUsingStdlib();
    compiler.showTypeTrace(commandParser.typeTrace());
    
    for (auto &infile: commandParser.infiles()) {
        if (infile.second == CmdFileExt::floral) {
            compile(infile, commandParser, compiler);
        }
    }
        
    note("Compiliation " + (compiler.hasErrors() ? "failed" : "finished in " + std::to_string(t.elapsed()) + " seconds"));
    
    if (!compiler.hasErrors()) {
        heading2("\nModifiers", (std::string)(compiler.notUsingStdlib ? "" : "[link standard lib] ") + (compiler.usingCFunctions ? "[link C bridge] " : ""));

        std::vector<std::string> objfiles {
            "~/Programming/floral-src/stdlib/obj/init.o"
        };
        if (!compiler.notUsingStdlib) {
            objfiles.push_back("~/Programming/floral-src/stdlib/obj/std.o");
        }
        if (compiler.usingCFunctions) {
            objfiles.push_back("~/Programming/floral-src/stdlib/obj/cbridge.o");
        }
        for (auto &infile: commandParser.infiles()) {
            make_objfile(objfiles, infile, compiler);
        }

        std::string link_exec_cmd {
            "ld -o " + outfile
        };
        for (auto objf: objfiles) {
            link_exec_cmd += ' ' + objf;
        }
        link_exec_cmd += " -lSystem";
        execute(link_exec_cmd);
    }
}
