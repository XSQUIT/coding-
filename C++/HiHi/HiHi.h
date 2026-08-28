#pragma once
#include <string>
#include <ctype.h>
#include <stdio.h>
#include <iostream>

namespace HiHi
{
    class sub
    {
        public:
            static std::string encode(std::string plain, std::string key);
            static std::string decode(std::string plain, std::string key);

    };
}
