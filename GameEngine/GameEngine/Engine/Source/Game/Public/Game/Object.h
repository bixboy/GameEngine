#pragma once

#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include "Math/Math.h"

namespace Engine::Game
{
    class Object
    {
    public:
        Object();
        explicit Object(std::string name);
        Object(std::string name, const Math::Transform& transform);
        virtual ~Object() = default;

        [[nodiscard]] virtual std::string_view GetTypeName() const noexcept;

        [[nodiscard]] const std::string& GetUUID() const noexcept { return uuid_; }
        void SetUUID(std::string uuid);

        [[nodiscard]] const std::string& GetName() const noexcept { return name_; }
        void SetName(std::string name);

        [[nodiscard]] const Math::Transform& GetTransform() const noexcept { return transform_; }
        void SetTransform(const Math::Transform& transform) noexcept { transform_ = transform; }

        [[nodiscard]] const Math::Vector3& GetPosition() const noexcept { return transform_.position; }
        void SetPosition(const Math::Vector3& position) noexcept { transform_.position = position; }
        void SetPosition(float x, float y, float z = 0.0f) noexcept { transform_.position = { x, y, z }; }

        [[nodiscard]] const Math::Rotator& GetRotation() const noexcept { return transform_.rotation; }
        void SetRotation(const Math::Rotator& rotation) noexcept { transform_.rotation = rotation; }

        [[nodiscard]] const Math::Vector3& GetScale() const noexcept { return transform_.scale; }
        void SetScale(const Math::Vector3& scale) noexcept { transform_.scale = scale; }

        virtual void SerializeBinary(std::ostream& stream) const;
        virtual void DeserializeBinary(std::istream& stream);

    protected:
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

        void SerializeTransform(std::ostream& stream) const;
        void DeserializeTransform(std::istream& stream);

        std::string uuid_;
        std::string name_;
        Math::Transform transform_{};
    };
}
