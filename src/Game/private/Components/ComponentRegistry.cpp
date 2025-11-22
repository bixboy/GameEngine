#include "Components/ComponentRegistry.h"
#include "Components/Component.h"
#include "Components/SpriteComponent.h"
#include "Components/SpriteAnimatorComponent.h"
#include "Components/AudioSourceComponent.h"

namespace BixEngine::Game
{
    void RegisterBuiltinComponents()
    {
        Component::StaticClass();
        SpriteComponent::StaticClass();
        SpriteAnimatorComponent::StaticClass();
        AudioSourceComponent::StaticClass();
    }
}
