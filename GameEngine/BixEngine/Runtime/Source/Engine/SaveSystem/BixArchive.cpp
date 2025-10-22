#include "Bix/Engine/SaveSystem/BixArchive.h"

#include <iomanip>
#include <sstream>

namespace BixEngine::Engine::SaveSystem
{
    void BixArchiveWriter::WriteGuid(const BixGuid& guid)
    {
        stream_.write(reinterpret_cast<const char*>(guid.data.data()), static_cast<std::streamsize>(guid.data.size()));
    }

    void BixArchiveWriter::WriteString(const String& value)
    {
        const auto view = value.View();
        const auto size = static_cast<std::uint32_t>(view.size());
        WritePrimitive(size);
        if (size == 0)
            return;
        stream_.write(view.data(), static_cast<std::streamsize>(view.size()));
    }

    void BixArchiveWriter::WriteStdString(const std::string& value)
    {
        const auto size = static_cast<std::uint32_t>(value.size());
        WritePrimitive(size);
        if (size == 0)
            return;
        stream_.write(value.data(), static_cast<std::streamsize>(value.size()));
    }

    void BixArchiveWriter::WriteVector2(const Math::Vector2& value)
    {
        WritePrimitive(value.x);
        WritePrimitive(value.y);
    }

    void BixArchiveWriter::WriteVector3(const Math::Vector3& value)
    {
        WritePrimitive(value.x);
        WritePrimitive(value.y);
        WritePrimitive(value.z);
    }

    void BixArchiveWriter::WriteRotator(const Math::Rotator& value)
    {
        WritePrimitive(value.pitch);
        WritePrimitive(value.yaw);
        WritePrimitive(value.roll);
    }

    void BixArchiveWriter::WriteTransform(const Math::Transform& value)
    {
        WriteVector3(value.position);
        WriteRotator(value.rotation);
        WriteVector3(value.scale);
    }

    void BixArchiveWriter::WriteColor(const SDL_Color& value)
    {
        stream_.write(reinterpret_cast<const char*>(&value), sizeof(SDL_Color));
    }

    void BixArchiveWriter::WriteObject(const BixObject* object)
    {
        const bool hasObject = object != nullptr;
        WritePrimitive(hasObject);
        if (!hasObject)
            return;
        SerializeObject(*this, *object);
    }

    BixGuid BixArchiveReader::ReadGuid()
    {
        BixGuid guid;
        stream_.read(reinterpret_cast<char*>(guid.data.data()), static_cast<std::streamsize>(guid.data.size()));
        if (!stream_)
            throw std::runtime_error("Failed to read GUID from archive.");
        return guid;
    }

    String BixArchiveReader::ReadString()
    {
        const auto text = ReadStdString();
        return String(text);
    }

    std::string BixArchiveReader::ReadStdString()
    {
        std::uint32_t size = 0;
        ReadPrimitive(size);
        std::string result(size, '\0');
        if (size == 0)
            return result;

        stream_.read(result.data(), static_cast<std::streamsize>(size));
        if (!stream_)
            throw std::runtime_error("Failed to read string from archive.");
        return result;
    }

    Math::Vector2 BixArchiveReader::ReadVector2()
    {
        Math::Vector2 value{};
        ReadPrimitive(value.x);
        ReadPrimitive(value.y);
        return value;
    }

    Math::Vector3 BixArchiveReader::ReadVector3()
    {
        Math::Vector3 value{};
        ReadPrimitive(value.x);
        ReadPrimitive(value.y);
        ReadPrimitive(value.z);
        return value;
    }

    Math::Rotator BixArchiveReader::ReadRotator()
    {
        Math::Rotator value{};
        ReadPrimitive(value.pitch);
        ReadPrimitive(value.yaw);
        ReadPrimitive(value.roll);
        return value;
    }

    Math::Transform BixArchiveReader::ReadTransform()
    {
        Math::Transform value{};
        value.position = ReadVector3();
        value.rotation = ReadRotator();
        value.scale = ReadVector3();
        return value;
    }

    SDL_Color BixArchiveReader::ReadColor()
    {
        SDL_Color color{};
        stream_.read(reinterpret_cast<char*>(&color), sizeof(SDL_Color));
        if (!stream_)
            throw std::runtime_error("Failed to read color from archive.");
        return color;
    }

    std::unique_ptr<BixObject> BixArchiveReader::ReadObject(BixObject* outer)
    {
        bool hasObject = false;
        ReadPrimitive(hasObject);
        if (!hasObject)
            return nullptr;
        return DeserializeObject(*this, outer);
    }
}

