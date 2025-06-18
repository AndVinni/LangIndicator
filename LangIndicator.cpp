#include "langindicator.h"
#include <vector>

static constexpr UINT_PTR DISPLAY_TIMER_ID = 1;
static constexpr UINT_PTR FADE_TIMER_ID = 2;

LangIndicator::LangIndicator(const Config* cfg)
    : cfg_(cfg)
    , hwnd_(nullptr)
    , hInst_(nullptr)
    , displayTimerId_(DISPLAY_TIMER_ID)
    , fadeTimerId_(FADE_TIMER_ID)
    , currentAlpha_(0)
{
}

LangIndicator::~LangIndicator()
{
    if (hwnd_) DestroyWindow(hwnd_);
}

bool LangIndicator::Init(HINSTANCE hInstance)
{
    hInst_ = hInstance;
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst_;
    wc.lpszClassName = L"LangIndicatorWindow";
    RegisterClassW(&wc);

    hwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        nullptr,
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT,
        cfg_->width, cfg_->height,
        nullptr, nullptr, hInst_, this);
    if (!hwnd_) return false;

    SetWindowLongPtr(hwnd_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(hwnd_, SW_HIDE);
    RegisterRawInput();
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
    currentAlpha_ = cfg_->initialAlpha;
    POINT pt; GetCursorPos(&pt);
    SetWindowPos(hwnd_, HWND_TOPMOST, pt.x, pt.y,
        cfg_->width, cfg_->height,
        SWP_NOACTIVATE);
    SetLayeredWindowAttributes(hwnd_, 0, currentAlpha_, LWA_ALPHA);
    ShowWindow(hwnd_, SW_SHOWNA);
    UpdateWindow(hwnd_);
    SetTimer(hwnd_, displayTimerId_, cfg_->displayTimeMs, nullptr);
}

void LangIndicator::UpdateLayout()
{
    wchar_t name[KL_NAMELENGTH] = {};
    if (GetKeyboardLayoutNameW(name)) currentLayout_ = name;
    else currentLayout_ = L"??";
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
                if (ri->header.dwType == RIM_TYPEMOUSE &&
                    (ri->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN))
                {
                    self->ShowIndicator();
                }
            }
        }
        return 0;
    }
    //case WM_INPUTLANGCHANGE:
    //    self->currentLayout_= std::wstring(reinterpret_cast<LPCWSTR>(lp));
    //    self->ShowIndicator();
    //    return 0;
    case WM_SETTINGCHANGE:
        if (wp==0 && (lp && wcscmp(reinterpret_cast<LPCWSTR>(lp), L"intl") == 0))
        {
            //self->currentLayout_ = std::wstring(reinterpret_cast<LPCWSTR>(lp));
            self->ShowIndicator();
        }
        return 0;
    case WM_TIMER:
        if (!self) break;
        if (wp == self->displayTimerId_)
        {
            KillTimer(hwnd, self->displayTimerId_);
            SetTimer(hwnd, self->fadeTimerId_, self->cfg_->fadeIntervalMs, nullptr);
        }
        else if (wp == self->fadeTimerId_) {
            if (self->currentAlpha_ > self->cfg_->alphaStep)
            {
                self->currentAlpha_ -= self->cfg_->alphaStep;
                SetLayeredWindowAttributes(hwnd, 0, self->currentAlpha_, LWA_ALPHA);
            }
            else
            {
                KillTimer(hwnd, self->fadeTimerId_);
                ShowWindow(hwnd, SW_HIDE);
            }
        }
        return 0;
    case WM_PAINT:
    {
        if (!self) break;
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        SetBkMode(dc, TRANSPARENT);
        COLORREF textColor = /* парсер cfg_->textColor */ RGB(255, 255, 255);
        SetTextColor(dc, textColor);
        HFONT font = CreateFontW(self->cfg_->fontSize, 0, 0, 0, FW_NORMAL, 0, 0, 0,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, L"Segoe UI");
        SelectObject(dc, font);
        RECT rc; GetClientRect(hwnd, &rc);
        DrawTextW(dc, self->currentLayout_.c_str(), -1, &rc,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        DeleteObject(font);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}
