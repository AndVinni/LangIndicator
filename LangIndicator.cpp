#include "langindicator.h"
#include <vector>
#include <string>
#include <Shellscalingapi.h >
#pragma comment(lib, "Shcore.lib")


// Global variable for access from WinEventProc
LangIndicator* g_instance = nullptr;

static constexpr UINT_PTR FADE_TIMER_ID = 1;
static constexpr UINT_PTR DELAY_TIMER_ID = 100; // delayed start
static constexpr UINT_PTR CARET_FADE_TIMER_ID = 101;  // animation at the caret

// Global low-level keyboard hook for tracking Alt+Shift/Ctrl+Shift
extern HHOOK g_kbHook = nullptr;


LRESULT CALLBACK LowLevelKeyboard(int nCode, WPARAM wp, LPARAM lp)
{
    // Low-level keyboard hook: track Ctrl, Alt and react to Shift+Alt or Shift+Ctrl
    static bool altDown = false;
    static bool ctrlDown = false;

    if (nCode == HC_ACTION)
    {
        auto k = reinterpret_cast<KBDLLHOOKSTRUCT*>(lp);

        // Update the state of Alt and Ctrl
        if (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN)
        {
            if (k->vkCode == VK_LMENU || k->vkCode == VK_RMENU)
                altDown = true;
            if (k->vkCode == VK_LCONTROL || k->vkCode == VK_RCONTROL)
                ctrlDown = true;
        }
        else if (wp == WM_KEYUP || wp == WM_SYSKEYUP)
        {
            if (k->vkCode == VK_LMENU || k->vkCode == VK_RMENU)
                altDown = false;
            if (k->vkCode == VK_LCONTROL || k->vkCode == VK_RCONTROL)
                ctrlDown = false;
        }

        // When pressing Shift we check the combination
        if ((wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN) &&
            (k->vkCode == VK_SHIFT || k->vkCode == VK_LSHIFT || k->vkCode == VK_RSHIFT))
        {
            if (altDown || ctrlDown)
            {
                if (g_instance && g_instance->GetHwnd())
                {
                    g_instance->SetWaitingForClick();
                    SetTimer(g_instance->GetHwnd(), DELAY_TIMER_ID, 300, nullptr);
                }
            }
        }
    }

    return CallNextHookEx(nullptr, nCode, wp, lp);
}


LangIndicator::LangIndicator(const Config* cfg)
    : cfg_(cfg)
    , hwnd_(nullptr)
    , hInst_(nullptr)
    , fadeTimerId_(FADE_TIMER_ID)
    , caretFadeTimerId_(CARET_FADE_TIMER_ID)
    , phase_(Phase::None)
    , currentAlpha_(0)
    , waitingForClick_(true)

{
}

LangIndicator::~LangIndicator()
{
    if (hwnd_) DestroyWindow(hwnd_);

    if (g_kbHook)
    {
        UnhookWindowsHookEx(g_kbHook);
        g_kbHook = nullptr;
    }
    if (hwnd_) DestroyWindow(hwnd_);

}


bool LangIndicator::Init(HINSTANCE hInstance)
{
    hInst_ = hInstance;

    WNDCLASSW wc = {};
    // Turn off the background fill so that there is no white frame
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst_;
    wc.lpszClassName = L"LangIndicatorWindow";
    RegisterClassW(&wc);

    hwnd_ = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW, wc.lpszClassName, nullptr, WS_POPUP,CW_USEDEFAULT, CW_USEDEFAULT,
                            cfg_->width, cfg_->height, nullptr, nullptr, hInst_, this);
    if (!hwnd_) return false;

    SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    HRGN rgn = CreateRoundRectRgn(0, 0, cfg_->width + 1, cfg_->height + 1, 20, 20);
    SetWindowRgn(hwnd_, rgn, TRUE); // hwnd_ — your window handle
    ShowWindow(hwnd_, SW_HIDE);
    RegisterRawInput();
    // Installing a global WH_KEYBOARD_LL hook for Alt+Shift/Ctrl+Shift
    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL,LowLevelKeyboard, hInst_, 0 );
    // optional: nullptr check
    // if (!g_kbHook) { /* log the error */ }
    UpdateLayout();
    return true;
}

void LangIndicator::Run()
{
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void LangIndicator::RegisterRawInput()
{
    RAWINPUTDEVICE rid{ 0 };
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd_;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

void LangIndicator::ShowIndicator()
{
    UpdateLayout();
    currentAlpha_ = 0;
    phase_ = Phase::FadeIn;

    POINT pt;
    GetCursorPos(&pt);
    SetWindowPos(hwnd_, HWND_TOPMOST, pt.x, pt.y, cfg_->width, cfg_->height, SWP_NOACTIVATE);

    // Set the initial transparency
    SetLayeredWindowAttributes(hwnd_, 0, currentAlpha_, LWA_ALPHA);

    // Show the window and disable it for WM_PAINT
    ShowWindow(hwnd_, SW_SHOWNA);
    InvalidateRect(hwnd_, nullptr, TRUE);

    // Let's start the timer
    SetTimer(hwnd_, fadeTimerId_, cfg_->fadeIntervalMs, nullptr);
}

void LangIndicator::Render(HDC dc)
{
    // 1. Fill the background of the rounded rectangle
    RECT rc;
    GetClientRect(hwnd_, &rc);
    HBRUSH hbr = CreateSolidBrush(ParseHexColor(cfg_->bgColor));
    HBRUSH oldBr = (HBRUSH)SelectObject(dc, hbr);
    HPEN hPen = (HPEN)SelectObject(dc, GetStockObject(NULL_PEN));
    RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, 20, 20);
    SelectObject(dc, oldBr);
    DeleteObject(hbr);
    SelectObject(dc, hPen);

    // 2. Draw text
    HFONT font = CreateFontW(cfg_->fontSize, 0, 0, 0, FW_NORMAL, 0, 0, 0,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH, L"Segoe UI");
    HFONT oldF = (HFONT)SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, ParseHexColor(cfg_->textColor));
    DrawTextW(dc, currentLayout_.c_str(), -1, &rc,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, oldF);
    DeleteObject(font);
}

void LangIndicator::UpdateLayout()
{
    // 1. Updating the target flow layout
    POINT pt;
    GetCursorPos(&pt);

    HWND hwndTarget = GetForegroundWindow();
    DWORD threadId = GetWindowThreadProcessId(hwndTarget, nullptr);
 
    // We get HKL for that thread
    HKL hkl = GetKeyboardLayout(threadId);

    // We extract LANGID from it
    LANGID langId = LOWORD(reinterpret_cast<ULONG_PTR>(hkl));

    // Convert LANGID to locale string code, for example "en-EN"
    WCHAR localeName[LOCALE_NAME_MAX_LENGTH] = {};
    if (LCIDToLocaleName(MAKELCID(langId, SORT_DEFAULT), localeName, LOCALE_NAME_MAX_LENGTH, 0))
    {
        // To get a short code ("EN"), take the first two letters after the hyphen:
        std::wstring s(localeName);
        auto pos = s.find(L'-');
        std::wstring primary = s.substr(0, pos);
        // primary is now L"en" — can be converted to uppercase: EN
        for (auto& c : primary)
            c = towupper(c);
        currentLayout_ = primary; // primary == L"EN"
    }
}

void LangIndicator::OnTimer()
{
    // general algorithm fade-in → fade-out, the same for both windows
    if (phase_ == Phase::FadeIn)
    {
        if (currentAlpha_ + cfg_->alphaStep < cfg_->initialAlpha)
        {
            currentAlpha_ += cfg_->alphaStep;
        }
        else
        {
            currentAlpha_ = cfg_->initialAlpha;
            phase_ = Phase::FadeOut;
        }
    }
    else if (phase_ == Phase::FadeOut)
    {
        if (currentAlpha_ > cfg_->alphaStep)
        {
            currentAlpha_ -= cfg_->alphaStep;
        }
        else
        {
            // stop both timers at once
            KillTimer(hwnd_, fadeTimerId_);
            KillTimer(hwnd_, caretFadeTimerId_);
            ShowWindow(hwnd_, SW_HIDE);
            phase_ = Phase::None;
            return;
        }
    }
    SetLayeredWindowAttributes(hwnd_, 0, currentAlpha_, LWA_ALPHA);
}

#ifdef _MSC_VER
__declspec(noinline)
#endif

void LangIndicator::ShowIndicatorAtCaret()
{
    UpdateLayout();
    currentAlpha_ = 0;
    phase_ = Phase::FadeIn;
    
    // 1) get the screen position of the caret
    POINT pt = this->FindCaretPosition();
    
    // If we got zero coordinates, use center of the screen
    if (pt.x == 0 && pt.y == 0)
    {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        pt.x = (screenWidth - cfg_->width) / 2;
        pt.y = (screenHeight - cfg_->height) / 2;
    }
    
    // 2) position the window near the carriage
    SetWindowPos(hwnd_, HWND_TOPMOST, pt.x, pt.y, cfg_->width, cfg_->height, SWP_NOACTIVATE);
    
    // 3) we start the animation in the same way as ShowIndicator()
    SetLayeredWindowAttributes(hwnd_, 0, currentAlpha_, LWA_ALPHA);
    ShowWindow(hwnd_, SW_SHOWNA);
    InvalidateRect(hwnd_, nullptr, TRUE);
    UpdateWindow(hwnd_);
    SetTimer(hwnd_, caretFadeTimerId_, cfg_->fadeIntervalMs, nullptr);
 }

#ifdef _MSC_VER
__declspec(noinline)
#endif

POINT LangIndicator::FindCaretPosition()
{
    // Let's try to get the caret window via GUIThreadInfo
    GUITHREADINFO gi{ sizeof(gi) };

    if (GetGUIThreadInfo(0, &gi) && gi.hwndCaret)
    {
        RECT caretRect = gi.rcCaret;
        
        // MapWindowPoints is generally more reliable than ClientToScreen.
        // It converts the caret rectangle from the client coordinates of the
        // caret's window to screen coordinates.
        MapWindowPoints(gi.hwndCaret, HWND_DESKTOP, (LPPOINT)&caretRect, 2);

        // Position the indicator's top-left corner at the bottom-left of the caret,
        // which is a common placement for IMEs and similar UI elements.
        POINT pt = { caretRect.left, caretRect.bottom };
        return pt;
    }
    
    // Return zero point if we can't get caret position
    return { 0, 0 };
}

LRESULT CALLBACK LangIndicator::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    auto self = reinterpret_cast<LangIndicator*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (msg)
    {
    case WM_INPUT:
    {
        HRAWINPUT hRaw = reinterpret_cast<HRAWINPUT>(lp);
        UINT size = 0;
        if (GetRawInputData(hRaw, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) == 0)
        {
            std::vector<BYTE> buf(size);
            if (GetRawInputData(hRaw, RID_INPUT, buf.data(), &size, sizeof(RAWINPUTHEADER)) == size)
            {
                RAWINPUT* ri = reinterpret_cast<RAWINPUT*>(buf.data());
                if (ri->header.dwType == RIM_TYPEMOUSE && (ri->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN))
                {
                    if (self && self->waitingForClick_)
                    {
                        self->waitingForClick_ = false;
                        PostMessageW(self->hwnd_, WM_SHOW_INDICATOR, 0, 0);
                    }
                }
            }
        }
        return 0;
    }

    case WM_TIMER:
        if (self)
        {   // 1) delayed window launch at the caret
            if (wp == DELAY_TIMER_ID)
            {
                KillTimer(self->hwnd_, DELAY_TIMER_ID);
                self->ShowIndicatorAtCaret();
                return 0;
            }
            // 2) animation of a normal window under the mouse
            if (wp == self->fadeTimerId_)
            {
                self->OnTimer();
                return 0;
            }
            // 3) window animation at the caret
            if (wp == self->caretFadeTimerId_)
            {
                self->OnTimer();
                return 0;
            }
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        if (self)
            self->Render(dc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_SHOW_INDICATOR:
        if (self)
        {
            self->ShowIndicator();
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// Color parser from "#RRGGBB" format strings to COLORREF
extern COLORREF ParseHexColor(const std::wstring& hex)
{
    if (hex.size() == 7 && hex[0] == L'#')
    {
        // We read 2 symbols at a time
        auto hexToByte = [&](int pos)
        {
            std::wstring part = hex.substr(pos, 2);
            return static_cast<BYTE>(std::stoul(part, nullptr, 16));
        };
        BYTE r = hexToByte(1);
        BYTE g = hexToByte(3);
        BYTE b = hexToByte(5);
        return RGB(r, g, b);
    }
    // Foulback - black
    return RGB(0, 0, 0);
}

