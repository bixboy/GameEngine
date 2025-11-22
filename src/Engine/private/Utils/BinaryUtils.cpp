#include "Utils/BinaryUtils.h"

namespace BixEngine::Utils
{
    // BinaryWriter implementation
    BinaryWriter::BinaryWriter(std::ostream& stream) : stream_(stream)
    {
    }

    bool BinaryWriter::WriteUint32(std::uint32_t value)
    {
        stream_.write(reinterpret_cast<const char*>(&value), sizeof(value));
        return static_cast<bool>(stream_);
    }

    bool BinaryWriter::WriteString(const String& value)
    {
        const auto length = static_cast<std::uint32_t>(value.size());
        return WriteUint32(length) && WriteBytes(value.c_str(), value.size());
    }

    bool BinaryWriter::Good() const noexcept
    {
        return static_cast<bool>(stream_);
    }

    bool BinaryWriter::WriteBytes(const char* data, std::size_t length)
    {
        if (length == 0)
            return true;

        stream_.write(data, static_cast<std::streamsize>(length));
        return static_cast<bool>(stream_);
    }

    // BinaryReader implementation
    BinaryReader::BinaryReader(std::istream& stream) : stream_(stream)
    {
    }

    bool BinaryReader::ReadUint32(std::uint32_t& value)
    {
        stream_.read(reinterpret_cast<char*>(&value), sizeof(value));
        return static_cast<bool>(stream_);
    }

    bool BinaryReader::ReadString(String& value)
    {
        std::uint32_t length = 0;
        if (!ReadUint32(length))
            return false;

        value.resize(length, '\0');
        if (length == 0)
            return true;

        stream_.read(value.data(), length);
        return static_cast<bool>(stream_);
    }

    bool BinaryReader::Good() const noexcept
    {
        return static_cast<bool>(stream_);
    }
}
