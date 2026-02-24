#include "BinaryIO.h"
#include <fstream>

namespace ps
{
    void BinaryFile::OpenRW(const std::string& path)
    {
        Close();
        m_stream.open(path, std::ios::binary | std::ios::in | std::ios::out);
        if (!m_stream) throw FileException("Не удалось открыть файл: " + path);
    }

    void BinaryFile::CreateRWTruncate(const std::string& path)
    {
        Close();
        m_stream.open(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!m_stream)
        {
            std::ofstream tmp(path, std::ios::binary | std::ios::trunc);
            if (!tmp) throw FileException("Не удалось создать файл: " + path);
            tmp.close();

            m_stream.open(path, std::ios::binary | std::ios::in | std::ios::out);
            if (!m_stream) throw FileException("Не удалось открыть созданный файл: " + path);
        }
    }

    void BinaryFile::Close()
    {
        if (m_stream.is_open())
        {
            m_stream.flush();
            m_stream.close();
        }
    }

    bool BinaryFile::IsOpen() const { return m_stream.is_open(); }

    std::uint64_t BinaryFile::Size()
    {
        auto cur = m_stream.tellg();
        m_stream.seekg(0, std::ios::end);
        auto end = m_stream.tellg();
        m_stream.seekg(cur);
        m_stream.seekp(cur);
        return static_cast<std::uint64_t>(end);
    }

    void BinaryFile::Seek(std::uint64_t pos)
    {
        m_stream.seekg(static_cast<std::streamoff>(pos), std::ios::beg);
        m_stream.seekp(static_cast<std::streamoff>(pos), std::ios::beg);
        if (!m_stream) throw FileException("Ошибка позиционирования в файле.");
    }

    std::uint64_t BinaryFile::Tell()
    {
        auto p = m_stream.tellp();
        if (p < 0) throw FileException("Ошибка получения позиции в файле.");
        return static_cast<std::uint64_t>(p);
    }

    void BinaryFile::Flush()
    {
        m_stream.flush();
        if (!m_stream) throw FileException("Ошибка flush().");
    }

    void BinaryFile::WriteBytes(const void* data, std::size_t size)
    {
        m_stream.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
        if (!m_stream) throw FileException("Ошибка записи байтов в файл.");
    }

    void BinaryFile::ReadBytes(void* data, std::size_t size)
    {
        m_stream.read(reinterpret_cast<char*>(data), static_cast<std::streamsize>(size));
        if (!m_stream) throw FileException("Ошибка чтения байтов из файла.");
    }

    void BinaryFile::WriteFixedString(const std::string& value, std::size_t fixedLen, char pad)
    {
        std::string s = value;
        if (s.size() > fixedLen) s.resize(fixedLen);
        if (s.size() < fixedLen) s.append(fixedLen - s.size(), pad);
        WriteBytes(s.data(), fixedLen);
    }

    std::string BinaryFile::ReadFixedString(std::size_t fixedLen)
    {
        std::string s(fixedLen, '\0');
        ReadBytes(s.data(), fixedLen);
        return s;
    }
}
