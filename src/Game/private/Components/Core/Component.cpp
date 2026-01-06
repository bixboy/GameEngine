#include "Components/Core/Component.h"
#include "Framework/Actor.h"

namespace BixEngine::Game
{
    Math::Transform& Component::GetTransform()
    {
        return owner_->GetTransformRef();
    }

    const Math::Transform& Component::GetTransform() const
    {
        return owner_->GetTransformRef();
    }
}
