#pragma once
#include <string>
#include <vector>
#include <optional>
#include "../core/BinaryIO.h"
#include "../domain/Models.h"

namespace ps
{
    class ProductFile final
    {
    public:
        void Create(const std::string& prdPath, std::uint16_t maxNameLen, const std::string& prsPath);
        void Open(const std::string& prdPath);
        void Close();
        bool IsOpen() const;

        const ProductFileHeader& Header() const;
        const std::string& PrdPath() const;
        const std::string& PrsPath() const;

        std::uint16_t MaxNameLen() const;

        std::vector<ComponentRecord> ReadAllRecords();
        ComponentRecord ReadRecordAt(std::uint32_t offset);
        std::optional<ComponentRecord> FindActiveByName(const std::string& name);

        ComponentRecord AddComponent(const std::string& name, ComponentType type);

        void MarkDeleted(std::uint32_t offset, bool deleted);
        void UpdatePointers(std::uint32_t offset, std::uint32_t firstSpecPtr, std::uint32_t nextPtr);

        // изменить имя/тип компонента, не трогая ссылки и указатели
        void UpdateComponent(std::uint32_t offset, const std::string& newName, ComponentType newType);

        void RebuildAlphabeticalLinks();

    private:
        static constexpr std::uint32_t NullPtr = 1;

        ProductFileHeader m_header{};
        std::string m_prdPath;
        std::string m_prsPath;
        BinaryFile m_file;

        std::uint64_t HeaderSize() const;

        void WriteHeader();
        void ReadHeaderAndValidate();

        void WriteRecordAt(std::uint32_t offset, const ComponentRecord& rec);
        std::uint32_t AppendRecord(const ComponentRecord& rec);
    };
}
