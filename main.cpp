// main.cpp — точка входа, регистрация автозагрузки и запуск цикла обработки событий
#include "config.h"
#include "langindicator.h"
#include <Windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    // 1. Проверяем и создаём запись в автозагрузке
    RegisterAutoRun();  // в config.cpp использует RegCreateKeyExW/RegSetValueExW

    // 2. Загружаем конфигурацию
    Config cfg;
    cfg.LoadOrCreate();

    // 3. Инициализируем индикатор
    LangIndicator indicator(&cfg);
    indicator.Initialize(hInstance);

    // 4. Запускаем цикл обработки сообщений Win32
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

