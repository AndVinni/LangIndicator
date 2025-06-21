// config.cpp — implementation of Config::LoadOrCreate and RegisterAutoRun
#include "config.h"
#include <Windows.h>
#include <shlwapi.h>
#include <fstream>
#include <sstream>
#pragma comment(lib, "shlwapi.lib")

static const wchar_t* CONFIG_NAME = L"config.json";
static const wchar_t* keyReg = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* subKey = L"LangIndicator";

void RegisterAutoRun()
{
    HKEY hKey;
    // 1) Open the autorun branch for recording
    LSTATUS status = RegOpenKeyExW( HKEY_CURRENT_USER, keyReg, 0, KEY_WRITE, &hKey );
    if (status != ERROR_SUCCESS)
    {
        // If we couldn't open it, we're leaving.
        return;
    }

    // 2) Check if there is already a value named subKey
    DWORD type = 0;
    DWORD dataSize = 0;
    status = RegQueryValueExW( hKey, subKey, nullptr, &type, nullptr, &dataSize );
    if (status == ERROR_SUCCESS && type == REG_SZ)
    {
        // Значение уже есть – ничего не делаем
        RegCloseKey(hKey);
        return;
    }

    // 3) There is no value - we create it
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    RegSetValueExW( hKey, subKey, 0, REG_SZ, reinterpret_cast<const BYTE*>(path), static_cast<DWORD>((wcslen(path) + 1) * sizeof(wchar_t)));
    // 4) We close the key
    RegCloseKey(hKey);
}

void Config::LoadOrCreate()
{
    wchar_t modPath[MAX_PATH];
    GetModuleFileNameW(nullptr, modPath, MAX_PATH);
    PathRemoveFileSpecW(modPath);
    std::wstring cfgPath = std::wstring(modPath) + L"\\" + CONFIG_NAME;

    if (!PathFileExistsW(cfgPath.c_str()))
    {
        width = 64; height = 32; fontSize = 24;
        initialAlpha = 128; /*displayTimeMs = 100;*/
        fadeIntervalMs = 2; alphaStep = 5;
        bgColor = L"#000000"; textColor = L"#FFFFFF";
        std::wofstream f(cfgPath, std::ios::binary);
        // To remove startup: langindicator.exe -u
        f << L"//To delete auto-upload: langindicator.exe -u\n";

        f << L"{\n"
            << L"  \"width\": " << width << L",\n"
            << L"  \"height\": " << height << L",\n"
            << L"  \"fontSize\": " << fontSize << L",\n"
            << L"  \"initialAlpha\": " << initialAlpha << L",\n"
            << L"  \"fadeIntervalMs\": " << fadeIntervalMs << L",\n"
            << L"  \"alphaStep\": " << alphaStep << L",\n"
            << L"  \"bgColor\": \"" << bgColor << L"\",\n"
            << L"  \"textColor\": \"" << textColor << L"\"\n"
            << L"}\n";
        return;
    }
    // simple parsing
    std::wifstream f(cfgPath);
    std::wstringstream ss; ss << f.rdbuf(); std::wstring s = ss.str();
    auto getInt = [&](std::wstring key) { size_t p = s.find(key); if (p == std::wstring::npos) return 0; return _wtoi(s.c_str() + s.find(L":", p) + 1); };
    auto getByte = [&](std::wstring key) { return static_cast<BYTE>(getInt(key)); };
    auto getStr = [&](std::wstring key) { size_t p = s.find(key); if (p == std::wstring::npos) return std::wstring(); p = s.find(L'"', p + key.size()) + 1; size_t e = s.find(L'"', p); return s.substr(p, e - p); };
    width = getInt(L"\"width\"");
    height = getInt(L"\"height\"");
    fontSize = getInt(L"\"fontSize\"");
    initialAlpha = getByte(L"\"initialAlpha\"");
    fadeIntervalMs = getInt(L"\"fadeIntervalMs\"");
    alphaStep = getByte(L"\"alphaStep\"");
    bgColor = getStr(L"\"bgColor\"");
    textColor = getStr(L"\"textColor\"");
}

// Removes an entry from startup
void UnregisterAutoRun()
{
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, keyReg, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
    {
        RegDeleteValueW(hKey, L"LangIndicator");
        RegCloseKey(hKey);
    }
}

