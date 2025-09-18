#pragma once

#include <SDL3/SDL_pixels.h>

#include "Math/Math.h"

namespace Engine::Graphics {
class Renderer;
}

namespace Engine::Game {

class Entity {
public:
    Entity(const Math::Transform& transform = Math::Transform(),
           SDL_Color color = {255, 255, 255, 255});

    Entity(float x, float y, float w, float h, SDL_Color color);

    void Update(float deltaTime);
    void Render(Graphics::Renderer& renderer) const;

    [[nodiscard]] Math::Vector3 GetPosition() const noexcept { return transform_.position; }
    [[nodiscard]] Math::Rotator GetRotation() const noexcept { return transform_.rotation; }
    [[nodiscard]] Math::Vector3 GetScale() const noexcept { return transform_.scale; }

    void SetPosition(const Math::Vector3& position) { transform_.position = position; }
    void SetRotation(const Math::Rotator& rotation) { transform_.rotation = rotation; }
    void SetScale(const Math::Vector3& scale) { transform_.scale = scale; }

    void SetPositionX(float x) { transform_.position.x = x; }
    void SetPositionY(float y) { transform_.position.y = y; }
    void SetPositionZ(float z) { transform_.position.z = z; }

    void SetScaleX(float x) { transform_.scale.x = x; }
    void SetScaleY(float y) { transform_.scale.y = y; }
    void SetScaleZ(float z) { transform_.scale.z = z; }

    void SetRotationPitch(float pitch) { transform_.rotation.pitch = pitch; }
    void SetRotationYaw(float yaw) { transform_.rotation.yaw = yaw; }
    void SetRotationRoll(float roll) { transform_.rotation.roll = roll; }

private:
    Math::Transform transform_{};
    SDL_Color color_{};
};

}  // namespace Engine::Game
