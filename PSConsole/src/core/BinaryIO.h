#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <type_traits>
#include "Errors.h"

namespace ps
{
    class BinaryFile final
    {
    public:
        void OpenRW(const std::string& path);
        void CreateRWTruncate(const std::string& path);
        void Close();

        bool IsOpen() const;

        std::uint64_t Size();
        void Seek(std::uint64_t pos);
        std::uint64_t Tell();

        void Flush();

        template<typename T>
        void WriteLE(const T& v)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            m_stream.write(reinterpret_cast<const char*>(&v), sizeof(T));
            if (!m_stream) throw FileException("Ошибка записи в файл.");
        }

        template<typename T>
        void ReadLE(T& v)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            m_stream.read(reinterpret_cast<char*>(&v), sizeof(T));
            if (!m_stream) throw FileException("Ошибка чтения из файла.");
        }

        void WriteBytes(const void* data, std::size_t size);
        void ReadBytes(void* data, std::size_t size);

        void WriteFixedString(const std::string& value, std::size_t fixedLen, char pad = ' ');
        std::string ReadFixedString(std::size_t fixedLen);

    private:
        std::fstream m_stream;
    };
}
