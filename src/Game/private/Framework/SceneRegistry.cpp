#include "Framework/SceneRegistry.h"
#include <unordered_map>
#include <mutex>
#include "Debug/Logger.h"
#include "Framework/Scene.h"

namespace
{
    struct RegistryData
    {
        std::unordered_map<BixEngine::String, BixEngine::Game::SceneFactory> factories;
        std::mutex mutex;
    };

    RegistryData& GetRegistryData()
    {
        static RegistryData data;
        return data;
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

        auto& data = GetRegistryData();
        std::lock_guard lock(data.mutex);

        if (data.factories.contains(name))
        {
            LOG_WARNING("Scene already registered, replacing existing factory: " + name);
        }

        data.factories[name] = std::move(factory);
    }

    std::unique_ptr<Scene> SceneRegistry::Create(const String& name)
    {
        auto& data = GetRegistryData();
        std::lock_guard lock(data.mutex);

        if (const auto it = data.factories.find(name); it != data.factories.end())
        {
            if (it->second)
                return it->second();

            LOG_ERROR("Scene factory for '" + name + "' is invalid.");
            return nullptr;
        }

        LOG_ERROR("Scene not registered: " + name);
        return nullptr;
    }

    std::vector<String> SceneRegistry::GetAvailableScenes()
    {
        auto& data = GetRegistryData();
        std::lock_guard lock(data.mutex);

        std::vector<String> scenes;
        scenes.reserve(data.factories.size());

        for (const auto& name : data.factories | std::views::keys)
        {
            scenes.push_back(name);
        }

        return scenes;
    }
}