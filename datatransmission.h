#ifndef DATATRANSMISSION_H
#define DATATRANSMISSION_H

#include <deque>
#include <iostream>
#include <thread>
#include <mutex>
#include "cipher.h"
#include "base64.h"

class DataTransmission
{
private:
    std::string key = "secretkey";
    std::thread transmitter;
    std::deque<std::pair<int,std::string>> messages;
    std::mutex messagesMutex;
    std::string prepareMessageForTransmission(std::string message);
    void transmitData();
public:
    DataTransmission();
    void Enqueue(int typeID, std::string message);
};

#endif // DATATRANSMISSION_H
