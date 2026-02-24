#pragma once
#include <string>
#include <vector>
#include "../core/BinaryIO.h"
#include "../domain/Models.h"

namespace ps
{
    class SpecFile final
    {
    public:
        void Create(const std::string& prsPath);
        void Open(const std::string& prsPath);
        void Close();
        bool IsOpen() const;

        std::vector<SpecRecord> ReadAllRecords();
        SpecRecord ReadRecordAt(std::uint32_t offset);

        std::uint32_t AddSpecItem(std::uint32_t componentPtr, std::uint16_t qty);

        void MarkDeleted(std::uint32_t offset, bool deleted);
        void UpdateNext(std::uint32_t offset, std::uint32_t nextPtr);

        std::uint32_t RebuildSpecLinks(std::uint32_t firstSpecPtr);

        bool HasActiveReferenceToComponent(std::uint32_t componentPtr);

    private:
        static constexpr std::uint32_t NullPtr = 1;

        std::string m_prsPath;
        BinaryFile m_file;

        std::uint64_t HeaderSize() const;

        void WriteHeader(std::uint32_t headPtr, std::uint32_t freePtr);
        void ReadHeader(std::uint32_t& headPtr, std::uint32_t& freePtr);

        void WriteRecordAt(std::uint32_t offset, const SpecRecord& rec);
        std::uint32_t AppendRecord(const SpecRecord& rec);
    };
}
