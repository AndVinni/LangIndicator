// config.cpp — реализация загрузки/сохранения config.json и автозапуска (W-версии)
#include "config.h"
#include <Windows.h>
#include <fstream>
#include <sstream>
#include <shlwapi.h>   // PathFileExistsW, PathRemoveFileSpecW
#pragma comment(lib, "shlwapi.lib")

static const wchar_t* CONFIG_NAME = L"config.json";

void RegisterAutoRun() {
    HKEY hKey;
    // Создаём/открываем ветку реестра для автозапуска
    RegCreateKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr);
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    RegSetValueExW(hKey, L"LangIndicator", 0, REG_SZ,
        reinterpret_cast<const BYTE*>(path),
        static_cast<DWORD>((wcslen(path) + 1) * sizeof(wchar_t)));
    RegCloseKey(hKey);
}

void Config::LoadOrCreate() {
    wchar_t modulePath[MAX_PATH];
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    PathRemoveFileSpecW(modulePath);
    std::wstring configPath = std::wstring(modulePath) + L"\\" + CONFIG_NAME;

    if (!PathFileExistsW(configPath.c_str())) {
        // Дефолтные значения
        width = 200;
        height = 50;
        fontSize = 24;
        initialAlpha = 192;
        displayTimeMs = 1500;
        fadeTimeMs = 1000;
        bgColor = L"#000000";
        textColor = L"#FFFFFF";

        std::wofstream f(configPath);
        f << L"{\n"
            << L"  \"width\": " << width << L",\n"
            << L"  \"height\": " << height << L",\n"
            << L"  \"fontSize\": " << fontSize << L",\n"
            << L"  \"initialAlpha\": " << initialAlpha << L",\n"
            << L"  \"displayTimeMs\": " << displayTimeMs << L",\n"
            << L"  \"fadeTimeMs\": " << fadeTimeMs << L",\n"
            << L"  \"bgColor\": \"" << bgColor << L"\",\n"
            << L"  \"textColor\": \"" << textColor << L"\"\n"
            << L"}\n";
        f.close();
    }
    else {
        // Простейший парсинг JSON
        std::wifstream f(configPath);
        std::wstringstream ss;
        ss << f.rdbuf();
        std::wstring s = ss.str();
        auto getInt = [&](const std::wstring& key)->int {
            size_t pos = s.find(key);
            if (pos == std::wstring::npos) return 0;
            pos = s.find(L":", pos) + 1;
            return _wtoi(s.c_str() + pos);
            };
        auto getStr = [&](const std::wstring& key)->std::wstring {
            size_t pos = s.find(key);
            if (pos == std::wstring::npos) return L"";
            pos = s.find(L"\"", pos + key.length()) + 1;
            size_t end = s.find(L"\"", pos);
            return s.substr(pos, end - pos);
            };
        width = getInt(L"\"width\"");
        height = getInt(L"\"height\"");
        fontSize = getInt(L"\"fontSize\"");
        initialAlpha = getInt(L"\"initialAlpha\"");
        displayTimeMs = getInt(L"\"displayTimeMs\"");
        fadeTimeMs = getInt(L"\"fadeTimeMs\"");
        bgColor = getStr(L"\"bgColor\"");
        textColor = getStr(L"\"textColor\"");
    }
}
