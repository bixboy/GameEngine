#include "Components/Core/ComponentRegistry.h"
#include "Components/Core/Component.h"
#include "Components/Sprite/SpriteComponent.h"
#include "Components/Sprite/SpriteAnimatorComponent.h"
#include "Components/Audio/AudioSourceComponent.h"
#include "Components/Core/BoxColliderComponent.h"
#include "Components/Core/CameraComponent.h" // Added
#include "Core/Registry.h"
#include "Core/ClassInfo.h"
#include "Debug/Logger.h"

namespace BixEngine::Game
{
    template<typename T>
    void RegisterComponent()
    {
        // Force static initialization
        const auto& cls = T::StaticClass(); 
        
        // Ensure registry has it (it should from StaticClass, but let's be safe)
        if (auto* info = const_cast<Bix::Reflection::ClassInfo*>(&cls))
        {
            // Always override with a known-good constructor lambda
            // The auto-generated one is seemingly unreliable/broken
            info->ConstructorFn = [](void*) -> void*
            {
                // Ensure T has default constructor
                return new T();
            };
        }
        else
        {
            LOG_ERROR("RegisterComponent: Failed to get ClassInfo for " + String(typeid(T).name()));
        }
    }

    void RegisterBuiltinComponents()
    {
        RegisterComponent<Component>();
        RegisterComponent<SpriteComponent>();
        RegisterComponent<SpriteAnimatorComponent>();
        RegisterComponent<AudioSourceComponent>();
        RegisterComponent<BoxColliderComponent>();
        RegisterComponent<CameraComponent>(); // Added
    }
}
