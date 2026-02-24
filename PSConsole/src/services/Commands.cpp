#include "Commands.h"
#include "../core/Errors.h"
#include "../domain/Models.h"
#include <sstream>
#include <fstream>

namespace ps
{
    class CreateCommand final : public ICommand
    {
    public:
        std::string Name() const override { return "Create"; }
        CommandResult Execute(const ParsedCommand& cmd, CatalogService& svc) override
        {
            CommandResult r;
            try
            {
                if (cmd.args.empty())
                {
                    r.error = "Create: ожидается имяФайла(максДлина[,prs]) или имяФайла максДлина [prs].";
                    return r;
                }

                auto first = cmd.args[0];
                auto pos = first.find('(');
                if (pos != std::string::npos)
                {
                    auto base = first.substr(0, pos);
                    auto inner = StripOuterParens(first.substr(pos));
                    auto innerArgs = SplitCsvArgs(inner);
                    if (innerArgs.empty()) { r.error = "Create: не указан максДлина."; return r; }

                    std::uint16_t maxLen = static_cast<std::uint16_t>(std::stoi(innerArgs[0]));
                    std::optional<std::string> prs;
                    if (innerArgs.size() > 1) prs = innerArgs[1];
                    svc.Create(base, maxLen, prs);
                    r.output = "OK\n";
                    return r;
                }

                if (cmd.args.size() < 2) { r.error = "Create: не указан максДлина."; return r; }

                std::uint16_t maxLen = static_cast<std::uint16_t>(std::stoi(cmd.args[1]));
                std::optional<std::string> prs;
                if (cmd.args.size() >= 3) prs = cmd.args[2];

                svc.Create(first, maxLen, prs);
                r.output = "OK\n";
            }
            catch (const PsException& ex) { r.error = ex.what(); }
            return r;
        }
    };

    class OpenCommand final : public ICommand
    {
    public:
        std::string Name() const override { return "Open"; }
        CommandResult Execute(const ParsedCommand& cmd, CatalogService& svc) override
        {
            CommandResult r;
            try
            {
                if (cmd.args.size() < 1) { r.error = "Open: ожидается имя файла."; return r; }
                svc.Open(cmd.args[0]);
                r.output = "OK\n";
            }
            catch (const PsException& ex) { r.error = ex.what(); }
            return r;
        }
    };

    class InputCommand final : public ICommand
    {
    public:
        std::string Name() const override { return "Input"; }
        CommandResult Execute(const ParsedCommand& cmd, CatalogService& svc) override
        {
            CommandResult r;
            try
            {
                if (cmd.args.size() < 1) { r.error = "Input: ожидаются аргументы."; return r; }

                if (cmd.args.size() == 2 && cmd.args[0].find('/') == std::string::npos)
                {
                    auto typeOpt = ParseComponentType(cmd.args[1]);
                    if (!typeOpt.has_value()) { r.error = "Input: тип должен быть Изделие/Узел/Деталь."; return r; }
                    svc.InputComponent(cmd.args[0], *typeOpt);
                    r.output = "OK\n";
                    return r;
                }

                auto s = cmd.args[0];
                auto slash = s.find('/');
                if (slash == std::string::npos) { r.error = "Input: формат владелец/комплектующее."; return r; }

                std::uint16_t qty = 1;
                if (cmd.args.size() >= 2) qty = static_cast<std::uint16_t>(std::stoi(cmd.args[1]));

                svc.InputSpecItem(s.substr(0, slash), s.substr(slash + 1), qty);
                r.output = "OK\n";
            }
            catch (const PsException& ex) { r.error = ex.what(); }
            return r;
        }
    };

    class DeleteCommand final : public ICommand
    {
    public:
        std::string Name() const override { return "Delete"; }
        CommandResult Execute(const ParsedCommand& cmd, CatalogService& svc) override
        {
            CommandResult r;
            try
            {
                if (cmd.args.size() < 1) { r.error = "Delete: ожидается аргумент."; return r; }

                auto s = cmd.args[0];
                auto slash = s.find('/');
                if (slash == std::string::npos) svc.DeleteComponent(s);
                else svc.DeleteSpecItem(s.substr(0, slash), s.substr(slash + 1));

                r.output = "OK\n";
            }
            catch (const PsException& ex) { r.error = ex.what(); }
            return r;
        }
    };

    class RestoreCommand final : public ICommand
    {
    public:
        std::string Name() const override { return "Restore"; }
        CommandResult Execute(const ParsedCommand& cmd, CatalogService& svc) override
        {
            CommandResult r;
            try
            {
                if (cmd.args.size() < 1) { r.error = "Restore: ожидается имя компонента или *."; return r; }
                if (cmd.args[0] == "*") svc.RestoreAll();
                else svc.RestoreComponent(cmd.args[0]);
                r.output = "OK\n";
            }
            catch (const PsException& ex) { r.error = ex.what(); }
            return r;
        }
    };

    class TruncateCommand final : public ICommand
    {
    public:
        std::string Name() const override { return "Truncate"; }
        CommandResult Execute(const ParsedCommand&, CatalogService& svc) override
        {
            CommandResult r;
            try { svc.Truncate(); r.output = "OK\n"; }
            catch (const PsException& ex) { r.error = ex.what(); }
            return r;
        }
    };

    class PrintCommand final : public ICommand
    {
    public:
        std::string Name() const override { return "Print"; }
        CommandResult Execute(const ParsedCommand& cmd, CatalogService& svc) override
        {
            CommandResult r;
            try
            {
                if (cmd.args.size() < 1) { r.error = "Print: ожидается имя компонента или *."; return r; }

                if (cmd.args[0] == "*")
                {
                    auto list = svc.ListComponents();
                    std::ostringstream oss;
                    oss << "Наименование\tТип\n";
                    for (const auto& c : list) oss << c.name << "\t" << ToString(c.type) << "\n";
                    r.output = oss.str();
                    return r;
                }

                r.output = svc.PrintSpecTree(cmd.args[0]);
            }
            catch (const PsException& ex) { r.error = ex.what(); }
            return r;
        }
    };

    class HelpCommand final : public ICommand
    {
    public:
        std::string Name() const override { return "Help"; }
        CommandResult Execute(const ParsedCommand& cmd, CatalogService& svc) override
        {
            CommandResult r;
            try
            {
                auto text = svc.HelpText();
                if (cmd.args.empty()) { r.output = text; return r; }

                std::ofstream out(cmd.args[0], std::ios::binary);
                out.write(text.data(), static_cast<std::streamsize>(text.size()));
                r.output = "OK\n";
            }
            catch (const std::exception& ex) { r.error = ex.what(); }
            return r;
        }
    };

    class ExitCommand final : public ICommand
    {
    public:
        std::string Name() const override { return "Exit"; }
        CommandResult Execute(const ParsedCommand&, CatalogService& svc) override
        {
            CommandResult r;
            svc.Close();
            r.shouldExit = true;
            return r;
        }
    };

    std::vector<std::unique_ptr<ICommand>> CreateDefaultCommands()
    {
        std::vector<std::unique_ptr<ICommand>> cmds;
        cmds.push_back(std::make_unique<CreateCommand>());
        cmds.push_back(std::make_unique<OpenCommand>());
        cmds.push_back(std::make_unique<InputCommand>());
        cmds.push_back(std::make_unique<DeleteCommand>());
        cmds.push_back(std::make_unique<RestoreCommand>());
        cmds.push_back(std::make_unique<TruncateCommand>());
        cmds.push_back(std::make_unique<PrintCommand>());
        cmds.push_back(std::make_unique<HelpCommand>());
        cmds.push_back(std::make_unique<ExitCommand>());
        return cmds;
    }
}
