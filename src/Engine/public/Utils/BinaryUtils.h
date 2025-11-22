#pragma once
#include <iostream>
#include "Containers/String.h"


namespace BixEngine::Utils
{
    class BinaryWriter
    {
    public:
        explicit BinaryWriter(std::ostream& stream);

        bool WriteUint32(std::uint32_t value);
        bool WriteString(const String& value);

        [[nodiscard]] bool Good() const noexcept;

    private:
        bool WriteBytes(const char* data, std::size_t length);

        std::ostream& stream_;
    };

    class BinaryReader
    {
    public:
        explicit BinaryReader(std::istream& stream);

        bool ReadUint32(std::uint32_t& value);
        bool ReadString(String& value);

        [[nodiscard]] bool Good() const noexcept;

    private:
        std::istream& stream_;
    };
}
