#include "Components/Core/ComponentRegistry.h"
#include "Components/Core/Component.h"
#include "Components/Sprite/SpriteComponent.h"
#include "Components/Sprite/SpriteAnimatorComponent.h"
#include "Components/Audio/AudioSourceComponent.h"

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
