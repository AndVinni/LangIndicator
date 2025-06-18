// langindicator.cpp — реализация отслеживания событий и отрисовки
#include "langindicator.h"
#include <string>
#include <vector>

static LangIndicator* g_instance = nullptr;

LangIndicator::LangIndicator()
    : hwnd_(nullptr),
    hInstance_(nullptr),
    displayTimerId_(1),
    fadeTimerId_(2),
    currentAlpha_(255),
    alphaStep_(15),
    displayTimeMs_(1000),
    fadeStepMs_(50)
{

    g_instance = this;
}

void LangIndicator::Initialize(HINSTANCE hInstance)
{
    // Регистрируем класс окна Unicode
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"LangIndicatorWindow";
    RegisterClassW(&wc);

    // Создаём прозрачное окно без рамки
    hwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        L"LangIndicatorWindow",
        nullptr,
        WS_POPUP,
        0, 0, cfg_->width, cfg_->height,
        nullptr, nullptr, hInstance, nullptr);
    // Регистрация при инициализации
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;                // Generic Desktop Controls
    rid.usUsage = 0x02;                // Mouse
    rid.dwFlags = RIDEV_INPUTSINK;     // получать, даже если не в фокусе
    rid.hwndTarget = hwnd_;                // наше окно
    RegisterRawInputDevices(&rid, 1, sizeof(rid));

    ShowWindow(hwnd_, SW_HIDE);  // явно скрываем
    UpdateWindow(hwnd_);

    // Создаём шрифт
    font_ = CreateFontW(
        cfg_->fontSize, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");


}

LRESULT CALLBACK LangIndicator::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        // ───────────────────────────────────────────────────────────────
        // 1) Событие Raw Input (микро‑события мыши)
        // ───────────────────────────────────────────────────────────────
    case WM_INPUT:
    {
        // lp содержит HRAWINPUT
        HRAWINPUT hRaw = reinterpret_cast<HRAWINPUT>(lp);

        // узнаём размер данных
        UINT size = 0;
        if (GetRawInputData(hRaw, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) == UINT(-1))
            break;
        std::vector<BYTE> buffer(size);
        if (GetRawInputData(hRaw, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size)
            break;

        RAWINPUT* ri = reinterpret_cast<RAWINPUT*>(buffer.data());
        if (ri->header.dwType == RIM_TYPEMOUSE)
        {
            USHORT flags = ri->data.mouse.usButtonFlags;
            if (flags & RI_MOUSE_LEFT_BUTTON_DOWN)
            {
                // координаты клика (относительно последнего положения)
                LONG dx = ri->data.mouse.lLastX;
                LONG dy = ri->data.mouse.lLastY;
                // показываем индикатор на текущей позиции курсора
                ShowIndicator();
            }
        }
        return 0;
    }

    // ───────────────────────────────────────────────────────────────
    // 2) Смена раскладки через стандартное сообщение
    // ───────────────────────────────────────────────────────────────
    case WM_INPUTLANGCHANGE:
    {
        // wp — флаг смены (обычно 0), lp — HKL новой раскладки
        HKL newLayout = reinterpret_cast<HKL>(lp);

        // Для отображения можно получить человеко‑читаемое имя:
        wchar_t name[KL_NAMELENGTH] = {};
        GetKeyboardLayoutNameW(name);

        ShowIndicator();
        return 0;
    }

    // ───────────────────────────────────────────────────────────────
    // 3) Альтернативная ловушка смен системных языковых настроек
    // ───────────────────────────────────────────────────────────────
    case WM_SETTINGCHANGE:
    {
        // lp указывает на NUL‑терминированную строку, например L"intl"
        if (lp != 0 && wcscmp(reinterpret_cast<LPCWSTR>(lp), L"intl") == 0)
        {
            ShowIndicator();
        }
        return 0;
    }

    // ───────────────────────────────────────────────────────────────
    // 4) Отрисовка индикатора
    // ───────────────────────────────────────────────────────────────
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        // TODO: здесь ваш код рисования с текущей альфой и текстом
        EndPaint(hwnd, &ps);
        return 0;
    }

    // ───────────────────────────────────────────────────────────────
    // 5) Таймеры отображения и исчезновения
    // ───────────────────────────────────────────────────────────────
    case WM_TIMER:
    {
        UINT_PTR id = wp;  // идентификатор таймера
        if (id == DISPLAY_TIMER_ID)
        {
            // закончилось время показа — запускаем fade‑out
            KillTimer(hwnd, DISPLAY_TIMER_ID);
            fadeTimerId_ = FADE_TIMER_ID;
            SetTimer(hwnd, fadeTimerId_, TIMER_INTERVAL_MS, nullptr);
        }
        else if (id == FADE_TIMER_ID)
        {
            // уменьшаем прозрачность
            currentAlpha_ -= alphaStep_;
            if (currentAlpha_ <= 0)
            {
                KillTimer(hwnd, FADE_TIMER_ID);
                ShowWindow(hwnd, SW_HIDE);
            }
            else
            {
                InvalidateRect(hwnd, nullptr, TRUE);
            }
        }
        return 0;
    }

    // ───────────────────────────────────────────────────────────────
    // 6) Всё остальное — дефолтная обработка
    // ───────────────────────────────────────────────────────────────
    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}


void LangIndicator::ShowIndicator()
{
    POINT pt;
    GetCursorPos(&pt);
    SetWindowPos(wnd_, HWND_TOPMOST, pt.x, pt.y,
        cfg_->width, cfg_->height, SWP_NOACTIVATE);

    currentAlpha_ = cfg_->initialAlpha;
    ShowWindow(wnd_, SW_SHOWNA);
    InvalidateRect(wnd_, nullptr, TRUE);

    // пауза перед fade‑out
    displayTimerId_ = 1;
    SetTimer(wnd_, fadeTimerId_, cfg_->displayTimeMs, nullptr);

}
