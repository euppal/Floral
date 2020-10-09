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
using namespace Floral;

void title(const std::string& str);
void heading(const std::string& str);
void heading2(const std::string& h, const std::string& str);
void note(const std::string& str);
void annotated(const std::string& caption, const std::string& content);
void execute(const std::string& cmd);

void compile(CmdFile& infile, const CommandParser& cmdParser, v2::Compiler& compiler);
void make_objfile(std::vector<std::string>& objfiles, CmdFile& infile, const v2::Compiler& compiler);

#endif /* test_hpp */
