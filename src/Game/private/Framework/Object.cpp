#include "Framework/Object.h"
#include <array>
#include <cstdint>
#include <random>
#include <stdexcept>


namespace BixEngine::Game
{
    namespace
    {
        constexpr std::uint32_t kBinaryFormatVersion = 1;
    }

    // --- Constructeurs ---

    Object::Object() : Object("Object")
    {
    }

    Object::Object(String name) : uuid_(GenerateUUID()), name_(std::move(name))
    {
    }

    Object::Object(String name, const Math::Transform& transform) : uuid_(GenerateUUID()), name_(std::move(name)), transform_(transform)
    {
    }

    // --- Getters / Setters ---

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
    
    String Object::GenerateUUID()
    {
        thread_local std::mt19937 gen([]()
            {
                std::random_device rd;
                return std::mt19937(rd());
            }());
        
        std::uniform_int_distribution<int> dist(0, 255);
        std::array<std::uint8_t, 16> data{};

        for (auto& byte : data)
            byte = static_cast<std::uint8_t>(dist(gen));

        data[6] = (data[6] & 0x0F) | 0x40;
        data[8] = (data[8] & 0x3F) | 0x80;
        
        String uuidStr(36, 0); 
        static constexpr char hexChars[] = "0123456789abcdef";

        int dstIndex = 0;
        for (size_t i = 0; i < 16; ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
                uuidStr[dstIndex++] = '-';

            uuidStr[dstIndex++] = hexChars[(data[i] >> 4) & 0x0F];
            uuidStr[dstIndex++] = hexChars[data[i] & 0x0F];
        }

        return uuidStr;
    }

    // --- Sérialisation ---

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
        
        if (length > 0)
            stream.write(value.data(), length);
    }

    String Object::ReadString(std::istream& stream)
    {
        std::uint32_t length = 0;
        ReadPrimitive(stream, length);

        if (length == 0) return "";

        String result(length, '\0');
        stream.read(result.data(), length);

        if (!stream)
            throw std::runtime_error("Failed to read string from stream.");

        return result;
    }

    void Object::SerializeTransform(std::ostream& stream) const
    {
        const auto pos = transform_.GetLocalPosition();
        WritePrimitive(stream, pos.x);
        WritePrimitive(stream, pos.y);
        WritePrimitive(stream, pos.z);

        const auto rot = transform_.GetLocalRotation();
        WritePrimitive(stream, rot.pitch);
        WritePrimitive(stream, rot.yaw);
        WritePrimitive(stream, rot.roll);

        const auto scale = transform_.GetLocalScale();
        WritePrimitive(stream, scale.x);
        WritePrimitive(stream, scale.y);
        WritePrimitive(stream, scale.z);
    }

    void Object::DeserializeTransform(std::istream& stream)
    {
        float px, py, pz;
        ReadPrimitive(stream, px);
        ReadPrimitive(stream, py);
        ReadPrimitive(stream, pz);
        transform_.SetPosition({px, py, pz});

        float rp, ry, rr;
        ReadPrimitive(stream, rp);
        ReadPrimitive(stream, ry);
        ReadPrimitive(stream, rr);
        transform_.SetRotation({rp, ry, rr});

        float sx, sy, sz;
        ReadPrimitive(stream, sx);
        ReadPrimitive(stream, sy);
        ReadPrimitive(stream, sz);
        transform_.SetScale({sx, sy, sz});
    }
}