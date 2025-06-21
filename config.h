// config.h — working with config.json and autorun
#pragma once
#include <string>
#include <Windows.h>

void RegisterAutoRun();
void UnregisterAutoRun();  // remove from startup

struct Config
{
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
