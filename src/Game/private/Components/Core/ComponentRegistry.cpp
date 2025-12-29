#include "Components/Core/ComponentRegistry.h"
#include "Components/Core/Component.h"
#include "Components/Sprite/SpriteComponent.h"
#include "Components/Sprite/SpriteAnimatorComponent.h"
#include "Components/Audio/AudioSourceComponent.h"
#include "Components/Core/BoxColliderComponent.h"
#include "Components/Core/CameraComponent.h" 
#include "Core/Registry.h"
#include "Core/ClassInfo.h"
#include "Debug/Logger.h"

namespace BixEngine::Game
{
    template<typename T>
    void RegisterComponent()
    {
        
        const auto& cls = T::StaticClass(); 
        
        
        if (auto* info = const_cast<Bix::Reflection::ClassInfo*>(&cls))
        {
            
            
            info->ConstructorFn = [](void*) -> void*
            {
                
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
        RegisterComponent<CameraComponent>(); 
    }
}
