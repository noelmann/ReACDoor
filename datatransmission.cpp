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
    message = prepareMessageForTransmission(message);
    std::unique_lock<std::mutex> lock(messagesMutex);
    messages.push_back(std::pair<int,std::string>(typeID,message));
}

void DataTransmission::transmitData()
{
    for(;;)
    {
        //Sleep(10);
        std::string type;
        std::string body;
        {
            std::unique_lock<std::mutex> lock(messagesMutex);
            if(messages.empty())
            {
                continue;
            }

            type = std::to_string(messages.front().first);
            body = messages.front().second;
        }


        try
        {
            //http::Request request{"http://echo.free.beeceptor.com"};
            http::Request request{transmissionTargetUrl};
            //http::Request request{"http://raspberrypi:8080"};
            //http::Request request{"http://localhost:8080"};



            const auto response = request.send("POST", body, {
                                                               {"Content-Type", "application/x-www-form-urlencoded"},{"type-ID", type},
                                                             },std::chrono::seconds(httpRequestTimeout_seconds));

            std::string result(response.body.begin(), response.body.end());
            //std::cout << result << '\n';

            if(result == transmissionConfirmation)
            {
                std::unique_lock<std::mutex> lock(messagesMutex);
                messages.pop_front();
            }

        }
        catch (const std::exception& e)
        {
            //std::cerr << "Request failed: " << e.what() << '\n';
        }
    }
}

