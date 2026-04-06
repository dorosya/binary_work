#include <iostream>
#include <string>

#include "core/ConsoleUtf8.h"
#include "core/Errors.h"
#include "core/UtfConv.h"
#include "domain/Parsing.h"
#include "services/CatalogService.h"
#include "services/CommandRegistry.h"
#include "services/Commands.h"

using namespace ps;

int main()
{
    try
    {
        ps::ConfigureConsoleForCyrillic();

        CatalogService service;
        CommandRegistry registry;
        for (auto& cmd : CreateDefaultCommands())
            registry.Register(std::move(cmd));

        ps::ConsoleWriteW(L"PS> ");

#if defined(_WIN32)
        std::wstring wideLine;
        while (std::getline(std::wcin, wideLine))
        {
            auto parsed = ParseCommandLine(WideToUtf8(wideLine));
#else
        std::string line;
        while (std::getline(std::cin, line))
        {
            auto parsed = ParseCommandLine(line);
#endif
            if (parsed.name.empty())
            {
                ps::ConsoleWriteW(L"PS> ");
                continue;
            }

            auto* handler = registry.Find(parsed.name);
            if (!handler)
            {
                ps::ConsoleWriteW(L"Неизвестная команда. Введите Help.\nPS> ");
                continue;
            }

            auto res = handler->Execute(parsed, service);

            if (!res.error.empty())
            {
                ps::ConsoleWriteW(L"Ошибка: ");
                ps::ConsoleWriteW(Utf8ToWide(res.error));
                ps::ConsoleWriteW(L"\n");
            }

            if (!res.output.empty())
                ps::ConsoleWriteW(Utf8ToWide(res.output));

            if (res.shouldExit) break;

            ps::ConsoleWriteW(L"PS> ");
        }
    }
    catch (const PsException& ex)
    {
        ps::ConsoleWriteErrW(L"Критическая ошибка: ");
        ps::ConsoleWriteErrW(Utf8ToWide(ex.what()));
        ps::ConsoleWriteErrW(L"\n");
        return 1;
    }
    catch (const std::exception& ex)
    {
        ps::ConsoleWriteErrW(L"Критическая ошибка: ");
        ps::ConsoleWriteErrW(Utf8ToWide(ex.what()));
        ps::ConsoleWriteErrW(L"\n");
        return 1;
    }

    return 0;
}
