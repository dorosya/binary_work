#pragma once

// Console I/O helpers for correct Cyrillic/Unicode in cmd.exe and Visual Studio debug console.
// Strategy:
// 1) On Windows, if stdout/stderr are real console handles, write using WriteConsoleW (UTF-16).
// 2) Otherwise write UTF-8 bytes.
// 3) On Linux/macOS, write UTF-8 to standard streams.

#include <cstdio>
#include <iostream>
#include <string>

#include "UtfConv.h"

#if defined(_WIN32)
  #include <windows.h>
  #include <fcntl.h>
  #include <io.h>
#endif

namespace ps
{
#if defined(_WIN32)
    inline bool IsConsoleHandle(HANDLE h)
    {
        if (h == nullptr || h == INVALID_HANDLE_VALUE) return false;
        DWORD mode = 0;
        return GetConsoleMode(h, &mode) != 0;
    }

    inline void ConfigureConsoleForCyrillic()
    {
        _setmode(_fileno(stdin),  _O_U16TEXT);
        _setmode(_fileno(stdout), _O_U16TEXT);
        _setmode(_fileno(stderr), _O_U16TEXT);

        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    }

    inline void ConsoleWriteW(const std::wstring& s)
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (IsConsoleHandle(hOut))
        {
            DWORD written = 0;
            WriteConsoleW(hOut, s.c_str(), static_cast<DWORD>(s.size()), &written, nullptr);
            return;
        }

        int need = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
        std::string utf8(need, '\0');
        WideCharToMultiByte(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), utf8.data(), need, nullptr, nullptr);
        _setmode(_fileno(stdout), _O_BINARY);
        fwrite(utf8.data(), 1, utf8.size(), stdout);
        fflush(stdout);
    }

    inline void ConsoleWriteErrW(const std::wstring& s)
    {
        HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
        if (IsConsoleHandle(hErr))
        {
            DWORD written = 0;
            WriteConsoleW(hErr, s.c_str(), static_cast<DWORD>(s.size()), &written, nullptr);
            return;
        }

        int need = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), nullptr, 0, nullptr, nullptr);
        std::string utf8(need, '\0');
        WideCharToMultiByte(CP_UTF8, 0, s.c_str(), static_cast<int>(s.size()), utf8.data(), need, nullptr, nullptr);
        _setmode(_fileno(stderr), _O_BINARY);
        fwrite(utf8.data(), 1, utf8.size(), stderr);
        fflush(stderr);
    }
#else
    inline void ConfigureConsoleForCyrillic() {}

    inline void ConsoleWriteW(const std::wstring& s)
    {
        std::cout << WideToUtf8(s);
        std::cout.flush();
    }

    inline void ConsoleWriteErrW(const std::wstring& s)
    {
        std::cerr << WideToUtf8(s);
        std::cerr.flush();
    }
#endif
}
