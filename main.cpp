// Utility for displaying the keyboard layout at the place where you enter text.
// V2.3
// Windows 11, C++ 20
// (C) Vinni, June 2025


// main.cpp — entry point: autostart, load configuration, start indicator

#include "config.h"
#include "langindicator.h"
#include <Windows.h>

HANDLE hThis = NULL;
const wchar_t* szwMutex = L"93509C5Ф41164AД5АЕ70791F1В898А03";


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR lpCmdLine, int)
{
    
    hThis = GetCurrentProcess(); // This one
    HANDLE mutex = CreateMutexExW(0, szwMutex, CREATE_MUTEX_INITIAL_OWNER, READ_CONTROL);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxExW(NULL, L"Application alredy run", L"Warning!", MB_OK, 0);
        return 1;
    }

    // Checking the "-u" key to remove from startup
    if (lpCmdLine && wcsstr(lpCmdLine, L"-u"))
    {
        UnregisterAutoRun();
        return 0;
    }
    RegisterAutoRun();                    // autostart

    Config cfg;                           // configuration
    cfg.LoadOrCreate();

    LangIndicator indicator(&cfg);       // we transfer the config
    g_instance = &indicator;
    if (!indicator.Init(hInstance))      // Window Initialization and Raw Input
        return -1;

    indicator.Run();                      // message cycle
    return 0;
}
