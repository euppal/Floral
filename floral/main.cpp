//
//  main.cpp
//  floral
//
//  Created by Ethan Uppal on 6/28/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//
//#include "driver.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "CommandParser.hpp"
#include "File IO.hpp"
using namespace Floral;

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
    //driver(commandParser);

    Token::invalid = new Token(TokenLoc::zero, TokenType::invalid, "");
    std::string buffer;
    read("/Users/ethanuppal/Library/Mobile Documents/com~apple~CloudDocs/Xcode Projects/floral/floral/proj/main.floral", buffer);
    v2::Preprocessor preprocessor(buffer, "main.floral");
    preprocessor.preprocess();
    return_if_errors(preprocessor)
    std::cout << preprocessor.preprocessedSource() << '\n';
//    std::vector<Token> tokens = lexer.lex();
//    for (const auto &tkn: tokens) tkn.print();
//    return_if_errors(lexer);
//    Parser parser(tokens);
//    File* file = parser.parse();
//    return_if_errors(parser);
//    if (!file) return 1;
//
//    file->print();
//    file->dump();
    static_assert(alignof(int) == 4, "oh no!");
    delete Token::invalid;
    return 0;
}
