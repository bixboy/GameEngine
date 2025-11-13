#include "Ressources/ResourceManager.h"


namespace BixEngine::resources
{
    void ResourceManager::Purge()
    {
        std::scoped_lock lock(mutex_);

        for (auto& [type, cache] : caches_)
        {
            for (auto it = cache.begin(); it != cache.end();)
            {
                if (it->second.expired())
                    it = cache.erase(it);
                else
                    ++it;
            }
        }
    }
}
