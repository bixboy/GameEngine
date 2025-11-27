#include "Framework/SceneRegistry.h"
#include <unordered_map>
#include "Debug/Logger.h"
#include "Framework/Scene.h"


namespace
{
    using FactoryMap = std::unordered_map<BixEngine::String, BixEngine::Game::SceneFactory>;

    FactoryMap& GetSceneFactoryMap()
    {
        static FactoryMap factories;
        return factories;
    }
}

namespace BixEngine::Game
{
    void SceneRegistry::Register(const String& name, SceneFactory factory)
    {
        if (name.empty())
        {
            LOG_ERROR("Attempted to register a scene with an empty name.");
            return;
        }

        if (!factory)
        {
            LOG_ERROR("Attempted to register a scene without a valid factory: " + name);
            return;
        }

        auto& registry = GetSceneFactoryMap();

        if (const auto it = registry.find(name); it != registry.end())
        {
            LOG_WARNING("Scene already registered, replacing existing factory: " + name);
        }

        registry[name] = std::move(factory);
    }

    std::unique_ptr<Scene> SceneRegistry::Create(const String& name)
    {
        auto& registry = GetSceneFactoryMap();

        if (const auto it = registry.find(name); it != registry.end())
        {
            if (it->second)
                return it->second();

            LOG_ERROR("Scene factory for '" + name + "' is invalid.");
            return nullptr;
        }

        LOG_ERROR("Scene not registered: " + name);
        return nullptr;
    }
}

