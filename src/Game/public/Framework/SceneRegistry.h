#pragma once
#include <functional>
#include <memory>
#include "Containers/String.h"


namespace BixEngine::Game
{
    class Scene;

    using SceneFactory = std::function<std::unique_ptr<Scene>()>;

    class SceneRegistry
    {
    public:
        static void Register(const String& name, SceneFactory factory);
        [[nodiscard]] static std::unique_ptr<Scene> Create(const String& name);
    };
}

#define REGISTER_SCENE(Type)                                                   \
    namespace                                                                 \
    {                                                                         \
        [[maybe_unused]] const bool _scene_registered_##Type = []()           \
        {                                                                     \
            ::BixEngine::Game::SceneRegistry::Register(                       \
                #Type,                                                        \
                []() -> std::unique_ptr<::BixEngine::Game::Scene>             \
                {                                                             \
                    return std::make_unique<Type>();                          \
                });                                                           \
            return true;                                                      \
        }();                                                                  \
    }

