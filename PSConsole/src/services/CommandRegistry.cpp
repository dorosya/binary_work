#include "CommandRegistry.h"
#include "Commands.h"

namespace ps
{
    void CommandRegistry::Register(std::unique_ptr<ICommand> cmd)
    {
        m_cmds.emplace(cmd->Name(), std::move(cmd));
    }

    ICommand* CommandRegistry::Find(const std::string& name) const
    {
        auto it = m_cmds.find(name);
        if (it == m_cmds.end()) return nullptr;
        return it->second.get();
    }
}
