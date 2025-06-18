// main.cpp — точка входа: автозапуск, загрузка конфигурации, запуск индикатора
#include "config.h"
#include "langindicator.h"
#include <Windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    RegisterAutoRun();                    // автозапуск

    Config cfg;                           // конфигурация
    cfg.LoadOrCreate();

    LangIndicator indicator(&cfg);       // передаём конфиг
    if (!indicator.Init(hInstance))      // инициализация окна и Raw Input
        return -1;

    indicator.Run();                      // цикл сообщений
    return 0;
}
