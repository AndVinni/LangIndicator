#pragma once
#include <windows.h> // for HHOOK and KBDLLHOOKSTRUCT
#include <string>
#include "config.h"

constexpr UINT WM_SHOW_INDICATOR = WM_APP + 1;

COLORREF ParseHexColor(const std::wstring& hex);

class LangIndicator
{
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
    const Config* cfg_;         // configuration
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

// Global variable for access from WinEventProc
extern LangIndicator* g_instance;
// Global keyboard hook for catching language switching
extern HHOOK g_kbHook;
