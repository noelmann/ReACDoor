#include "datatransmission.h"
#include "HTTPRequest.hpp"



DataTransmission::DataTransmission() : transmitter(&transmitData,this)
{

}

std::string DataTransmission::prepareMessageForTransmission(std::string message)
{
    return base64_encode(Cipher::XOR(message,key));

}


void DataTransmission::Enqueue(int typeID, std::string message)
{
    std::unique_lock<std::mutex> lock(messagesMutex);
    messages.push_back(std::pair<int,std::string>(typeID,prepareMessageForTransmission(message)));
}

void DataTransmission::transmitData()
{
    for(;;)
    {
        Sleep(10);
    if(messages.empty())
    {
        continue;
    }


        try
        {
            //http::Request request{"http://echo.free.beeceptor.com"};
            http::Request request{"http://reacdoormaster:8080"};
            //http::Request request{"http://raspberrypi:8080"};
            //http::Request request{"http://localhost:8080"};
            const std::string type = std::to_string(messages.front().first);
            const std::string body = messages.front().second;


            const auto response = request.send("POST", body, {
                                                               {"Content-Type", "application/x-www-form-urlencoded"},{"type-ID", type}
                                                             });

            std::string result(response.body.begin(), response.body.end());
            //std::cout << result << '\n';

            std::unique_lock<std::mutex> lock(messagesMutex);
            messages.pop_front();
        }
        catch (const std::exception& e)
        {
            //std::cerr << "Request failed: " << e.what() << '\n';
        }
    }
}

