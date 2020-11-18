//
//  test.cpp
//  Floral in C++ - Compiled
//
//  Created by Ethan Uppal on 9/2/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "driver.hpp"
#include "Sources.hpp"

#define ON_VERBOSE if (commandParser.isVerbose() && !commandParser.printNotRunCmds())

void driver(CommandParser& commandParser) {
    if (commandParser.hasErrors()) {
        for (auto &error: commandParser.errors()) {
            error.print();
        }
        return;
    }
    
    if (commandParser.showHelp()) {
        title("floralc - The Floral Compiler");
        annotated("\nUSAGE", "floralc [options] files...");
        annotated(
            "\nOPTIONS",
            "\n"
            "-###                   Print (but do not run) the commands to run for this compilation\n"
            "-c                     Compile the source without linking\n"
            "-cat-src, -s           Concatenate the preprocessed source code\n"
            "-dump-type-trace, -t   Dump the static analyzer's type trace\n"
            "-stack-guard, -g       Inserts the xor of the return address and the base pointer and ensures that it remains unmodified\n"
            "-o <target>            Specifies the executable target name\n"
            "-open-asm              Open the generated assembly for debugging purposes\n"
            "-O                     Use optimizations\n"
            "-print-ast, -a         Print the AST for debugging purposes\n"
            "-S                     Stop after assembly generation\n"
            "-use <lib>, -U<lib>    Link with the specified library (e.g. 'stl', 'C')\n"
            "-verbose, -v           Produce verbose (colored) output"
        );
        return;
    }
    
    ON_VERBOSE title("floralc - The Floral Compiler");
    
    const std::string outfile { commandParser.outfile().first };

    Compiler compiler;

    Timer t;
    t.reset();
    
    _setup();
    
    // Config
    compiler.optimization = commandParser.optimization();
    compiler._stackGuard = commandParser.stackGuard();
    compiler.showTypeTrace(commandParser.typeTrace());
    
    int compileError {};
    std::set<std::string> libs;
    commandParser.initLibs(libs);
    
    for (auto &infile: commandParser.infiles()) {
        if (infile.second == CmdFileExt::floral) {
            ON_VERBOSE annotated("Compiling", infile.first);
            compileError += (compile(infile, commandParser, compiler, libs) != 0);
        }
    }
    
    ON_VERBOSE annotated("Target", outfile);

    ON_VERBOSE note("Compiliation " + (compiler.hasErrors() || compileError ? "failed" : "finished in " + std::to_string(t.elapsed()) + " seconds"));
    if (compiler.hasErrors() || compileError || commandParser.stopAtASM()) return;
    
    ON_VERBOSE heading2("\nModifiers", (std::string)(commandParser.usingSTL() ? "[link standard lib] " : "") + (commandParser.usingCBridge() ? "[link C bridge] " : ""));
    
    std::vector<std::string> objfiles {
        FLORAL_RUNTIME(core)
    };
    for (auto &lib: libs) {
        objfiles.push_back(liblocFromName(lib));
    }
    for (auto &infile: commandParser.infiles()) {
        make_objfile(objfiles, infile, compiler, commandParser);
    }
    
    if (commandParser.justCompile()) return;
    
    std::string link_exec_cmd {
        "ld -o " + outfile
    };
    for (auto objf: objfiles) {
        link_exec_cmd += ' ' + objf;
    }
    link_exec_cmd += " -lSystem";
    execute(link_exec_cmd, commandParser);
    
    _free();
}
