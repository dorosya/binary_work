#pragma once
#include <stdexcept>
#include <string>

namespace ps
{
    class PsException : public std::runtime_error
    {
    public:
        explicit PsException(const std::string& message) : std::runtime_error(message) {}
    };

    class FileException final : public PsException
    {
    public:
        explicit FileException(const std::string& message) : PsException(message) {}
    };

    class ValidationException final : public PsException
    {
    public:
        explicit ValidationException(const std::string& message) : PsException(message) {}
    };
}
