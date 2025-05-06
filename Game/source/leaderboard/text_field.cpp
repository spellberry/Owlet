#include "leaderboard/text_field.hpp"

#include <atomic>
#include <iostream>
#include <thread>
#include <windows.h>

HHOOK keyboardHook;
std::atomic<bool> running(false);  // Initially not running
std::string input;
std::atomic<bool> updateString(false);
std::thread keyboardThread;

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT* pKeyboard = (KBDLLHOOKSTRUCT*)lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
        {
            DWORD vkCode = pKeyboard->vkCode;
            bool shiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;
            bool capsLockOn = GetKeyState(VK_CAPITAL) & 0x0001;

            char key;

            if (vkCode == VK_RETURN)
            {
                updateString = true;
                running = false;
                return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
            }

            if (vkCode >= 'A' && vkCode <= 'Z')
            {
                if (shiftPressed || capsLockOn)
                {
                    key = static_cast<char>(vkCode);
                }
                else
                {
                    key = static_cast<char>(vkCode + 32);  // Convert to lowercase
                }
            }
            else
            {
                BYTE keyboardState[256];
                GetKeyboardState(keyboardState);
                if (shiftPressed)
                {
                    keyboardState[VK_SHIFT] = 0x80;
                }
                WCHAR buffer[2];
                int result = ToAscii(vkCode, pKeyboard->scanCode, keyboardState, (LPWORD)buffer, 0);
                if (result == 1)
                {
                    key = static_cast<char>(buffer[0]);
                }
                else
                {
                    key = '\0';
                }
            }

            if (key != '\0')
            {
                if (key == '\b')
                {
                    if (input.size() > 0) input.pop_back();
                }
                else if (input.size() < 16)
                    input = input + key;
                updateString = true;
            }
        }
    }
    return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

void SetHook()
{
    if (!(keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0)))
    {
        std::cerr << "Failed to install hook!" << std::endl;
    }
}

void ReleaseHook() { UnhookWindowsHookEx(keyboardHook); }

void KeyboardHookThread()
{
    SetHook();
    MSG msg;
    while (running.load())
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(10);  // Sleep to prevent high CPU usage
    }
    ReleaseHook();
}

void TextField::StartKeyboardHook()
{
    running = true;
    keyboardThread = std::thread(KeyboardHookThread);
}

void TextField::StopKeyboardHook()
{
    running = false;
    if (keyboardThread.joinable())
    {
        keyboardThread.join();
    }
}

void TextField::SetUpdateString(bool updateStringState) { updateString = updateStringState; }

bool TextField::GetUpdateString() { return updateString; }

std::string TextField::GetTextInput() { return input; }

void TextField::ResetTextInput() { input = ""; }

void TextField::SetRunningState(bool newRunningState) { running = newRunningState; }

bool TextField::GetRunningState() { return running; }
