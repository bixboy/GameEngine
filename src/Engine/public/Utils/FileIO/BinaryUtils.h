#pragma once
#include <iostream>
#include <cstdint>
#include <cstddef>
#include "Containers/String.h"


namespace Bix::Reflection
{
    struct ClassInfo;
}

namespace BixEngine::Utils
{
    class BinaryWriter
    {
    public:
        explicit BinaryWriter(std::ostream& stream);

        bool WriteUint32(std::uint32_t value);
        bool WriteString(const String& value);

        template <typename T>
        bool WritePrimitive(const T& value)
        {
            stream_.write(reinterpret_cast<const char*>(&value), sizeof(T));
            return static_cast<bool>(stream_);
        }

        [[nodiscard]] bool Good() const noexcept;

    private:
        bool WriteBytes(const char* data, std::size_t length);

        std::ostream& stream_;
    };

    class BinaryReader
    {
    public:
        explicit BinaryReader(std::istream& stream);

        bool ReadBytes(char* data, std::size_t length);
        bool ReadUint32(std::uint32_t& value);
        bool ReadString(String& value);

        template <typename T>
        bool ReadPrimitive(T& value)
        {
            stream_.read(reinterpret_cast<char*>(&value), sizeof(T));
            return static_cast<bool>(stream_);
        }

        [[nodiscard]] bool Good() const noexcept;

    private:
        std::istream& stream_;
    };
}
