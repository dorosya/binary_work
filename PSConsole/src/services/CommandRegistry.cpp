#include "CommandRegistry.h"
#include "Commands.h"

#include <cctype>

namespace ps
{
    static std::string NormalizeCommandName(const std::string& name)
    {
        std::string normalized = name;
        for (auto& ch : normalized)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return normalized;
    }

    void CommandRegistry::Register(std::unique_ptr<ICommand> cmd)
    {
        m_cmds.emplace(NormalizeCommandName(cmd->Name()), std::move(cmd));
    }

    ICommand* CommandRegistry::Find(const std::string& name) const
    {
        auto it = m_cmds.find(NormalizeCommandName(name));
        if (it == m_cmds.end()) return nullptr;
        return it->second.get();
    }
}
