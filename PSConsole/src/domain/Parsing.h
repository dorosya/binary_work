#pragma once
#include <string>
#include <vector>

namespace ps
{
    struct ParsedCommand
    {
        std::string name;
        std::vector<std::string> args;
        std::string raw;
    };

    ParsedCommand ParseCommandLine(const std::string& line);
    std::string StripOuterParens(const std::string& s);
    std::vector<std::string> SplitCsvArgs(const std::string& s);
}
