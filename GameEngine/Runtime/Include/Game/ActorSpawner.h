#pragma once
#include <memory>
#include <type_traits>
#include <utility>
#include "Bix/Game/Actor.h"
#include "Bix/Game/SceneManager.h"

namespace BixEngine::Game
{
    class ActorSpawner
    {
        public:
            template<typename T, typename... Args>
            static T* SpawnActor(Args&&... args)
            {
                static_assert(std::is_base_of_v<Actor, T>, "T must derive from Actor");

                Scene* activeScene = SceneManager::GetActiveScene();
                if (!activeScene) return nullptr;

                auto actor = std::make_unique<T>(std::forward<Args>(args)...);
                T* actorPtr = actor.get();

                activeScene->AddActor(std::move(actor));

                return actorPtr;
            }
    };
}
