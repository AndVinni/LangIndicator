// langindicator.h — объявление класса для отслеживания событий и отрисовки индикатора
#pragma once
#include "config.h"
#include <Windows.h>

#define DISPLAY_TIMER_ID 1
#define FADE_TIMER_ID 2

class LangIndicator
{
public:
    LangIndicator();
    ~LangIndicator();

    void Initialize(HINSTANCE hInstance);
    void Run();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    const Config* cfg_;
    HWND hwnd_;
    HFONT font_;
    HINSTANCE hInstance_;
    UINT_PTR displayTimerId_;
    UINT_PTR fadeTimerId_;
    int currentAlpha_;
    int alphaStep_;
    std::wstring currentLayout_;
    int displayTimeMs_;
    int fadeStepMs_;

    void ShowIndicator();
    void RegisterRawInput(HWND hwnd);
    void UpdateLayout();
};
