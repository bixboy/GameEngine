#pragma once
#include <memory>
#include "Game/Actor.h"
#include "Game/SceneManager.h"

namespace Engine::Game
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