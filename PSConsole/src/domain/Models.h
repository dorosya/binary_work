#pragma once
#include <cstdint>
#include <string>
#include <optional>

namespace ps
{
    enum class ComponentType : std::uint8_t
    {
        Product = 0,
        Node = 1,
        Detail = 2
    };

    inline std::string ToString(ComponentType type)
    {
        switch (type)
        {
        case ComponentType::Product: return "Изделие";
        case ComponentType::Node: return "Узел";
        case ComponentType::Detail: return "Деталь";
        default: return "Неизвестно";
        }
    }

    inline std::optional<ComponentType> ParseComponentType(const std::string& s)
    {
        if (s == "Изделие" || s == "изделие") return ComponentType::Product;
        if (s == "Узел" || s == "узел") return ComponentType::Node;
        if (s == "Деталь" || s == "деталь") return ComponentType::Detail;
        return std::nullopt;
    }

    struct ComponentRecord
    {
        bool deleted = false;
        std::uint32_t firstSpecPtr = 1; // NULL = 1
        std::uint32_t nextPtr = 1;
        ComponentType type = ComponentType::Detail;
        std::string name;
        std::uint32_t fileOffset = 0;
    };

    struct SpecRecord
    {
        bool deleted = false;
        std::uint32_t componentPtr = 1;
        std::uint16_t qty = 1;
        std::uint32_t nextPtr = 1;
        std::uint32_t fileOffset = 0;
    };

    struct ProductFileHeader
    {
        std::uint16_t dataLen = 0;
        std::uint32_t headPtr = 1;
        std::uint32_t freePtr = 0;
        std::string specFileName; // 16 bytes fixed
    };
}
