#include "langindicator.h"
#include <vector>
#include <string>


// Глобальная переменная для доступа из WinEventProc
LangIndicator* g_instance = nullptr;

static constexpr UINT_PTR FADE_TIMER_ID = 1;
static constexpr UINT_PTR DELAY_TIMER_ID = 100;

// ---------------------------------------------------------------------------+
// Глобальный низкоуровневый клавиатурный хук для отслеживания Alt+Shift/Ctrl+Shift
static HHOOK g_kbHook = nullptr;

LRESULT CALLBACK LowLevelKeyboard(int nCode, WPARAM wp, LPARAM lp)
{
    // Низкоуровневый клавиатурный хук: отслеживаем Ctrl, Alt и реагируем на Shift+Alt или Shift+Ctrl
    static bool altDown = false;
    static bool ctrlDown = false;

    if (nCode == HC_ACTION)
    {
        auto k = reinterpret_cast<KBDLLHOOKSTRUCT*>(lp);

        // Обновляем состояние Alt и Ctrl
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

        // При нажатии Shift проверяем комбинацию
        if ((wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN) &&
            (k->vkCode == VK_SHIFT || k->vkCode == VK_LSHIFT || k->vkCode == VK_RSHIFT))
        {
            if (altDown || ctrlDown)
            {
                if (g_instance && g_instance->GetHwnd())
                {
                    g_instance->SetWaitingForClick();
                    SetTimer(g_instance->GetHwnd(), DELAY_TIMER_ID, 200, nullptr);
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
    // Отключаем фоновую заливку, чтобы не было белой рамки
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
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
    HRGN rgn = CreateRoundRectRgn(0, 0, cfg_->width + 1, cfg_->height + 1, 20, 20);
    SetWindowRgn(hwnd_, rgn, TRUE); // hwnd_ — ваш дескриптор окна
    ShowWindow(hwnd_, SW_HIDE);
    RegisterRawInput();
    // Устанавливаем глобальный WH_KEYBOARD_LL‑хук для Alt+Shift/Ctrl+Shift
    g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL,LowLevelKeyboard, hInst_, 0 );
    // необязательно: проверка на nullptr
    // if (!g_kbHook) { /* логируйте ошибку */ }
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

    // Устанавливаем начальную прозрачность
    SetLayeredWindowAttributes(hwnd_, 0, currentAlpha_, LWA_ALPHA);

    // Показываем окно и инвалидируем для WM_PAINT
    ShowWindow(hwnd_, SW_SHOWNA);
    InvalidateRect(hwnd_, nullptr, TRUE);

    // Стартуем таймер
    SetTimer(hwnd_, fadeTimerId_, cfg_->fadeIntervalMs, nullptr);
}

void LangIndicator::Render(HDC dc)
{
    // 1. Заливаем фон закруглённого прямоугольника
    RECT rc;
    GetClientRect(hwnd_, &rc);
    HBRUSH hbr = CreateSolidBrush(ParseHexColor(cfg_->bgColor));
    HBRUSH oldBr = (HBRUSH)SelectObject(dc, hbr);
    HPEN hPen = (HPEN)SelectObject(dc, GetStockObject(NULL_PEN));
    RoundRect(dc, rc.left, rc.top, rc.right, rc.bottom, 20, 20);
    SelectObject(dc, oldBr);
    DeleteObject(hbr);
    SelectObject(dc, hPen);

    // 2. Рисуем текст
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
    // 1. Обновляем раскладку целевого потока
    POINT pt;
    GetCursorPos(&pt);

    HWND hwndTarget = GetForegroundWindow();
    DWORD threadId = GetWindowThreadProcessId(hwndTarget, nullptr);
 
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
        std::wstring primary = s.substr(0, pos);
        // primary теперь L"ru" — можно привести к верхнему регистру: RU
        for (auto& c : primary)
            c = towupper(c);
        currentLayout_ = primary; // primary == L"RU"
    }
}

void LangIndicator::OnTimer()
{
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
                    //self->ShowIndicator();
                    // Показываем только первый клик после смены раскладки
                    if (self->waitingForClick_)
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
        if (!self) break;

        // Отложенный показ после переключения раскладки
        if (wp == DELAY_TIMER_ID) {
            KillTimer(self->hwnd_, DELAY_TIMER_ID);
            self->ShowIndicator();
            return 0;
        }

        // Фаза fade‑in/fade‑out
        if (wp == self->fadeTimerId_) {
            self->OnTimer();
            return 0;
        }
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        // Делегируем всю отрисовку
        self->Render(dc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_SHOW_INDICATOR:

        self->ShowIndicator();

    return 0;
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
    // Фолбэк — чёрный
    return RGB(0, 0, 0);
}
