#pragma once
#include "SDL3/SDL_pixels.h"
#include "../Math/Math.h"

class Renderer;

class Entity
{
public:
    Entity(const Transform& transform = Transform(),
           SDL_Color color = {255, 255, 255, 255});

    Entity(float x, float y, float w, float h, SDL_Color color);

    void Update(float dt);
    void Render(Renderer& renderer);

    Vector3 GetPosition() const { return transform.position; }
    Rotator GetRotation() const { return transform.rotation; }
    Vector3 GetScale()    const { return transform.scale; }

    // --- Setters ---
    void SetPosition(const Vector3& position) { transform.position = position; }
    void SetRotation(const Rotator& rotation) { transform.rotation = rotation; }
    void SetScale(const Vector3& scale)       { transform.scale = scale; }

    // --- Partial setters ---
    void SetPositionX(float x) { transform.position.x = x; }
    void SetPositionY(float y) { transform.position.y = y; }
    void SetPositionZ(float z) { transform.position.z = z; }

    void SetScaleX(float x) { transform.scale.x = x; }
    void SetScaleY(float y) { transform.scale.y = y; }
    void SetScaleZ(float z) { transform.scale.z = z; }

    void SetRotationPitch(float p) { transform.rotation.pitch = p; }
    void SetRotationYaw(float y)   { transform.rotation.yaw   = y; }
    void SetRotationRoll(float r)  { transform.rotation.roll  = r; }

private:
    Transform transform;
    SDL_Color color;
};
