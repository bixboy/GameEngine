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
                // If the resource is only held by the cache (use_count == 1), we can remove it.
                if (it->second.use_count() == 1)
                    it = cache.erase(it);
                else
                    ++it;
            }
        }
    }
}
