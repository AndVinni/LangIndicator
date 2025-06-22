// Utility for displaying the keyboard layout at the place where you enter text.
// V2.4
// Windows 11, C++ 20
// (C) Vinni, June 2025


// main.cpp — entry point: autostart, load configuration, start indicator

#include "config.h"
#include "langindicator.h"
#include <Windows.h>
#include <tlhelp32.h>

HANDLE hThis = NULL;
const wchar_t* szwMutex = L"93509C5Ф41164AД5АЕ70791F1В898А03";


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR lpCmdLine, int)
{
    // If an uninstall key is provided, terminate other copies of the program
    if (lpCmdLine && wcsstr(lpCmdLine, L"-u"))
    {
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
        return 0;
    }

    hThis = GetCurrentProcess(); // This one
    HANDLE mutex = CreateMutexExW(0, szwMutex, CREATE_MUTEX_INITIAL_OWNER, READ_CONTROL);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBoxExW(NULL, L"Application alredy run", L"Warning!", MB_OK, 0);
        return 1;
    }

    RegisterAutoRun();                    // autostart

    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    Config cfg;                           // configuration
    cfg.LoadOrCreate();

    LangIndicator indicator(&cfg);       // we transfer the config
    g_instance = &indicator;
    if (!indicator.Init(hInstance))      // Window Initialization and Raw Input
        return -1;

    indicator.Run();                      // message cycle
    return 0;
}
