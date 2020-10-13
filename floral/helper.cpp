//
//  helper.cpp
//  floralc
//
//  Created by Ethan Uppal on 10/9/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "driver.hpp"
ColoredStream out;

void title(const std::string& str) {
    out << Color::magenta << Color::bold << str << Color::reset << '\n';
}

void heading(const std::string& str) {
    
}

void heading2(const std::string& h, const std::string& str) {
    out << Color::blue << Color::bold << h << ": " << Color::reset << str << '\n';
}

void note(const std::string& str) {
    out << Color::cyan << str << Color::reset << '\n';
}

void annotated(const std::string& caption, const std::string& content) {
    out << Color::cyan << caption << ": " << Color::reset << content << '\n';
}

void execute(const std::string& cmd) {
    annotated("Executing", cmd);
    system(cmd.c_str());
}

void compile(CmdFile& infile, const CommandParser& cmdParser, v2::Compiler& compiler) {    
    std::string result {'\n'};
    read(infile.first, result);
    if (result.empty()) {
        std::cout << "Unable to locate file at path\n";
        return;
    }
    
    // Lexing
    Lexer lexer { result };
    
    // Tokens into vector
    std::vector<Token> tokens {lexer.lex()};
    //for (const auto &tkn: tokens) tkn.print();
    result = lexer.code;
    if (cmdParser.catSource()) {
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
    parser.setPath(infile.first);
    File *file { parser.parse() };
    if (!file) return;
    
    if (parser.hasErrors()) {
        for (auto &error: parser.errors()) {
            error.print(result);
        }
        return;
    }
    
    for (auto &use: parser.use()) {
        switch (use) {
            case Use::C:
                compiler.usingCFunctions = true;
                break;
            case Use::stdlib:
                compiler.notUsingStdlib = false;
                break;
            default:
                break;
        }
    }
    
    // Compiling
    compiler.setSource(result);
    compiler.setPath(infile.first);
    
    while (infile.first.back() != '.') {
        infile.first.pop_back();
    }
    infile.first.pop_back();
    infile.first += ".nasm";
    infile.second = CmdFileExt::nasm;

    compiler.setOutputDestination(infile.first);
    compiler.compile(file);
    
    if (compiler.hasErrors()) {
        for (auto &error: compiler.errors()) {
            error.print(result);
        }
    } else {
        execute("open -a /Applications/Atom.app \"" + infile.first + '\"');
    }
    
    if (cmdParser.logDebugInfo()) {
        // View file node
        out << Color::blue << Color::bold << "AST Info" << Color::reset << "\n--------\n" << Color::cyan << "";
        file->print();
        file->dump();
    }
    
    
    // Delete dynamically allocated memory
    delete file;
}

void make_objfile(std::vector<std::string>& objfiles, CmdFile& infile, const v2::Compiler& compiler) {
    if (infile.second == CmdFileExt::nasm) {
        std::string objfile { infile.first };
        objfile.erase(objfile.end() - 4, objfile.end());
        objfile.push_back('o');
        execute("/usr/local/bin/nasm -f macho64 -o \"" + objfile + "\" \"" + infile.first + '\"');
        objfiles.push_back('\"' + objfile + '\"');
        infile.first = objfile;
        infile.second = CmdFileExt::object;
    } else if (infile.second == CmdFileExt::c) {
        execute("gcc -c \"" + infile.first + " \"-O" + std::to_string(compiler.optimization));
        infile.first.pop_back();
        infile.first.push_back('o');
        infile.first = '\"' + infile.first + '\"';
        infile.second = CmdFileExt::object;
    }
}
