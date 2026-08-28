//#include "HiHi.h"
#include <string>
#include <ctype.h>
#include <stdio.h>
#include <iostream>

std::string encode(std::string plain, std::string key);

int main(int argc, char *argv[])
{
    std::string key = argv[1];
    std::string plain = "example";

    printf("Plain: ");
    std::cin >> plain;

    std::string result, begin = encode(plain, key);

    printf("Normal: %s\n", begin);
    printf("Cipher: %s\n", result);
    
}

std::string encode(std::string plain, std::string key)
{
    int len = plain.length();
    std::string encoded = "0";
    encoded = plain;

    for (int i = 0; i < len; i++)
    {
        if (isalpha(encoded[i]))
        {
            if (islower(encoded[i]))
            {
                int pos = encoded[i] - 'a';
                char newChar = tolower(key[pos]);
                encoded[i] = newChar;
            }
            else if (isupper(encoded[i]))
            {
                int pos = encoded[i] - 'A';
                char newChar = toupper(key[pos]);
                encoded[i] = newChar;
            }
        }
    }
    return encoded, plain;
}
