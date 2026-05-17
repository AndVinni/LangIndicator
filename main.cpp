// Utility for displaying the keyboard layout at the place where you enter text.
// V2.4
// Windows 11, C++ 20
// (C) Vinni, June 2025


// main.cpp — entry point: autostart, load configuration, start indicator

#include "config.h"
#include "langindicator.h"
#include "logger.h"
#include <Windows.h>
#include <tlhelp32.h>

HANDLE hThis = NULL;
const wchar_t* szwMutex = L"93509C5Ф41164AД5АЕ70791F1В898А03";


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR lpCmdLine, int)
{
    Config cfg;                           // configuration
    cfg.LoadOrCreate();
    g_enableLogging = cfg.logging;        // Set logging status from config

    Log(L"Application starting...");
    // If an uninstall key is provided, terminate other copies of the program
    if (lpCmdLine && wcsstr(lpCmdLine, L"-u"))
    {
        Log(L"Uninstall key found. Terminating other instances and unregistering autorun.");
        // Iterate through processes using Toolhelp to find other instances
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap != INVALID_HANDLE_VALUE)
        {
            PROCESSENTRY32W pe = { sizeof(pe) };
            if (Process32FirstW(hSnap, &pe))
            {
                wchar_t thisExe[MAX_PATH];
                GetModuleFileNameW(nullptr, thisExe, MAX_PATH);
                wchar_t exeName[MAX_PATH];
                wchar_t* pName = wcsrchr(thisExe, L'\\');
                wcscpy_s(exeName, MAX_PATH, pName ? pName + 1 : thisExe);
                do
                {
                    if ((_wcsicmp(pe.szExeFile, exeName) == 0) && (pe.th32ProcessID != GetCurrentProcessId()))
                    {
                        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                        if (hProc)
                        {
                            Log(L"Terminating process ID: " + std::to_wstring(pe.th32ProcessID));
                            TerminateProcess(hProc, 0);
                            CloseHandle(hProc);
                        }
                    }
                } while (Process32NextW(hSnap, &pe));
            }
            CloseHandle(hSnap);
        }
        // Remove from startup
        UnregisterAutoRun();
        Log(L"Uninstall finished. Exiting.");
        return 0;
    }

    hThis = GetCurrentProcess(); // This one
    HANDLE mutex = CreateMutexExW(0, szwMutex, CREATE_MUTEX_INITIAL_OWNER, READ_CONTROL);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        Log(L"Mutex already exists. Application is already running. Exiting.");
        MessageBoxExW(NULL, L"Application already run", L"Warning!", MB_OK, 0);
        return 1;
    }
    
    Log(L"Mutex created. This is the single instance.");


    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    Log(L"Config loaded.");

    LangIndicator indicator(&cfg);       // we transfer the config
    g_instance = &indicator;
    if (!indicator.Init(hInstance))      // Window Initialization and Raw Input
    {
        Log(L"Indicator initialization failed. Exiting.");
        return -1;
    }
    Log(L"Indicator initialized successfully.");

    RegisterAutoRun();                    // autostart
    Log(L"Registered for auto-run.");

    // Wait for the shell (taskbar) to be ready, this is important for autorun
    Log(L"Waiting for Shell_TrayWnd...");
    while (FindWindowW(L"Shell_TrayWnd", NULL) == NULL)
    {
        Sleep(1000);
    }
    Log(L"Shell_TrayWnd found.");

    indicator.ShowIndicatorAtCaret();     // show indicator at startup
    Log(L"Initial indicator shown. Entering message loop.");
    indicator.Run();                      // message cycle
    Log(L"Message loop finished. Exiting application.");
    return 0;
}
