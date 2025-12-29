#include "Ressources/Core/ResourceManager.h"


namespace BixEngine::resources
{
    void ResourceManager::Purge()
    {
        std::scoped_lock lock(mutex_);

        for (auto& [type, cache] : caches_)
        {
            for (auto it = cache.begin(); it != cache.end();)
            {
                
                if (it->second.use_count() == 1)
                    it = cache.erase(it);
                else
                    ++it;
            }
        }
    }
}
