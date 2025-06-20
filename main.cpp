// main.cpp — точка входа: автозапуск, загрузка конфигурации, запуск индикатора
#include "config.h"
#include "langindicator.h"
#include <Windows.h>


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR lpCmdLine, int)
{
    // Проверка ключа "-u" для удаления из автозагрузки
    if (lpCmdLine && wcsstr(lpCmdLine, L"-u"))
    {
        UnregisterAutoRun();
        return 0;
    }
    RegisterAutoRun();                    // автозапуск

    Config cfg;                           // конфигурация
    cfg.LoadOrCreate();

    LangIndicator indicator(&cfg);       // передаём конфиг
    g_instance = &indicator;
    if (!indicator.Init(hInstance))      // инициализация окна и Raw Input
        return -1;

    indicator.Run();                      // цикл сообщений
    return 0;
}
