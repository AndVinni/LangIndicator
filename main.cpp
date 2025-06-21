// main.cpp — entry point: autostart, load configuration, start indicator
#include "config.h"
#include "langindicator.h"
#include <Windows.h>


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR lpCmdLine, int)
{
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
