#include "ProductFile.h"
#include <algorithm>

namespace ps
{
    static std::string TrimSpaces(const std::string& s)
    {
        auto b = s.find_first_not_of(' ');
        if (b == std::string::npos) return "";
        auto e = s.find_last_not_of(' ');
        return s.substr(b, e - b + 1);
    }

    void ProductFile::Create(const std::string& prdPath, std::uint16_t maxNameLen, const std::string& prsPath)
    {
        if (maxNameLen == 0 || maxNameLen > 5000)
            throw ValidationException("Некорректная максимальная длина имени компонента.");

        m_prdPath = prdPath;
        m_prsPath = prsPath;

        m_header.dataLen = static_cast<std::uint16_t>(1 + maxNameLen);
        m_header.headPtr = NullPtr;
        m_header.specFileName = prsPath;

        m_file.CreateRWTruncate(m_prdPath);

        const char sig[2] = { 'P','S' };
        m_file.WriteBytes(sig, 2);
        m_file.WriteLE<std::uint16_t>(m_header.dataLen);
        m_file.WriteLE<std::uint32_t>(m_header.headPtr);
        m_file.WriteLE<std::uint32_t>(0);
        m_file.WriteFixedString(m_prsPath, 16, ' ');

        m_header.freePtr = static_cast<std::uint32_t>(m_file.Tell());
        m_file.Seek(2 + 2 + 4);
        m_file.WriteLE<std::uint32_t>(m_header.freePtr);
        m_file.Flush();
    }

    void ProductFile::Open(const std::string& prdPath)
    {
        m_prdPath = prdPath;
        m_file.OpenRW(m_prdPath);
        ReadHeaderAndValidate();
        m_prsPath = TrimSpaces(m_header.specFileName);
    }

    void ProductFile::Close() { m_file.Close(); }
    bool ProductFile::IsOpen() const { return m_file.IsOpen(); }

    const ProductFileHeader& ProductFile::Header() const { return m_header; }
    const std::string& ProductFile::PrdPath() const { return m_prdPath; }
    const std::string& ProductFile::PrsPath() const { return m_prsPath; }

    std::uint16_t ProductFile::MaxNameLen() const { return static_cast<std::uint16_t>(m_header.dataLen - 1); }
    std::uint64_t ProductFile::HeaderSize() const { return 2ull + 2ull + 4ull + 4ull + 16ull; }

    void ProductFile::WriteHeader()
    {
        m_file.Seek(0);
        const char sig[2] = { 'P','S' };
        m_file.WriteBytes(sig, 2);
        m_file.WriteLE<std::uint16_t>(m_header.dataLen);
        m_file.WriteLE<std::uint32_t>(m_header.headPtr);
        m_file.WriteLE<std::uint32_t>(m_header.freePtr);
        m_file.WriteFixedString(m_header.specFileName, 16, ' ');
    }

    void ProductFile::ReadHeaderAndValidate()
    {
        m_file.Seek(0);
        char sig[2]{};
        m_file.ReadBytes(sig, 2);
        if (!(sig[0] == 'P' && sig[1] == 'S'))
            throw FileException("Сигнатура файла отсутствует или неверна (ожидалось 'PS').");

        m_file.ReadLE<std::uint16_t>(m_header.dataLen);
        m_file.ReadLE<std::uint32_t>(m_header.headPtr);
        m_file.ReadLE<std::uint32_t>(m_header.freePtr);
        m_header.specFileName = m_file.ReadFixedString(16);

        if (m_header.dataLen < 2)
            throw FileException("Некорректная длина области данных (dataLen) в заголовке.");
    }

    void ProductFile::WriteRecordAt(std::uint32_t offset, const ComponentRecord& rec)
    {
        m_file.Seek(offset);
        std::uint8_t del = rec.deleted ? 1 : 0;
        m_file.WriteLE<std::uint8_t>(del);
        m_file.WriteLE<std::uint32_t>(rec.firstSpecPtr);
        m_file.WriteLE<std::uint32_t>(rec.nextPtr);
        m_file.WriteLE<std::uint8_t>(static_cast<std::uint8_t>(rec.type));
        m_file.WriteFixedString(rec.name, MaxNameLen(), ' ');
    }

    std::uint32_t ProductFile::AppendRecord(const ComponentRecord& rec)
    {
        auto offset = static_cast<std::uint32_t>(m_file.Size());
        WriteRecordAt(offset, rec);
        m_header.freePtr = static_cast<std::uint32_t>(m_file.Size());
        WriteHeader();
        m_file.Flush();
        return offset;
    }

    ComponentRecord ProductFile::ReadRecordAt(std::uint32_t offset)
    {
        ComponentRecord rec;
        rec.fileOffset = offset;

        m_file.Seek(offset);
        std::uint8_t del = 0;
        m_file.ReadLE<std::uint8_t>(del);
        rec.deleted = (del != 0);

        m_file.ReadLE<std::uint32_t>(rec.firstSpecPtr);
        m_file.ReadLE<std::uint32_t>(rec.nextPtr);

        std::uint8_t t = 0;
        m_file.ReadLE<std::uint8_t>(t);
        rec.type = static_cast<ComponentType>(t);

        rec.name = TrimSpaces(m_file.ReadFixedString(MaxNameLen()));
        return rec;
    }

    std::vector<ComponentRecord> ProductFile::ReadAllRecords()
    {
        std::vector<ComponentRecord> out;
        auto sz = m_file.Size();
        auto pos = HeaderSize();
        const std::uint64_t recSize = 1ull + 4ull + 4ull + static_cast<std::uint64_t>(m_header.dataLen);

        while (pos + recSize <= sz)
        {
            out.push_back(ReadRecordAt(static_cast<std::uint32_t>(pos)));
            pos += recSize;
        }
        return out;
    }

    std::optional<ComponentRecord> ProductFile::FindActiveByName(const std::string& name)
    {
        auto target = TrimSpaces(name);
        for (const auto& r : ReadAllRecords())
            if (!r.deleted && r.name == target) return r;
        return std::nullopt;
    }

    ComponentRecord ProductFile::AddComponent(const std::string& name, ComponentType type)
    {
        auto nm = TrimSpaces(name);
        if (nm.empty()) throw ValidationException("Пустое имя компонента.");
        if (nm.size() > MaxNameLen()) throw ValidationException("Имя компонента длиннее maxNameLen (Create).");
        if (FindActiveByName(nm).has_value()) throw ValidationException("Дублирование имен компонентов.");

        ComponentRecord newRec;
        newRec.deleted = false;
        newRec.firstSpecPtr = NullPtr;
        newRec.nextPtr = NullPtr;
        newRec.type = type;
        newRec.name = nm;

        auto newOffset = AppendRecord(newRec);
        newRec.fileOffset = newOffset;

        // вставка в алфавитный список
        if (m_header.headPtr == NullPtr)
        {
            m_header.headPtr = newOffset;
            WriteHeader();
            m_file.Flush();
            return newRec;
        }

        std::uint32_t prev = NullPtr;
        std::uint32_t cur = m_header.headPtr;

        while (cur != NullPtr)
        {
            auto curRec = ReadRecordAt(cur);
            if (!curRec.deleted && curRec.name > nm) break;
            prev = cur;
            cur = curRec.nextPtr;
        }

        if (prev == NullPtr)
        {
            newRec.nextPtr = m_header.headPtr;
            WriteRecordAt(newOffset, newRec);
            m_header.headPtr = newOffset;
            WriteHeader();
            m_file.Flush();
            return newRec;
        }

        auto prevRec = ReadRecordAt(prev);
        newRec.nextPtr = cur;
        WriteRecordAt(newOffset, newRec);
        prevRec.nextPtr = newOffset;
        WriteRecordAt(prev, prevRec);
        m_file.Flush();
        return newRec;
    }

    void ProductFile::MarkDeleted(std::uint32_t offset, bool deleted)
    {
        auto r = ReadRecordAt(offset);
        r.deleted = deleted;
        WriteRecordAt(offset, r);
        m_file.Flush();
    }

    void ProductFile::UpdatePointers(std::uint32_t offset, std::uint32_t firstSpecPtr, std::uint32_t nextPtr)
    {
        auto r = ReadRecordAt(offset);
        r.firstSpecPtr = firstSpecPtr;
        r.nextPtr = nextPtr;
        WriteRecordAt(offset, r);
        m_file.Flush();
    }

    void ProductFile::UpdateComponent(std::uint32_t offset, const std::string& newName, ComponentType newType)
    {
        auto r = ReadRecordAt(offset);
        r.name = TrimSpaces(newName);
        r.type = newType;
        WriteRecordAt(offset, r);
        m_file.Flush();
    }


    void ProductFile::RebuildAlphabeticalLinks()
    {
        auto all = ReadAllRecords();
        std::vector<ComponentRecord> active;
        for (auto& r : all) if (!r.deleted) active.push_back(r);

        std::sort(active.begin(), active.end(), [](const auto& a, const auto& b) { return a.name < b.name; });

        for (std::size_t i = 0; i < active.size(); i++)
        {
            auto next = (i + 1 < active.size()) ? active[i + 1].fileOffset : NullPtr;
            UpdatePointers(active[i].fileOffset, active[i].firstSpecPtr, static_cast<std::uint32_t>(next));
        }

        m_header.headPtr = active.empty() ? NullPtr : active.front().fileOffset;
        m_header.freePtr = static_cast<std::uint32_t>(m_file.Size());
        WriteHeader();
        m_file.Flush();
    }
}
