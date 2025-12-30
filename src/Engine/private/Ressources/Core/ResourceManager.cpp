#include "Ressources/Core/ResourceManager.h"

namespace BixEngine::Resources
{
    void ResourceManager::Purge()
    {
        std::scoped_lock lock(mutex_);

        for (auto& cache : caches_ | std::views::values)
        {
            for (auto it = cache.begin(); it != cache.end();)
            {
                if (it->second.use_count() == 1)
                {
                    it = cache.erase(it);
                }
                else
                    ++it;
            }
        }
    }
}