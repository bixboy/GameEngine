#pragma once

#include "Bix/Core/String.h"
#include "Bix/Engine/SaveSystem/BixReflection.h"
#include "Bix/Math/Math.h"

namespace BixEngine::Game
{
    class Object : public Engine::SaveSystem::BixObject
    {
    public:
        BIX_CLASS(Object, ::BixEngine::Engine::SaveSystem::BixObject);

        Object();
        explicit Object(String name);
        Object(String name, const Math::Transform& transform);
        ~Object() override = default;

        [[nodiscard]] virtual String GetTypeName() const noexcept;

        [[nodiscard]] String GetUUID() const { return GetGuid().ToString(); }
        void SetUUID(const String& uuid);

        [[nodiscard]] const String& GetName() const noexcept { return name_; }
        void SetName(String name);

        [[nodiscard]] const Math::Transform& GetTransform() const noexcept { return transform_; }
        void SetTransform(const Math::Transform& transform) noexcept { transform_ = transform; }

        [[nodiscard]] const Math::Vector3& GetPosition() const noexcept { return transform_.position; }
        void SetPosition(const Math::Vector3& position) noexcept { transform_.position = position; }
        void SetPosition(float x, float y, float z = 0.0f) noexcept { transform_.position = { x, y, z }; }

        [[nodiscard]] const Math::Rotator& GetRotation() const noexcept { return transform_.rotation; }
        void SetRotation(const Math::Rotator& rotation) noexcept { transform_.rotation = rotation; }

        [[nodiscard]] const Math::Vector3& GetScale() const noexcept { return transform_.scale; }
        void SetScale(const Math::Vector3& scale) noexcept { transform_.scale = scale; }

    protected:
        static void RegisterProperties(::BixEngine::Engine::SaveSystem::BixClass& cls);

    private:
        BIX_PROPERTY(String, name_);
        BIX_PROPERTY(Math::Transform, transform_, {});
    };
}

