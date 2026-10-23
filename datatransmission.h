#ifndef DATATRANSMISSION_H
#define DATATRANSMISSION_H

#include <deque>
#include <iostream>
#include <thread>
#include <mutex>

class DataTransmission
{
private:

    std::thread transmitter;
    std::deque<std::string> messages;
    std::mutex messagesMutex;
    void transmitData();
public:
    DataTransmission();
    void Enqueue(std::string message);
};

#endif // DATATRANSMISSION_H
