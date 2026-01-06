#include "Components/Core/ComponentRegistry.h"
#include "Components/Core/Component.h"
#include "Debug/Logger.h"
#include <type_traits>
#include "Components/Audio/AudioSourceComponent.h"
#include "Components/Core/BoxColliderComponent.h"
#include "Components/Sprite/SpriteAnimatorComponent.h"
#include "Components/Core/CameraComponent.h"
#include "Components/Sprite/SpriteComponent.h"


namespace BixEngine::Game
{
    template<typename T>
    void RegisterComponent()
    {
        static_assert(std::is_base_of_v<Component, T>, "Registered type must derive from Component!");
        static_assert(!std::is_abstract_v<T>, "Cannot register abstract component class!");

        const auto& cls = T::StaticClass(); 
        
        if (auto* info = const_cast<Reflection::ClassInfo*>(&cls))
        {
            info->ConstructorFn = [](void*) -> void*
            {
                return new T();
            };
            
            LOG_INFO("Registered component: " + String(info->Name));
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
        RegisterComponent<CameraComponent>();
        
        RegisterComponent<AudioSourceComponent>();
        RegisterComponent<BoxColliderComponent>();
        
    }
}
