#include "Utils/FileIO/BinaryUtils.h"


namespace BixEngine::Utils
{
    // ==============================================================================
    // BinaryWriter
    // ==============================================================================
    
    BinaryWriter::BinaryWriter(std::ostream& stream) : stream_(stream)
    {}

    bool BinaryWriter::Good() const noexcept
    {
        return static_cast<bool>(stream_);
    }

    bool BinaryWriter::WriteBytes(const char* data, std::size_t length)
    {
        if (length > 0)
            stream_.write(data, static_cast<std::streamsize>(length));
        
        return Good();
    }

    bool BinaryWriter::WriteUint32(std::uint32_t value)
    {
        return WriteBytes(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    bool BinaryWriter::WriteString(const String& value)
    {
        const auto len = static_cast<std::uint32_t>(value.size());
        return WriteUint32(len) && WriteBytes(value.c_str(), len);
    }
    

    // ==============================================================================
    // BinaryReader
    // ==============================================================================
    
    BinaryReader::BinaryReader(std::istream& stream) : stream_(stream)
    {}

    bool BinaryReader::Good() const noexcept
    {
        return static_cast<bool>(stream_);
    }

    bool BinaryReader::ReadBytes(char* data, std::size_t length)
    {
        if (length > 0)
            stream_.read(data, static_cast<std::streamsize>(length));
        
        return Good();
    }

    bool BinaryReader::ReadUint32(std::uint32_t& value)
    {
        return ReadBytes(reinterpret_cast<char*>(&value), sizeof(value));
    }

    bool BinaryReader::ReadString(String& value)
    {
        std::uint32_t len = 0;
        if (!ReadUint32(len))
            return false;

        if (len > 0)
        {
            value.resize(len);
            if (!ReadBytes(value.data(), len))
                return false;
        }
        else
        {
            value.clear();
        }
        return true;
    }
}