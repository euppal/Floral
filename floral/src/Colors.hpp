//
//  Colors.hpp
//  colors2
//
//  Created by Ethan Uppal on 9/7/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Colors_hpp
#define Colors_hpp

#define COLOR_CODE_RESET "\e[0;0m"

#include <iostream>
#include <string>

enum class Color {
    reset = 0,
    black = 30,
    red,
    green,
    yellow,
    blue,
    magenta,
    cyan,
    white = 37,
    backgroundBlack = 40,
    backgroundRed,
    backgroundGreen,
    backgroundYellow,
    backgroundBlue,
    backgroundMagenta,
    backgroundCyan,
    backgroundWhite = 47,
    bold = 1,
    unbold = 2
};

struct ColoredStream {
    ColoredStream() {}
    
    int foreground, background = 0;
    bool isBold = false;
    bool resetAutomatically = false;
    
    friend ColoredStream& operator <<(ColoredStream& stream, Color color);
    friend ColoredStream& operator <<(ColoredStream& stream, const std::string& str);
    friend ColoredStream& operator <<(ColoredStream& stream, char c);
};


#endif /* Colors_hpp */
