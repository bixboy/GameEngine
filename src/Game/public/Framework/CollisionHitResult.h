#pragma once
#include "Framework/Actor.h"
#include "Math/Vector2.h"

namespace BixEngine::Game
{
    struct CollisionHitResult
    {
        Actor* OtherActor = nullptr;
        Math::Vector2<float> ContactPoint;
        Math::Vector2<float> Normal;
        float Impulse = 0.0f;
    };
}
