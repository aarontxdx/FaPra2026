#pragma once

#include <iostream>

namespace Debug
{
    inline void DebugBreak()
    {
        std::cout << "\n=== Enter zum Fortfahren ===";
        std::cin.get();
    }
}