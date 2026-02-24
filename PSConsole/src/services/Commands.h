#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../domain/Parsing.h"
#include "CatalogService.h"

namespace ps
{
    struct CommandResult
    {
        bool shouldExit = false;
        std::string output;
        std::string error;
    };

    class ICommand
    {
    public:
        virtual ~ICommand() = default;
        virtual std::string Name() const = 0;
        virtual CommandResult Execute(const ParsedCommand& cmd, CatalogService& svc) = 0;
    };

    std::vector<std::unique_ptr<ICommand>> CreateDefaultCommands();
}
