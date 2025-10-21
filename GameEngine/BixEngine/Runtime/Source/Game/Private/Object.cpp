#include "Bix/Game/Object.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <istream>
#include <ostream>
#include <random>
#include <sstream>
#include <stdexcept>

namespace BixEngine::Game
{
    namespace
    {
        constexpr std::uint32_t kBinaryFormatVersion = 1;
    }

    Object::Object() : Object("Object")
    {
    }

    Object::Object(String name) : uuid_(GenerateUUID()), name_(std::move(name))
    {
    }

    Object::Object(String name, const Math::Transform& transform) : uuid_(GenerateUUID()), name_(std::move(name)), transform_(transform)
    {
    }

    void Object::SetUUID(String uuid)
    {
        uuid_ = std::move(uuid);
    }

    void Object::SetName(String name)
    {
        name_ = std::move(name);
    }

    String Object::GetTypeName() const noexcept
    {
        return "Object";
    }

    void Object::SerializeBinary(std::ostream& stream) const
    {
        WritePrimitive(stream, kBinaryFormatVersion);
        WriteString(stream, uuid_);
        WriteString(stream, name_);
        SerializeTransform(stream);
        SerializeBinaryImpl(stream);
    }

    void Object::DeserializeBinary(std::istream& stream)
    {
        std::uint32_t version = 0;
        ReadPrimitive(stream, version);
        
        if (version != kBinaryFormatVersion)
            throw std::runtime_error("Unsupported object binary version.");

        uuid_ = ReadString(stream);
        name_ = ReadString(stream);
        DeserializeTransform(stream);
        DeserializeBinaryImpl(stream);
    }

    void Object::WriteString(std::ostream& stream, const String& value)
    {
        const auto length = static_cast<std::uint32_t>(value.size());

        WritePrimitive(stream, length);
        stream.write(value.data(), static_cast<std::streamsize>(length));
    }

    String Object::ReadString(std::istream& stream)
    {
        std::uint32_t length = 0;
        ReadPrimitive(stream, length);

        String result(length, '\0');
        stream.read(result.data(), static_cast<std::streamsize>(length));

        if (!stream)
            throw std::runtime_error("Failed to read string from stream.");

        return result;
    }

    String Object::GenerateUUID()
    {
        std::array<std::uint8_t, 16> data{};

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, 255);

        for (auto& byte : data)
            byte = static_cast<std::uint8_t>(dist(gen));

        data[6] = static_cast<std::uint8_t>((data[6] & 0x0F) | 0x40);
        data[8] = static_cast<std::uint8_t>((data[8] & 0x3F) | 0x80);

        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        for (std::size_t i = 0; i < data.size(); ++i)
        {
            oss << std::setw(2) << static_cast<int>(data[i]);
            if (i == 3 || i == 5 || i == 7 || i == 9)
                oss << '-';
        }

        return oss.str();
    }

    void Object::SerializeTransform(std::ostream& stream) const
    {
        WritePrimitive(stream, transform_.position.x);
        WritePrimitive(stream, transform_.position.y);
        WritePrimitive(stream, transform_.position.z);

        WritePrimitive(stream, transform_.rotation.pitch);
        WritePrimitive(stream, transform_.rotation.yaw);
        WritePrimitive(stream, transform_.rotation.roll);

        WritePrimitive(stream, transform_.scale.x);
        WritePrimitive(stream, transform_.scale.y);
        WritePrimitive(stream, transform_.scale.z);
    }

    void Object::DeserializeTransform(std::istream& stream)
    {
        ReadPrimitive(stream, transform_.position.x);
        ReadPrimitive(stream, transform_.position.y);
        ReadPrimitive(stream, transform_.position.z);

        ReadPrimitive(stream, transform_.rotation.pitch);
        ReadPrimitive(stream, transform_.rotation.yaw);
        ReadPrimitive(stream, transform_.rotation.roll);

        ReadPrimitive(stream, transform_.scale.x);
        ReadPrimitive(stream, transform_.scale.y);
        ReadPrimitive(stream, transform_.scale.z);
    }
}
