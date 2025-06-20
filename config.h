// config.h — работа с config.json и автозапуск
#pragma once
#include <string>
#include <Windows.h>

void RegisterAutoRun();
void UnregisterAutoRun();  // удаление из автозагрузки

struct Config {
    int width;
    int height;
    int fontSize;
    BYTE initialAlpha;
    /*int displayTimeMs;*/
    int fadeIntervalMs;
    BYTE alphaStep;
    std::wstring bgColor;
    std::wstring textColor;

    void LoadOrCreate();
};