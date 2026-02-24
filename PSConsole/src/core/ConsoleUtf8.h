#pragma once

// Console I/O helpers for correct Cyrillic/Unicode in cmd.exe and Visual Studio debug console.
// Strategy:
// 1) If stdout/stderr are real Windows console handles -> write using WriteConsoleW (UTF-16).
// 2) Otherwise (pipe/redirection/VS output capture) -> fall back to UTF-8 bytes.

#include <string>

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
        // Try to switch CRT wide streams to UTF-16 mode (works for real console windows).
        // If VS/debugger captures output via pipes, this may be ignored — that's OK because
        // we'll use WriteConsoleW when we can, and UTF-8 otherwise.
        _setmode(_fileno(stdin),  _O_U16TEXT);
        _setmode(_fileno(stdout), _O_U16TEXT);
        _setmode(_fileno(stderr), _O_U16TEXT);

        // Keep console CPs consistent for any narrow output that might still happen.
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

        // Fallback: write UTF-8 bytes to stdout
        int need = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0, nullptr, nullptr);
        std::string utf8(need, '\0');
        WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), utf8.data(), need, nullptr, nullptr);
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

        int need = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0, nullptr, nullptr);
        std::string utf8(need, '\0');
        WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), utf8.data(), need, nullptr, nullptr);
        _setmode(_fileno(stderr), _O_BINARY);
        fwrite(utf8.data(), 1, utf8.size(), stderr);
        fflush(stderr);
    }
#else
    inline void ConfigureConsoleForCyrillic() {}
    inline void ConsoleWriteW(const std::wstring& s) { (void)s; }
    inline void ConsoleWriteErrW(const std::wstring& s) { (void)s; }
#endif
}
