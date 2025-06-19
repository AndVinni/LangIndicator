#include "langindicator.h"
#include <vector>

static constexpr UINT_PTR FADE_TIMER_ID = 1;

LangIndicator::LangIndicator(const Config* cfg)
    : cfg_(cfg)
    , hwnd_(nullptr)
    , hInst_(nullptr)
    , fadeTimerId_(FADE_TIMER_ID)
    , phase_(Phase::None)
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
    //UpdateLayout();
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
    //currentAlpha_ = cfg_->initialAlpha;
    currentAlpha_ = 0;
    POINT pt; GetCursorPos(&pt);
    SetWindowPos(hwnd_, HWND_TOPMOST, pt.x, pt.y, cfg_->width, cfg_->height, SWP_NOACTIVATE);
    // Устанавливаем регион окна с закруглёнными краями
    HRGN hRgn = CreateRoundRectRgn(0, 0, cfg_->width, cfg_->height, 20, 20);
    SetWindowRgn(hwnd_, hRgn, TRUE);
    SetLayeredWindowAttributes(hwnd_, 0, currentAlpha_, LWA_ALPHA);
    ShowWindow(hwnd_, SW_SHOWNA);
    // Сбросить область для перерисовки
    InvalidateRect(hwnd_, nullptr, TRUE);
    UpdateWindow(hwnd_);
    //SetTimer(hwnd_, displayTimerId_, cfg_->displayTimeMs, nullptr);
    phase_ = Phase::FadeIn;
    SetTimer(hwnd_, fadeTimerId_, cfg_->fadeIntervalMs, nullptr);
}

void LangIndicator::UpdateLayout()
{
    // 1. Обновляем раскладку целевого потока
    POINT pt;
    GetCursorPos(&pt);

    // Определяем окно под курсором
    HWND hwndTarget = WindowFromPoint(pt);
    DWORD processId = 0;
    DWORD threadId = GetWindowThreadProcessId(hwndTarget, &processId);

    // Получаем HKL для того потока
    HKL hkl = GetKeyboardLayout(threadId);

    // Из него извлекаем LANGID
    LANGID langId = LOWORD(reinterpret_cast<ULONG_PTR>(hkl));

    // Переводим LANGID в строковый код локали, например "ru-RU"
    WCHAR localeName[LOCALE_NAME_MAX_LENGTH] = {};
    if (LCIDToLocaleName(MAKELCID(langId, SORT_DEFAULT), localeName, LOCALE_NAME_MAX_LENGTH, 0))
    {
        // Получить короткий код ("RU"), берём первые две буквы после дефиса:
        std::wstring s(localeName);
        auto pos = s.find(L'-');
        //(pos != std::wstring::npos) ? s.substr(0, pos) : s; // на всякий случай
        std::wstring primary = s.substr(0, pos);

        // primary теперь L"ru" — можно привести к верхнему регистру: RU
        for (auto& c : primary)
            c = towupper(c);
        currentLayout_ = primary; // primary == L"RU"

    }

}

void LangIndicator::OnTimer() {
    if (phase_ == Phase::FadeIn) {
        if (currentAlpha_ + cfg_->alphaStep < cfg_->initialAlpha) {
            currentAlpha_ += cfg_->alphaStep;
        }
        else {
            currentAlpha_ = cfg_->initialAlpha;
            phase_ = Phase::FadeOut;
        }
    }
    else if (phase_ == Phase::FadeOut) {
        if (currentAlpha_ > cfg_->alphaStep) {
            currentAlpha_ -= cfg_->alphaStep;
        }
        else {
            KillTimer(hwnd_, fadeTimerId_);
            ShowWindow(hwnd_, SW_HIDE);
            phase_ = Phase::None;
            return;
        }
    }
    SetLayeredWindowAttributes(hwnd_, 0, currentAlpha_, LWA_ALPHA);
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
            self->ShowIndicator();
        }
        return 0;
    case WM_TIMER:
        if (self && wp == self->fadeTimerId_)
            self->OnTimer();
        return 0;
    case WM_PAINT:
    {
        if (!self) break;
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);

        // Сначала затираем фон
        RECT rc;
        GetClientRect(hwnd, &rc);
        COLORREF bgRef = ParseHexColor(self->cfg_->bgColor);
        // Создаём кисть для фона
        HBRUSH hbr = CreateSolidBrush(ParseHexColor(self->cfg_->bgColor));
        HBRUSH oldBrush = (HBRUSH)SelectObject(dc, hbr);
        // Убираем контур
        HPEN hPen = (HPEN)SelectObject(dc, GetStockObject(NULL_PEN));
        // Рисуем закруглённый прямоугольник
        RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, 20, 20);
        // Восстанавливаем объекты
        SelectObject(dc, oldBrush);
        DeleteObject(hbr);
        SelectObject(dc, hPen);

        // Создаём и устанавливаем шрифт
        HFONT font = CreateFontW(self->cfg_->fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH, L"Segoe UI");
        SelectObject(dc, font);

        // Убираем фон текста и устанавливаем белый цвет
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, ParseHexColor(self->cfg_->textColor));
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

// Парсер цвета из строк формата "#RRGGBB" в COLORREF
static COLORREF ParseHexColor(const std::wstring& hex)
{
    if (hex.size() == 7 && hex[0] == L'#')
    {
        // Читаем по 2 символа
        auto hexToByte = [&](int pos) {
            std::wstring part = hex.substr(pos, 2);
            return static_cast<BYTE>(std::stoul(part, nullptr, 16));
            };
        BYTE r = hexToByte(1);
        BYTE g = hexToByte(3);
        BYTE b = hexToByte(5);
        return RGB(r, g, b);
    }
    // Фолбэк — чёрный
    return RGB(0, 0, 0);
}
