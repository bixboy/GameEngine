#include "Game/Object.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <stdexcept>

namespace Engine::Game
{
    namespace
    {
        constexpr std::uint32_t kBinaryFormatVersion = 1;
    }

    Object::Object()
        : Object("Object")
    {
    }

    Object::Object(std::string name)
        : uuid_(GenerateUUID()), name_(std::move(name))
    {
    }

    Object::Object(std::string name, const Math::Transform& transform)
        : uuid_(GenerateUUID()), name_(std::move(name)), transform_(transform)
    {
    }

    void Object::SetUUID(std::string uuid)
    {
        uuid_ = std::move(uuid);
    }

    void Object::SetName(std::string name)
    {
        name_ = std::move(name);
    }

    std::string_view Object::GetTypeName() const noexcept
    {
        return "Object";
    }

    void Object::SerializeJson(nlohmann::json& json) const
    {
        json["type"] = GetTypeName();
        json["uuid"] = uuid_;
        json["name"] = name_;

        auto& transformJson = json["transform"];
        SerializeTransform(transformJson);

        SerializeJsonImpl(json);
    }

    void Object::DeserializeJson(const nlohmann::json& json)
    {
        if (json.contains("uuid") && json["uuid"].is_string())
        {
            uuid_ = json["uuid"].get<std::string>();
        }
        else
        {
            uuid_ = GenerateUUID();
        }

        if (json.contains("name") && json["name"].is_string())
            name_ = json["name"].get<std::string>();

        if (json.contains("transform"))
            DeserializeTransform(json["transform"]);

        DeserializeJsonImpl(json);
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

    void Object::WriteString(std::ostream& stream, std::string_view value)
    {
        const auto length = static_cast<std::uint32_t>(value.size());
        WritePrimitive(stream, length);
        stream.write(value.data(), static_cast<std::streamsize>(length));
    }

    std::string Object::ReadString(std::istream& stream)
    {
        std::uint32_t length = 0;
        ReadPrimitive(stream, length);
        std::string result(length, '\0');
        stream.read(result.data(), static_cast<std::streamsize>(length));
        if (!stream)
            throw std::runtime_error("Failed to read string from stream.");
        return result;
    }

    std::string Object::GenerateUUID()
    {
        std::array<std::uint8_t, 16> data{};

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(0, 255);

        for (auto& byte : data)
            byte = static_cast<std::uint8_t>(dist(gen));

        // Set UUID version (4) and variant (RFC 4122)
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

    void Object::SerializeTransform(nlohmann::json& json) const
    {
        json = {
            {"position", {{"x", transform_.position.x}, {"y", transform_.position.y}, {"z", transform_.position.z}}},
            {"rotation", {{"pitch", transform_.rotation.pitch}, {"yaw", transform_.rotation.yaw}, {"roll", transform_.rotation.roll}}},
            {"scale", {{"x", transform_.scale.x}, {"y", transform_.scale.y}, {"z", transform_.scale.z}}}
        };
    }

    void Object::DeserializeTransform(const nlohmann::json& json)
    {
        if (json.contains("position"))
        {
            const auto& position = json["position"];
            transform_.position.x = position.value("x", transform_.position.x);
            transform_.position.y = position.value("y", transform_.position.y);
            transform_.position.z = position.value("z", transform_.position.z);
        }

        if (json.contains("rotation"))
        {
            const auto& rotation = json["rotation"];
            transform_.rotation.pitch = rotation.value("pitch", transform_.rotation.pitch);
            transform_.rotation.yaw = rotation.value("yaw", transform_.rotation.yaw);
            transform_.rotation.roll = rotation.value("roll", transform_.rotation.roll);
        }

        if (json.contains("scale"))
        {
            const auto& scale = json["scale"];
            transform_.scale.x = scale.value("x", transform_.scale.x);
            transform_.scale.y = scale.value("y", transform_.scale.y);
            transform_.scale.z = scale.value("z", transform_.scale.z);
        }
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
