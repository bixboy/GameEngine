#pragma once
#include "Vector3.h"
#include "Rotator.h"

struct Transform
{
    Vector3 position;
    Rotator rotation;
    Vector3 scale;

    Transform()
        : position(Vector3::Zero()), rotation(Rotator::Zero()), scale(Vector3::One()) {}

    Transform(const Vector3& pos, const Rotator& rot, const Vector3& scl)
        : position(pos), rotation(rot), scale(scl) {}
};
