#include "cipher.h"

std::string Cipher::XOR(std::string message, std::string key)
{
    std::string processedMessage = message;
    int message_length = message.size();
    int key_length = key.size();
    for(int i = 0;i<message_length;i++)
    {
        processedMessage[i] = message[i]^key[i%key_length];
    }

    return processedMessage;
}