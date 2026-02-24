#include "Parsing.h"
#include <cctype>

namespace ps
{
    static std::string Trim(const std::string& s)
    {
        std::size_t b = 0;
        while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) b++;
        std::size_t e = s.size();
        while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) e--;
        return s.substr(b, e - b);
    }

    std::string StripOuterParens(const std::string& s)
    {
        auto t = Trim(s);
        if (t.size() >= 2 && t.front() == '(' && t.back() == ')')
            return Trim(t.substr(1, t.size() - 2));
        return t;
    }

    std::vector<std::string> SplitCsvArgs(const std::string& s)
    {
        std::vector<std::string> out;
        std::string cur;
        bool inQuotes = false;

        for (char ch : s)
        {
            if (ch == '"') { inQuotes = !inQuotes; continue; }
            if (!inQuotes && ch == ',')
            {
                out.push_back(Trim(cur));
                cur.clear();
                continue;
            }
            cur.push_back(ch);
        }
        if (!cur.empty() || !out.empty()) out.push_back(Trim(cur));
        return out;
    }

    ParsedCommand ParseCommandLine(const std::string& line)
    {
        ParsedCommand cmd;
        cmd.raw = line;

        auto t = Trim(line);
        if (t.empty()) return cmd;

        std::size_t i = 0;
        while (i < t.size() && !std::isspace(static_cast<unsigned char>(t[i]))) i++;
        cmd.name = t.substr(0, i);

        auto rest = Trim(t.substr(i));
        if (rest.empty()) return cmd;

        if (rest.front() == '(')
        {
            cmd.args = SplitCsvArgs(StripOuterParens(rest));
            return cmd;
        }

        // пробельные args
        std::vector<std::string> args;
        std::string cur;
        for (char ch : rest)
        {
            if (std::isspace(static_cast<unsigned char>(ch)))
            {
                if (!cur.empty()) { args.push_back(Trim(cur)); cur.clear(); }
                continue;
            }
            cur.push_back(ch);
        }
        if (!cur.empty()) args.push_back(Trim(cur));
        cmd.args = args;
        return cmd;
    }
}
