#pragma once
#include <string>

// Регистрирует приложение в автозагрузке (WinAPI Unicode)
void RegisterAutoRun();

struct Config {
    int width;
    int height;
    int fontSize;
    int initialAlpha;
    int displayTimeMs;
    int fadeTimeMs;
    std::wstring bgColor;
    std::wstring textColor;

    // Загружает или создаёт config.json
    void LoadOrCreate();
};

