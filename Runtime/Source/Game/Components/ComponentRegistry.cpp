#include "Game/Components/ComponentRegistry.h"

#include "Game/Components/Component.h"
#include "Game/Components/SpriteComponent.h"
#include "Game/Components/SpriteAnimatorComponent.h"

namespace BixEngine::Game
{
    void RegisterBuiltinComponents()
    {
        Component::StaticClass();
        SpriteComponent::StaticClass();
        SpriteAnimatorComponent::StaticClass();
    }
}
