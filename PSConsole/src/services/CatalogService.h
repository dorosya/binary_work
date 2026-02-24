#pragma once
#include <string>
#include <vector>
#include <optional>
#include "../domain/Models.h"
#include "../infra/ProductFile.h"
#include "../infra/SpecFile.h"

namespace ps
{
    struct SpecItemView
    {
        std::string partName;
        std::uint16_t qty = 1;
        ComponentType type = ComponentType::Detail;
    };

    class CatalogService final
    {
    public:
        bool HasOpenFiles() const;

        void Create(const std::string& baseName, std::uint16_t maxNameLen, const std::optional<std::string>& prsNameOpt);
        void Open(const std::string& baseName);
        void Close();

        void InputComponent(const std::string& name, ComponentType type);
        void InputSpecItem(const std::string& ownerName, const std::string& partName, std::uint16_t qty = 1);

        void UpdateComponent(const std::string& oldName, const std::string& newName, ComponentType newType);

        void DeleteComponent(const std::string& name);
        void DeleteSpecItem(const std::string& ownerName, const std::string& partName);

        void RestoreAll();
        void RestoreComponent(const std::string& name);

        void Truncate();

        std::vector<ComponentRecord> ListComponents();
        std::vector<SpecItemView> ListSpecItems(const std::string& ownerName);
        std::string PrintSpecTree(const std::string& name);

        std::string HelpText() const;

    private:
        static constexpr std::uint32_t NullPtr = 1;

        ProductFile m_products;
        SpecFile m_specs;

        static std::string EnsureExt(const std::string& base, const std::string& ext);
        void EnsureOpen() const;

        std::vector<SpecRecord> ReadSpecChain(std::uint32_t firstSpecPtr);
        void PrintTreeRec(std::string& out, const ComponentRecord& node, const std::string& prefix, bool isLast, int depth);

        void TruncateRebuildFiles();
    };
}
