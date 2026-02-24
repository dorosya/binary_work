#pragma once
#include <memory>
#include <string>
#include <unordered_map>

namespace ps
{
    class ICommand;

    class CommandRegistry final
    {
    public:
        void Register(std::unique_ptr<ICommand> cmd);
        ICommand* Find(const std::string& name) const;

    private:
        std::unordered_map<std::string, std::unique_ptr<ICommand>> m_cmds;
    };
}
