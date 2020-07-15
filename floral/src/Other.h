//
//  Other.h
//  floral
//
//  Created by Ethan Uppal on 7/14/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Other_h
#define Other_h

namespace Floral {
    void _setup() {
        Token::invalid = new Token({0, 0}, TokenType::invalid, "");
    }
    void _free() {
        delete Token::invalid;
    }
}

#endif /* Other_h */
