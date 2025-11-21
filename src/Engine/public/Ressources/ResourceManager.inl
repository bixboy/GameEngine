#pragma once
#include "Logger.h"

namespace BixEngine::resources
{
    inline ResourceManager& ResourceManager::Get()
    {
        static ResourceManager instance;
        return instance;
    }

    // ────────────────────────────────────────────────
    // 📦 Get Resource
    // ────────────────────────────────────────────────
    template <typename T>
    std::shared_ptr<T> ResourceManager::Get(const String& path)
    {
        static_assert(std::is_base_of_v<IResource, T>, "T must derive from IResource");
        const std::type_index typeIndex(typeid(T));
        {
            std::scoped_lock lock(mutex_);
            auto cacheIt = caches_.find(typeIndex);
            if (cacheIt != caches_.end())
            {
                auto& cache = cacheIt->second;
                if (auto it = cache.find(path); it != cache.end())
                {
                    return std::static_pointer_cast<T>(it->second);
                }
            }
        }

        std::shared_ptr<T> resource = LoadResource<T>(path);

        if (!resource)
        {
            LOG_WARNING("⚠️ Failed to load resource: " + path + " — using fallback.");
            resource = GetDefault<T>();
        }

        {
            std::scoped_lock lock(mutex_);
            caches_[typeIndex][path] = resource;
        }

        return resource;
    }

    // ────────────────────────────────────────────────
    // 🧱 Register / Unregister Loaders
    // ────────────────────────────────────────────────
    template <typename T>
    void ResourceManager::RegisterLoader(std::function<std::shared_ptr<T>(const String&)> loader)
    {
        static_assert(std::is_base_of_v<IResource, T>, "T must derive from IResource");
        std::scoped_lock lock(mutex_);

        loaders_[std::type_index(typeid(T))] =
            [loader](const String& path) -> std::shared_ptr<IResource>
            {
                return std::static_pointer_cast<IResource>(loader(path));
            };
    }

    template <typename T>
    void ResourceManager::UnregisterLoader()
    {
        std::scoped_lock lock(mutex_);
        loaders_.erase(std::type_index(typeid(T)));
    }

    // ────────────────────────────────────────────────
    // ⚙️ Load Resource
    // ────────────────────────────────────────────────
    template <typename T>
    std::shared_ptr<T> ResourceManager::LoadResource(const String& path)
    {
        std::function<std::shared_ptr<IResource>(const String&)> loader;
        {
            std::scoped_lock lock(mutex_);
            const auto it = loaders_.find(std::type_index(typeid(T)));
            if (it == loaders_.end())
            {
                LOG_ERROR("❌ No loader registered for resource type: " + std::string(typeid(T).name()));
                return nullptr;
            }

            loader = it->second;
        }

        if (!loader)
            return nullptr;

        auto resource = loader(path);
        return std::static_pointer_cast<T>(resource);
    }


    // ────────────────────────────────────────────────
    // 🔍 Get Loaded Resource Keys
    // ────────────────────────────────────────────────
    template <typename T>
    std::vector<String> ResourceManager::GetLoadedResourceKeys()
    {
        std::vector<String> keys;
        const std::type_index typeIndex(typeid(T));
    
        std::scoped_lock lock(mutex_);
    
        auto cacheIt = caches_.find(typeIndex);
        if (cacheIt != caches_.end())
        {
            for (const auto& [path, resource] : cacheIt->second)
            {
                if (resource)
                {
                    keys.push_back(path);
                }
            }
        }
        return keys;
    }

    // ────────────────────────────────────────────────
    // ⚠️ Default resource handling
    // ────────────────────────────────────────────────
    template <typename T>
    void ResourceManager::SetDefault(std::shared_ptr<T> defaultResource)
    {
        static_assert(std::is_base_of_v<IResource, T>);
        std::scoped_lock lock(mutex_);
        defaults_[std::type_index(typeid(T))] = defaultResource;
    }

    template <typename T>
    std::shared_ptr<T> ResourceManager::GetDefault()
    {
        std::scoped_lock lock(mutex_);
        const auto it = defaults_.find(std::type_index(typeid(T)));

        if (it != defaults_.end())
            return std::static_pointer_cast<T>(it->second);

        return nullptr;
    }
}
