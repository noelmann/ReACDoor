#ifndef CIPHER_H
#define CIPHER_H

#include <iostream>

class Cipher
{
public:
    static std::string XOR(std::string message, std::string key);
};

#endif // CIPHER_H
