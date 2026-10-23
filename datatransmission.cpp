#include "datatransmission.h"
#include "HTTPRequest.hpp"



DataTransmission::DataTransmission() : transmitter(&transmitData,this)
{

}


void DataTransmission::Enqueue(std::string message)
{
    std::unique_lock<std::mutex> lock(messagesMutex);
    messages.push_back(message);
}

void DataTransmission::transmitData()
{
    for(;;)
    {
        Sleep(1000);
    if(messages.empty())
    {
        continue;
    }


        try
        {
            http::Request request{"http://echo.free.beeceptor.com"};
            const std::string body = "|||"+messages.front();

            const auto response = request.send("POST", body, {
                                                               {"Content-Type", "application/x-www-form-urlencoded"},{"messageType","secretSauce"}
                                                             });

            std::string result(response.body.begin(), response.body.end());
            std::cout << result << '\n';

            std::unique_lock<std::mutex> lock(messagesMutex);
            messages.pop_front();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Request failed: " << e.what() << '\n';
        }
    }
}

