#include "SpecFile.h"

namespace ps
{
    void SpecFile::Create(const std::string& prsPath)
    {
        m_prsPath = prsPath;
        m_file.CreateRWTruncate(m_prsPath);
        WriteHeader(NullPtr, static_cast<std::uint32_t>(HeaderSize()));
        m_file.Flush();
    }

    void SpecFile::Open(const std::string& prsPath)
    {
        m_prsPath = prsPath;
        m_file.OpenRW(m_prsPath);
        std::uint32_t h=0,f=0;
        ReadHeader(h,f);
    }

    void SpecFile::Close() { m_file.Close(); }
    bool SpecFile::IsOpen() const { return m_file.IsOpen(); }

    std::uint64_t SpecFile::HeaderSize() const { return 8ull; }

    void SpecFile::WriteHeader(std::uint32_t headPtr, std::uint32_t freePtr)
    {
        m_file.Seek(0);
        m_file.WriteLE<std::uint32_t>(headPtr);
        m_file.WriteLE<std::uint32_t>(freePtr);
    }

    void SpecFile::ReadHeader(std::uint32_t& headPtr, std::uint32_t& freePtr)
    {
        m_file.Seek(0);
        m_file.ReadLE<std::uint32_t>(headPtr);
        m_file.ReadLE<std::uint32_t>(freePtr);
    }

    void SpecFile::WriteRecordAt(std::uint32_t offset, const SpecRecord& rec)
    {
        m_file.Seek(offset);
        std::uint8_t del = rec.deleted ? 1 : 0;
        m_file.WriteLE<std::uint8_t>(del);
        m_file.WriteLE<std::uint32_t>(rec.componentPtr);
        m_file.WriteLE<std::uint16_t>(rec.qty);
        m_file.WriteLE<std::uint32_t>(rec.nextPtr);
    }

    std::uint32_t SpecFile::AppendRecord(const SpecRecord& rec)
    {
        auto offset = static_cast<std::uint32_t>(m_file.Size());
        WriteRecordAt(offset, rec);

        std::uint32_t head=0, free=0;
        ReadHeader(head, free);
        free = static_cast<std::uint32_t>(m_file.Size());
        WriteHeader(head, free);

        m_file.Flush();
        return offset;
    }

    SpecRecord SpecFile::ReadRecordAt(std::uint32_t offset)
    {
        SpecRecord rec;
        rec.fileOffset = offset;

        m_file.Seek(offset);
        std::uint8_t del = 0;
        m_file.ReadLE<std::uint8_t>(del);
        rec.deleted = (del != 0);

        m_file.ReadLE<std::uint32_t>(rec.componentPtr);
        m_file.ReadLE<std::uint16_t>(rec.qty);
        m_file.ReadLE<std::uint32_t>(rec.nextPtr);
        return rec;
    }

    std::vector<SpecRecord> SpecFile::ReadAllRecords()
    {
        std::vector<SpecRecord> out;
        auto sz = m_file.Size();
        auto pos = HeaderSize();
        const std::uint64_t recSize = 1ull + 4ull + 2ull + 4ull;

        while (pos + recSize <= sz)
        {
            out.push_back(ReadRecordAt(static_cast<std::uint32_t>(pos)));
            pos += recSize;
        }
        return out;
    }

    std::uint32_t SpecFile::AddSpecItem(std::uint32_t componentPtr, std::uint16_t qty)
    {
        SpecRecord r;
        r.deleted = false;
        r.componentPtr = componentPtr;
        r.qty = qty;
        r.nextPtr = NullPtr;
        return AppendRecord(r);
    }

    void SpecFile::MarkDeleted(std::uint32_t offset, bool deleted)
    {
        auto r = ReadRecordAt(offset);
        r.deleted = deleted;
        WriteRecordAt(offset, r);
        m_file.Flush();
    }

    void SpecFile::UpdateNext(std::uint32_t offset, std::uint32_t nextPtr)
    {
        auto r = ReadRecordAt(offset);
        r.nextPtr = nextPtr;
        WriteRecordAt(offset, r);
        m_file.Flush();
    }

    std::uint32_t SpecFile::RebuildSpecLinks(std::uint32_t firstSpecPtr)
    {
        if (firstSpecPtr == NullPtr) return NullPtr;

        std::vector<SpecRecord> chain;
        std::uint32_t cur = firstSpecPtr;
        while (cur != NullPtr)
        {
            auto rec = ReadRecordAt(cur);
            if (!rec.deleted) chain.push_back(rec);
            cur = rec.nextPtr;
        }

        for (std::size_t i = 0; i < chain.size(); i++)
        {
            auto next = (i + 1 < chain.size()) ? chain[i + 1].fileOffset : NullPtr;
            UpdateNext(chain[i].fileOffset, static_cast<std::uint32_t>(next));
        }

        return chain.empty() ? NullPtr : chain.front().fileOffset;
    }

    bool SpecFile::HasActiveReferenceToComponent(std::uint32_t componentPtr)
    {
        for (const auto& r : ReadAllRecords())
            if (!r.deleted && r.componentPtr == componentPtr) return true;
        return false;
    }
}
