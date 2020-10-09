//
//  main.cpp
//  floral
//
//  Created by Ethan Uppal on 6/28/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "CommandParser.hpp"
void driver(Floral::CommandParser& commandParse);

int main(int argc, const char* argv[]) {
    Floral::CommandParser commandParser (
        { argc, argv }
    );
    driver(commandParser);
    return 0;
}
