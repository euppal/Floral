//
//  Colors.cpp
//  colors2
//
//  Created by Ethan Uppal on 9/7/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#include "Colors.hpp"

ColoredStream& operator <<(ColoredStream& stream, Color color) {
    const int value = static_cast<int>(color);
    if (color == Color::reset) {
        stream.foreground = 0;
        stream.background = 0;
        stream.isBold = false;
        std::cout << COLOR_CODE_RESET;
        return stream;
    }
    else if (color == Color::bold) {
        stream.isBold = true;
        return stream;
    }
    else if (color == Color::unbold) {
        stream.isBold = false;
        return stream;
    }
    
    if (value >= 30 && value <= 37) stream.foreground = value;
        else if (value >= 40 && value <= 47) stream.background = value;
            
            return stream;
}

ColoredStream& operator <<(ColoredStream& stream, const std::string& str) {
    
    if (stream.foreground) {
        std::string code;
        if (stream.background) {
            code = "\e[" + std::to_string(stream.foreground) + ';' + std::to_string(stream.background) + (stream.isBold ? ";1m" : "m");
        } else {
            code = "\e[" + std::to_string(stream.foreground) + (stream.isBold ? ";1m" : "m");
        }
        std::cout << code;
    }
        
    std::cout << str;
    if (stream.resetAutomatically) {
        std::cout << COLOR_CODE_RESET;
        stream.foreground = 0;
        stream.background = 0;
    }
    return stream;
}
        
ColoredStream& operator <<(ColoredStream& stream, char c) {
    if (stream.foreground) {
        std::string code;
        if (stream.background) {
            code = "\e[" + std::to_string(stream.foreground) + ';' + std::to_string(stream.background) + (stream.isBold ? ";1m" : "m");
        } else {
            code = "\e[" + std::to_string(stream.foreground) + (stream.isBold ? ";1m" : "m");
        }
        std::cout << code;
    }
        
    std::cout << c;
    if (stream.resetAutomatically) {
        std::cout << COLOR_CODE_RESET;
        stream.foreground = 0;
        stream.background = 0;
    }
    return stream;
}
        
