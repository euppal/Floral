//
//  test.hpp
//  floral
//
//  Created by Ethan Uppal on 8/29/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef test_hpp
#define test_hpp

#include "CommandParser.hpp"
#include "Compiler.hpp"
#include "Colors.hpp"
#include "File IO.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include <set>
using namespace Floral;

void title(const std::string& str);
void heading(const std::string& str);
void heading2(const std::string& h, const std::string& str);
void note(const std::string& str);
void annotated(const std::string& caption, const std::string& content);
void execute(const std::string& cmd, const CommandParser& cmdParser);
void print(const std::string& str);

int compile(CmdFile& infile, const CommandParser& cmdParser, Compiler& compiler, std::set<std::string>& libs);
void make_objfile(std::vector<std::string>& objfiles, CmdFile& infile, const Compiler& compiler, const CommandParser& cmdParser);

#endif /* test_hpp */
