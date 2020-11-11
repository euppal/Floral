//
//  Timer.hpp
//  floral
//
//  Created by Ethan Uppal on 7/3/20.
//  Copyright © 2020 Ethan Uppal. All rights reserved.
//

#ifndef Timer_h
#define Timer_h

/*
 * The folliwng class ius taken from https://www.learncpp.com/cpp-tutorial/8-16-timing-your-code/
 */
#include <chrono> // for std::chrono functions
 
class Timer
{
private:
    // Type aliases to make accessing nested type easier
    using clock_t = std::chrono::high_resolution_clock;
    using second_t = std::chrono::duration<double, std::ratio<1>>;
    
    std::chrono::time_point<clock_t> m_beg;
 
public:
    Timer() : m_beg(clock_t::now())
    {
    }
    
    void reset()
    {
        m_beg = clock_t::now();
    }
    
    double elapsed() const
    {
        return std::chrono::duration_cast<second_t>(clock_t::now() - m_beg).count();
    }
};

#endif /* Timer_h */
