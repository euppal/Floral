//
//  main.cpp
//  floral
//
//  Created by Ethan Uppal on 6/28/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "driver.hpp"
void driver(Floral::CommandParser& commandParser);

#define return_if_errors(error_reporting) \
    if ((error_reporting).hasErrors()) { \
        for (const auto &error: (error_reporting).errors()) { \
            error.print(); \
        } \
        return 1; \
    }

int main(int argc, const char* argv[]) {
    Floral::CommandParser commandParser (
        { argc, argv }
    );
    driver(commandParser);
    
//    std::string buffer;
//    read("/Users/ethanuppal/Library/Mobile Documents/com~apple~CloudDocs/Xcode Projects/floral/floral/proj/main.floral", buffer);
//    Lexer lexer(buffer);
//    std::vector<Token> tokens = lexer.lex();
////    for (const auto &tkn: tokens) tkn.print();
//    return_if_errors(lexer);
//    Parser parser(tokens);
//    File* file = parser.parse();
//    return_if_errors(parser);
//    if (!file) return 1;
//    
//    file->print();
//    file->dump();
    
    return 0;
}
