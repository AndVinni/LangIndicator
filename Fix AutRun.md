Спасибо за дополнение! Теперь контекст полный — и **я вижу конкретную проблему**, из-за которой автозапуск может «пропадать» (точнее, приложение **не запускается вовсе**, а Windows это интерпретирует как «падение при запуске» → отключение автозапуска).

---

### 🚨 **Критическая ошибка: `RegisterAutoRun()` вызывается *до* загрузки конфига, но *после* этого приложение может упасть.**

Давайте разберёмся, почему это **проблема для автозапуска**.

#### 🔥 Сценарий отказа (что происходит на практике):

1. Приложение **запускается при входе в Windows** (через `Run` в реестре).
2. `RegisterAutoRun()` **добавляет ключ в реестр** (или убеждается, что он есть).
3. `cfg.LoadOrCreate()` — **неожиданно падает или зависает** (например, файл конфига повреждён, путь недоступен, антивирус блокирует, путь содержит `%APPDATA%`, но сессия ещё не инициализирована).
4. `indicator.Init()` — **падает** (например, `RegisterRawInputDevices` возвращает `FALSE`, или `CreateWindowExW` не работает в Session 0).
5. `return -1` — приложение **аварийно завершается до появления окна**.

➡️ **Результат для Windows**:
- Приложение запускается → через секунду исчезает (или зависает).
- Windows видит: *«Программа из `Run` не запустилась или упала»*.
- Спустя несколько таких попыток (например, после обновления или смены профиля) Windows **автоматически отключает автозапуск**:
  - либо удаляет ключ из `Run`,
  - либо создаёт ключ `DisableStartupApps` (в некоторых версиях),
  - либо добавляет в `Run` ключ с суффиксом `.disabled` или `@`,
  - либо (реже) — просто ставит флаг "Slow startup" и скрывает из утилит.

✅ **Подтверждение**: откройте `Диспетчер задач → Вкладка "Автозагрузка"`, найдите ваш exe — если там написано *"Отключено — ошибка запуска"* — **это именно та причина**.

---

### 🔍 Почему `cfg.LoadOrCreate()` или `Init()` могут падать при автозапуске?

| Причина | Объяснение |
|--------|------------|
| **Session 0** | При запуске из `Run` приложение может стартовать в **Session 0** (до входа пользователя), где **нет рабочей станции (Winsta0)**, и `GetCursorPos`, `WindowFromPoint`, `GetKeyboardLayout` **не работают** (или блокируются). |
| **Нет доступа к `%APPDATA%`** | Если конфиг лежит в `SHGetFolderPath(CSIDL_APPDATA)`, но **профиль пользователя ещё не загружен**, `Load()` может вернуть ошибку или зависнуть (ожидание сети при roaming-профилях). |
| **Антивирус / Defender** | Блокирует запись в `Run` или `cfg`-файл при первом запуске. |
| **Raw Input в Session 0** | `RegisterRawInputDevices(..., RIDEV_INPUTSINK)` может **не зарегистрироваться** (в Session 0 нет активного пользователя), и `Init()` возвращает `false` → `return -1`. |
| **Очередь сообщений не готова** | До входа пользователя в сессию `GetMessage` может вести себя непредсказуемо. |

---

### ✅ Как исправить: **«Умный автозапуск» + отказоустойчивость**

#### 1. **Перенесите `RegisterAutoRun()` в конец, после успешной инициализации**

Это **обязательно**. Нельзя утверждать, что приложение "запущено корректно", пока оно не запустилось *и* работает.

```cpp
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    // ✅ Сначала загружаем конфиг (если не получится — не будем перезаписывать Run-ключ)
    Config cfg;
    if (!cfg.LoadOrCreate()) {
        // Логируем ошибку в файл или event log — это критично!
        LogError(L"Failed to load config");  // ← обязательно добавьте!
        return -1;
    }

    LangIndicator indicator(&cfg);
    if (!indicator.Init(hInstance)) {
        LogError(L"Failed to initialize indicator");
        return -1;
    }

    // ✅ Только ПОСЛЕ успешного запуска убеждаемся, что автозапуск включён
    RegisterAutoRun();  

    indicator.Run();
    return 0;
}
```

> 📌 **Важно**: `RegisterAutoRun()` должен быть **idempotent** — не удалять другие ключи, не добавлять дважды.

#### 2. **Добавьте обработку Session 0 (предотвращение запуска в ней)**

```cpp
bool IsInSession0() {
    DWORD sessionID = 0;
    ProcessIdToSessionId(GetCurrentProcessId(), &sessionID);
    return sessionID == 0;
}
```

И используйте в `wWinMain`:

```cpp
if (IsInSession0()) {
    // Не запускаемся в Session 0!
    // Но всё равно registering auto-run (если он должен быть),
    // но без инициализации GUI
    RegisterAutoRun();
    return 0;
}
```

#### 3. **Измените `RegisterAutoRun()` — чтобы он не зависел от контекста**

Типичная реализация (с проверкой пути и отсутствием дубликатов):

```cpp
void RegisterAutoRun()
{
    HKEY hKey;
    LONG res = RegOpenKeyExW(HKEY_CURRENT_USER, 
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", 
        0, KEY_SET_VALUE, &hKey);
    if (res != ERROR_SUCCESS) return;

    // Получаем путь к EXE
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);

    // Добавляем кавычки на случай пробелов
    std::wstring cmdLine = L"\"" + std::wstring(exePath) + L"\"";

    // Проверяем, есть ли уже такой ключ — чтобы не перезаписывать лишний раз
    DWORD type, size = MAX_PATH * 2;
    WCHAR existing[MAX_PATH];
    res = RegGetValueW(hKey, nullptr, L"LangIndicator", RRF_RT_ANY, &type, existing, &size);
    if (res == ERROR_SUCCESS && type == REG_SZ && _wcsicmp(existing, cmdLine.c_str()) == 0) {
        RegCloseKey(hKey);
        return; // уже OK
    }

    // Устанавливаем ключ
    RegSetValueExW(hKey, L"LangIndicator", 0, REG_SZ, 
        reinterpret_cast<const BYTE*>(cmdLine.c_str()), 
        (cmdLine.size() + 1) * sizeof(WCHAR));

    RegCloseKey(hKey);
}
```

> 🔍 **Совет**: назовите ключ `L"LangIndicator"` (а не `exe`), чтобы не зависеть от пути и не конфликтовать с другими версиями.

#### 4. **Улучшите обработку ошибок в `LangIndicator::Init()`**

Сейчас вы просто возвращаете `false`. Добавьте логирование:

```cpp
bool LangIndicator::Init(HINSTANCE hInstance)
{
    hInst_ = hInstance;
    // ... (WNDCLASS, RegisterClassW) ...

    if (!RegisterClassW(&wc)) {
        LogError(L"RegisterClassW failed: " + std::to_wstring(GetLastError()));
        return false;
    }

    hwnd_ = CreateWindowExW(...);
    if (!hwnd_) {
        LogError(L"CreateWindowExW failed: " + std::to_wstring(GetLastError()));
        return false;
    }

    // ... и т.д.
}
```

#### 5. **Добавьте таймаут на `cfg.LoadOrCreate()`** (на случай зависаний)

```cpp
// В config.cpp:
bool Config::LoadOrCreate() {
    HANDLE hEvent = CreateEventW(nullptr, TRUE, FALSE, L"ConfigLoadDone");
    if (!hEvent) return false;

    // Запускаем загрузку в потоке
    std::thread([this, hEvent]() {
        try {
            // ... реальная загрузка ...
        } catch (...) {
            SetEvent(hEvent); // чтобы не висеть вечно
        }
    }).detach();

    // Ждём до 2 секунд
    DWORD waitRes = WaitForSingleObject(hEvent, 2000);
    CloseHandle(hEvent);

    return waitRes == WAIT_OBJECT_0;
}
```

---

### 🛠️ Как диагностировать в текущий момент

1. **Откройте «Автозагрузку» в Диспетчере задач** → найдите `LangIndicator` → посмотрите статус.
2. **Включите «Журнал событий» → Приложения** → ищите ошибки от `LangIndicator.exe`.
3. **Добавьте запись в журнал событий из кода**:
   ```cpp
   void LogToEventLog(const std::wstring& msg) {
       HANDLE hEventSource = RegisterEventSourceW(nullptr, L"LangIndicator");
       if (hEventSource) {
           LPCWSTR strings[1] = { msg.c_str() };
           ReportEventW(hEventSource, EVENTLOG_ERROR_TYPE, 0, 1001, nullptr, 1, 0, strings, nullptr);
           DeregisterEventSource(hEventSource);
       }
   }
   ```
   И вызывайте её в `wWinMain` при падении.

---

### ✅ Краткий чек-лист для фикса

| Действие | Статус |
|---------|--------|
| ✅ `RegisterAutoRun()` — **только после успешной инициализации** | 🔴 Текущая ошибка |
| ✅ Обработка `Session 0` | 🔴 Может быть проблемой |
| ✅ `cfg.LoadOrCreate()` — таймаут и исключения | 🟡 Риск зависания |
| ✅ Логирование всех ошибок (файл + Event Log) | 🔴 Нет — вы теряете диагностику |
| ✅ Обработка `WM_ENDSESSION` и корректное завершение | 🟡 Риск "зависания" |

---

Если внедрите эти изменения, **автозапуск перестанет "теряться"** — приложение будет либо запускаться корректно, либо **не запускаться, но не ломать реестр** (и Windows не будет его отключать).

Хочешь — я могу прислать **полный исправленный `main.cpp` с логированием и защитой от Session 0**.