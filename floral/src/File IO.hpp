//
//  File IO.hpp
//  floral
//
//  Created by Ethan Uppal on 6/29/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef File_IO_h
#define File_IO_h

#include <string>

namespace Floral {
    void read(const std::string path, std::string &result);
    void write(const std::string path, const std::string &contents);
}

#endif /* Files_h */
