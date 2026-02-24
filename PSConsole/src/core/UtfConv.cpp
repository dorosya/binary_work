#include "UtfConv.h"

#if defined(_WIN32)
  #include <windows.h>
#else
  #include <codecvt>
  #include <locale>
#endif

namespace ps
{
    std::wstring Utf8ToWide(const std::string& utf8)
    {
        if (utf8.empty()) return std::wstring();

#if defined(_WIN32)
        const int needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), nullptr, 0);
        if (needed <= 0) return std::wstring();
        std::wstring wide(needed, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), wide.data(), needed);
        return wide;
#else
        // Proper UTF-8 conversion for Linux/macOS.
        // Note: std::wstring_convert is deprecated in C++17 but still widely available in libstdc++/libc++.
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        return conv.from_bytes(utf8);
#endif
    }

    std::string WideToUtf8(const std::wstring& wide)
    {
        if (wide.empty()) return std::string();

#if defined(_WIN32)
        const int needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), nullptr, 0, nullptr, nullptr);
        if (needed <= 0) return std::string();
        std::string utf8(needed, '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), utf8.data(), needed, nullptr, nullptr);
        return utf8;
#else
        std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
        return conv.to_bytes(wide);
#endif
    }
}
