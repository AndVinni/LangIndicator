// config.cpp — реализация Config::LoadOrCreate и RegisterAutoRun
#include "config.h"
#include <Windows.h>
#include <shlwapi.h>
#include <fstream>
#include <sstream>
#pragma comment(lib, "shlwapi.lib")

static const wchar_t* CONFIG_NAME = L"config.json";

void RegisterAutoRun() {
    HKEY hKey;
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
    wchar_t modPath[MAX_PATH];
    GetModuleFileNameW(nullptr, modPath, MAX_PATH);
    PathRemoveFileSpecW(modPath);
    std::wstring cfgPath = std::wstring(modPath) + L"\\" + CONFIG_NAME;

    if (!PathFileExistsW(cfgPath.c_str())) {
        width = 200; height = 50; fontSize = 24;
        initialAlpha = 192; displayTimeMs = 1500;
        fadeIntervalMs = 30; alphaStep = 5;
        bgColor = L"#000000"; textColor = L"#FFFFFF";
        std::wofstream f(cfgPath);
        f << L"{\n"
            << L"  \"width\": " << width << L",\n"
            << L"  \"height\": " << height << L",\n"
            << L"  \"fontSize\": " << fontSize << L",\n"
            << L"  \"initialAlpha\": " << initialAlpha << L",\n"
            << L"  \"displayTimeMs\": " << displayTimeMs << L",\n"
            << L"  \"fadeIntervalMs\": " << fadeIntervalMs << L",\n"
            << L"  \"alphaStep\": " << alphaStep << L",\n"
            << L"  \"bgColor\": \"" << bgColor << L"\",\n"
            << L"  \"textColor\": \"" << textColor << L"\"\n"
            << L"}\n";
        return;
    }
    // простой парсинг
    std::wifstream f(cfgPath);
    std::wstringstream ss; ss << f.rdbuf(); std::wstring s = ss.str();
    auto getInt = [&](std::wstring key) { size_t p = s.find(key); if (p == std::wstring::npos) return 0; return _wtoi(s.c_str() + s.find(L":", p) + 1); };
    auto getByte = [&](std::wstring key) { return static_cast<BYTE>(getInt(key)); };
    auto getStr = [&](std::wstring key) { size_t p = s.find(key); if (p == std::wstring::npos) return std::wstring(); p = s.find(L'"', p + key.size()) + 1; size_t e = s.find(L'"', p); return s.substr(p, e - p); };
    width = getInt(L"\"width\"");
    height = getInt(L"\"height\"");
    fontSize = getInt(L"\"fontSize\"");
    initialAlpha = getByte(L"\"initialAlpha\"");
    displayTimeMs = getInt(L"\"displayTimeMs\"");
    fadeIntervalMs = getInt(L"\"fadeIntervalMs\"");
    alphaStep = getByte(L"\"alphaStep\"");
    bgColor = getStr(L"\"bgColor\"");
    textColor = getStr(L"\"textColor\"");
}
