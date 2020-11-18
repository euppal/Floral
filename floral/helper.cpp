//
//  helper.cpp
//  floralc
//
//  Created by Ethan Uppal on 10/9/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "driver.hpp"
ColoredStream out(std::cout);

void title(const std::string& str) {
    out << Color::magenta << Color::bold << str << Color::reset << '\n';
}

inline void heading(const std::string& str) {

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

void execute(const std::string& cmd, const CommandParser& cmdParser) {
    if (cmdParser.printNotRunCmds()) {
        annotated("Command", cmd);
        return;
    }
    if (cmdParser.isVerbose()) {
        annotated("Executing", cmd);
    }
    system(cmd.c_str());
}

void print(const std::string& str) {
    out << str;
}

int compile(CmdFile& infile, const CommandParser& cmdParser, Compiler& compiler, std::set<std::string>& libs) {
    if (cmdParser.printNotRunCmds()) {
        while (infile.first.back() != '.') {
            infile.first.pop_back();
        }
        infile.first.pop_back();
        infile.first += ".nasm";
        infile.second = CmdFileExt::nasm;
        annotated("Generate", infile.first);
        if (cmdParser.openASM()) {
            execute("open -a /Applications/Atom.app \"" + infile.first + '\"', cmdParser);
        }
        return 0;
    }
    
    std::string result {'\n'};
    read(infile.first, result);
    if (result.empty()) {
        std::cout << "Unable to locate file at path\n";
        return 1;
    }

    // Lexing
    v2::Lexer lexer { result, infile.first, cmdParser };

    // Tokens into vector
    std::vector<Token> tokens {lexer.lex()};
    //for (const auto &tkn: tokens) tkn.print();
    result = lexer.preprocessor().source();
    if (cmdParser.catSource()) {
        std::cout << result << '\n';
    }

    if (lexer.hasErrors()) {
        for (auto &error: lexer.errors()) {
            error.print(result);
        }
        return 1;
    }

    // Parsing
    Parser parser { tokens };
    parser.setPath(infile.first);
    File *file { parser.parse() };
    if (!file) return 1;

    if (parser.hasErrors()) {
        for (auto &error: parser.errors()) {
            error.print(result);
        }
        return 1;
    }

    for (auto &use: parser.use()) {
        switch (use) {
            case Use::stl:
                libs.insert("stl");
                break;
            case Use::libc:
                libs.insert("libc");
                break;
            default:
                break;
        }
    }

    if (cmdParser.logDebugInfo()) {
        // View file node
        out << Color::blue << Color::bold << "AST Info" << Color::reset << "\n--------\n" << Color::cyan << "";
        file->print();
        file->dump();
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
        dealloc(file);
        return 1;
    }
    if (cmdParser.openASM()) {
        execute("open -a /Applications/Atom.app \"" + infile.first + '\"', cmdParser);
    }

    // Delete dynamically allocated memory
    dealloc(file);

    return 0;
}

void make_objfile(std::vector<std::string>& objfiles, CmdFile& infile, const Compiler& compiler, const CommandParser& cmdParser) {
    if (infile.second == CmdFileExt::nasm) {
        std::string objfile { infile.first };
        objfile.erase(objfile.end() - 4, objfile.end());
        objfile.push_back('o');
        execute("/usr/local/bin/nasm -f macho64 -o \"" + objfile + "\" \"" + infile.first + '\"', cmdParser);
        objfiles.push_back('\"' + objfile + '\"');
        infile.first = objfile;
        infile.second = CmdFileExt::object;
    } else if (infile.second == CmdFileExt::c) {
        execute("gcc -c \"" + infile.first + " \"-O" + std::to_string(compiler.optimization), cmdParser);
        infile.first.pop_back();
        infile.first.push_back('o');
        infile.first = '\"' + infile.first + '\"';
        infile.second = CmdFileExt::object;
    }
}
