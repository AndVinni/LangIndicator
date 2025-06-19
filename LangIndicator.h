#pragma once
#include <windows.h>
#include <string>
#include "config.h"

COLORREF ParseHexColor(const std::wstring& hex);

class LangIndicator {
public:
    explicit LangIndicator(const Config* cfg);
    ~LangIndicator();

    bool Init(HINSTANCE hInstance);
    void Run();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

private:
    const Config* cfg_;         // конфигурация
    HWND hwnd_;
    HINSTANCE hInst_;
    enum class Phase { None, FadeIn, FadeOut } phase_;
    UINT_PTR fadeTimerId_;
    BYTE currentAlpha_;
    std::wstring currentLayout_;

    void RegisterRawInput();
    void ShowIndicator();
    void UpdateLayout();
    void OnTimer();
};