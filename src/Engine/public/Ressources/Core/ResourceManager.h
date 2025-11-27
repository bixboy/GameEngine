#pragma once
#include <functional>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include <mutex>
#include "Ressources/Core/IResource.h"
#include "Containers/String.h"


namespace BixEngine::resources
{
    class ResourceManager
    {
    public:
        static ResourceManager& Get();

        template <typename T>
        std::shared_ptr<T> Get(const String& path);

        template <typename T>
        void RegisterLoader(std::function<std::shared_ptr<T>(const String&)> loader);

        template <typename T>
        void UnregisterLoader();

        template <typename T>
        std::vector<String> GetLoadedResourceKeys();

        template <typename T>
        void SetDefault(std::shared_ptr<T> defaultResource);

        void Purge();

    private:
        ResourceManager() = default;
        ~ResourceManager() = default;

        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        template <typename T>
        std::shared_ptr<T> LoadResource(const String& path);

        template <typename T>
        std::shared_ptr<T> GetDefault();

        std::unordered_map<std::type_index, std::unordered_map<String, std::shared_ptr<IResource>>> caches_;
        std::unordered_map<std::type_index, std::shared_ptr<IResource>> defaults_;
        std::unordered_map<std::type_index, std::function<std::shared_ptr<IResource>(const String&)>> loaders_;

        std::mutex mutex_;
    };
}

#include "ResourceManager.inl"
