#pragma once

#include <iosfwd>
#include <string>
#include <string_view>

#include "Math/Math.h"

namespace nlohmann { class json; }

namespace Engine::Game
{
    class Object
    {
    public:
        Object();
        explicit Object(std::string name);
        Object(std::string name, const Math::Transform& transform);
        virtual ~Object() = default;

        [[nodiscard]] std::string_view GetUUID() const noexcept { return uuid_; }
        void SetUUID(std::string uuid);

        [[nodiscard]] std::string_view GetName() const noexcept { return name_; }
        void SetName(std::string name);

        [[nodiscard]] virtual std::string_view GetTypeName() const noexcept;

        [[nodiscard]] const Math::Transform& GetTransform() const noexcept { return transform_; }
        void SetTransform(const Math::Transform& transform) noexcept { transform_ = transform; }

        [[nodiscard]] Math::Vector3 GetPosition() const noexcept { return transform_.position; }
        void SetPosition(const Math::Vector3& position) noexcept { transform_.position = position; }
        void SetPositionX(float x) noexcept { transform_.position.x = x; }
        void SetPositionY(float y) noexcept { transform_.position.y = y; }
        void SetPositionZ(float z) noexcept { transform_.position.z = z; }

        [[nodiscard]] Math::Rotator GetRotation() const noexcept { return transform_.rotation; }
        void SetRotation(const Math::Rotator& rotation) noexcept { transform_.rotation = rotation; }
        void SetRotationPitch(float pitch) noexcept { transform_.rotation.pitch = pitch; }
        void SetRotationYaw(float yaw) noexcept { transform_.rotation.yaw = yaw; }
        void SetRotationRoll(float roll) noexcept { transform_.rotation.roll = roll; }

        [[nodiscard]] Math::Vector3 GetScale() const noexcept { return transform_.scale; }
        void SetScale(const Math::Vector3& scale) noexcept { transform_.scale = scale; }
        void SetScaleX(float x) noexcept { transform_.scale.x = x; }
        void SetScaleY(float y) noexcept { transform_.scale.y = y; }
        void SetScaleZ(float z) noexcept { transform_.scale.z = z; }

        virtual void SerializeJson(nlohmann::json& json) const;
        virtual void DeserializeJson(const nlohmann::json& json);

        virtual void SerializeBinary(std::ostream& stream) const;
        virtual void DeserializeBinary(std::istream& stream);

    protected:
        [[nodiscard]] Math::Transform& GetMutableTransform() noexcept { return transform_; }

        virtual void SerializeJsonImpl(nlohmann::json& json) const {}
        virtual void DeserializeJsonImpl(const nlohmann::json& json) {}
        virtual void SerializeBinaryImpl(std::ostream& stream) const {}
        virtual void DeserializeBinaryImpl(std::istream& stream) {}

        template<typename T>
        static void WritePrimitive(std::ostream& stream, const T& value)
        {
            stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
        }

        template<typename T>
        static void ReadPrimitive(std::istream& stream, T& value)
        {
            stream.read(reinterpret_cast<char*>(&value), sizeof(T));
        }

        static void WriteString(std::ostream& stream, std::string_view value);
        static std::string ReadString(std::istream& stream);

    private:
        static std::string GenerateUUID();

        void SerializeTransform(nlohmann::json& json) const;
        void DeserializeTransform(const nlohmann::json& json);
        void SerializeTransform(std::ostream& stream) const;
        void DeserializeTransform(std::istream& stream);

        std::string uuid_;
        std::string name_;
        Math::Transform transform_{};
    };
}
