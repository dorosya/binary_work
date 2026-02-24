#pragma once

#include <string>

namespace ps
{
    // Converts UTF-8 string to UTF-16 (std::wstring on Windows).
    // On non-Windows platforms this still returns a wstring (best-effort).
    std::wstring Utf8ToWide(const std::string& utf8);

    // Converts UTF-16/UTF-32 wide string to UTF-8.
    std::string WideToUtf8(const std::wstring& wide);
}
