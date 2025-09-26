#pragma once
#include <memory>
#include <vector>
#include "Math/Math.h"

namespace Engine::Graphics { class Renderer; }

namespace Engine::Game
{
    class Component;
    
    class Actor
    {
        public:
            explicit Actor(const Math::Transform& transform = Math::Transform());
            virtual ~Actor();

            virtual void BeginPlay();
            virtual void Update(float deltaTime);
            virtual void Render(Graphics::Renderer& renderer) const;

             void AddComponent(std::unique_ptr<Component> component);

            [[nodiscard]] Math::Vector3 GetPosition() const noexcept { return transform_.position; }
            [[nodiscard]] Math::Rotator GetRotation() const noexcept { return transform_.rotation; }
            [[nodiscard]] Math::Vector3 GetScale() const noexcept { return transform_.scale; }

            void SetPosition(const Math::Vector3& position) { transform_.position = position; }
            void SetRotation(const Math::Rotator& rotation) { transform_.rotation = rotation; }
            void SetScale(const Math::Vector3& scale)       { transform_.scale = scale; }

            void SetPositionX(float x) { transform_.position.x = x; }
            void SetPositionY(float y) { transform_.position.y = y; }
            void SetPositionZ(float z) { transform_.position.z = z; }

            void SetScaleX(float x) { transform_.scale.x = x; }
            void SetScaleY(float y) { transform_.scale.y = y; }
            void SetScaleZ(float z) { transform_.scale.z = z; }

            void SetRotationPitch(float pitch) { transform_.rotation.pitch = pitch; }
            void SetRotationYaw(float yaw)     { transform_.rotation.yaw = yaw; }
            void SetRotationRoll(float roll)   { transform_.rotation.roll = roll; }

        private:
        Math::Transform transform_;
        
        std::vector<std::unique_ptr<Component>> components_;

        bool hasBegunPlay_{false};
    };
}
