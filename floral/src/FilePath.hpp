//
//  FilePath.hpp
//  floral
//
//  Created by Ethan Uppal on 7/4/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef FilePath_hpp
#define FilePath_hpp

#include <string>
#include <vector>

namespace Floral {
    struct FilePath {
        struct Component {
            int start, len;
        };
        std::string _path;
        std::vector<Component> components;
        size_t offset{};
        
        FilePath(const std::string &path): _path(path) {
            int temp {-1};
            for (int i {}; i < path.size(); ++i) {
                if (path[i] == '/' || i+1 == path.size()) {
                    if (temp == -1)
                        temp = i;
                    else {
                        Component comp { temp, i-temp+1 };
                        components.push_back(comp);
                        temp = i;
                    }
                }
            }
        }
        
        std::string last() {
            if (components.empty()) return _path;
            return _path.substr(components.back().start, components.back().len);
        }
        void drop() {
            _path.erase(_path.size() - components.back().len);
            components.pop_back();
        }
        void keepLast(size_t n) {
            if (components.size() - offset > n) {
                const auto newLast {components[components.size() - n - 1]};
                _path.erase(components.front().start, newLast.start + newLast.len);
                offset++;
            } else {
                _path.clear();
                components.clear();
            }
        }
        std::string path() {
            return _path;
        }
        size_t componentCount() {
            return components.size();
        }
    };
}

/*
 * Floral::FilePath filePath { "/path/to/my/file.txt" };
 * std::cout << filePath.last() << '\n';
 * filePath.drop();
 * std::cout << filePath.last() << '\n';
 * std::cout << filePath.path() << '\n';
 * std::cout << filePath.componentCount() << '\n';
 */

#endif /* FilePath_hpp */
