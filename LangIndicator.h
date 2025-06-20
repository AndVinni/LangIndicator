#pragma once
#include <windows.h> // для HHOOK и KBDLLHOOKSTRUCT
#include <string>
#include "config.h"

constexpr UINT WM_SHOW_INDICATOR = WM_APP + 1;

COLORREF ParseHexColor(const std::wstring& hex);

class LangIndicator {
public:
    explicit LangIndicator(const Config* cfg);
    ~LangIndicator();

    bool Init(HINSTANCE hInstance);
    void Run();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    void ShowIndicator();
    void Render(HDC dc);
    HWND GetHwnd() const { return hwnd_; }
    void SetWaitingForClick() { waitingForClick_ = true; }


private:
    const Config* cfg_;         // конфигурация
    HWND hwnd_;
    HINSTANCE hInst_;
    enum class Phase { None, FadeIn, FadeOut } phase_;
    UINT_PTR fadeTimerId_;
    BYTE currentAlpha_;
    std::wstring currentLayout_;
    bool waitingForClick_;

    void RegisterRawInput();
    void UpdateLayout();
    void OnTimer();
};

// Глобальная переменная для доступа из WinEventProc
extern LangIndicator* g_instance;
// Глобальный хук клавиатуры для ловли переключения языка
extern HHOOK g_kbHook;
