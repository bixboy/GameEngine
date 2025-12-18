#pragma once
#include <istream>
#include <ostream>
#include "Containers/String.h"
#include "Math/Transform.h"
#include "Object.generated.h"


namespace BixEngine::Game
{
    BCLASS()

    class Object
    {
        GENERATED_BODY()

    public:
        Object();
        explicit Object(String name);

        Object(String name, const Math::Transform& transform);
        virtual ~Object() = default;

        [[nodiscard]] virtual String GetTypeName() const noexcept;

        [[nodiscard]] String GetUUID() const noexcept { return uuid_; }
        void SetUUID(String uuid);

        [[nodiscard]] String GetName() const noexcept { return name_; }
        void SetName(String name);

        [[nodiscard]] Math::Transform GetTransform() const noexcept { return transform_; }
        [[nodiscard]] const Math::Transform& GetTransformRef() const noexcept { return transform_; }
        [[nodiscard]] Math::Transform& GetTransformRef() noexcept { return transform_; }
        void SetTransform(const Math::Transform& transform) noexcept { transform_ = transform; }

        [[nodiscard]] Math::Vector3 GetPosition() const noexcept { return transform_.position; }
        [[nodiscard]] Math::Vector3 GetWorldPosition() const noexcept { return transform_.GetWorldPosition(); }
        void SetPosition(const Math::Vector3& position) noexcept { transform_.position = position; }
        void SetPosition(float x, float y, float z = 0.0f) noexcept { transform_.position = {x, y, z}; }

        [[nodiscard]] Math::Rotator GetRotation() const noexcept { return transform_.rotation; }
        void SetRotation(const Math::Rotator& rotation) noexcept { transform_.rotation = rotation; }

        [[nodiscard]] Math::Vector3 GetScale() const noexcept { return transform_.scale; }
        void SetScale(const Math::Vector3& scale) noexcept { transform_.scale = scale; }

        virtual void SerializeBinary(std::ostream& stream) const;
        virtual void DeserializeBinary(std::istream& stream);

    protected:
        virtual void SerializeBinaryImpl(std::ostream&) const
        {
        }

        virtual void DeserializeBinaryImpl(std::istream&)
        {
        }

        template <typename T>
        static void WritePrimitive(std::ostream& stream, const T& value)
        {
            stream.write(reinterpret_cast<const char*>(&value), sizeof(T));
        }

        template <typename T>
        static void ReadPrimitive(std::istream& stream, T& value)
        {
            stream.read(reinterpret_cast<char*>(&value), sizeof(T));
        }

        static void WriteString(std::ostream& stream, const String& value);
        static String ReadString(std::istream& stream);

    private:
        static String GenerateUUID();

        void SerializeTransform(std::ostream& stream) const;
        void DeserializeTransform(std::istream& stream);

        String uuid_;
        String name_;
        Math::Transform transform_{};
    };
}
