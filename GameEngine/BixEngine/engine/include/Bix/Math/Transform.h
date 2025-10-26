#pragma once

#include "Bix/Math/Rotator.h"
#include "Bix/Math/Vector3.h"

namespace BixEngine::Math
{
    struct Transform
        {
        Vector3 position{Vector3::Zero()};
        Rotator rotation{Rotator::Zero()};
        Vector3 scale{Vector3::One()};

        Transform() = default;
        Transform(const Vector3& pos, const Rotator& rot, const Vector3& scl)
            : position(pos), rotation(rot), scale(scl) {}
    };
}
