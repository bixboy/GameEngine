#pragma once
#include <memory>
#include <type_traits>
#include <utility>
#include "Debug/Logger.h"
#include "Containers/String.h"
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include "Framework/SceneManager.h"


namespace BixEngine::Game
{
    class BGameplayStatics
    {
    public:
        // ────────────────────────────────────────────────
        // 🌍 Gestion des scènes
        // ────────────────────────────────────────────────

        static void OpenScene(void* worldContext, const String& sceneName);
        static void LoadScene(void* worldContext, const String& sceneName);
        static void UnloadScene(void* worldContext, const String& sceneName);
        static void PreloadScene(void* worldContext, const String& sceneName);

        [[nodiscard]] static Scene* GetActiveScene() noexcept;
        [[nodiscard]] static SceneManager* ResolveSceneManager(void* worldContext);

        // ────────────────────────────────────────────────
        // 🧱 Gestion des acteurs
        // ────────────────────────────────────────────────

        template <typename T, typename... Args>
        static T* SpawnActor(void* worldContext, Args&&... args);
    };

    template <typename T, typename... Args>
    T* BGameplayStatics::SpawnActor(void* worldContext, Args&&... args)
    {
        static_assert(std::is_base_of_v<Actor, T>, "T must derive from Actor");

        SceneManager* manager = ResolveSceneManager(worldContext);
        if (!manager)
        {
            LOG_ERROR("❌ Cannot spawn actor: no valid SceneManager for world context.");
            return nullptr;
        }

        Scene* scene = manager->GetActiveScene();
        if (!scene)
        {
            LOG_ERROR("❌ Cannot spawn actor: no active scene.");
            return nullptr;
        }

        auto actor = std::make_unique<T>(std::forward<Args>(args)...);
        T* actorPtr = actor.get();
        scene->AddActor(std::move(actor));

        LOG_INFO("🧩 Spawned actor of type: " + String(typeid(T).name()) + " in scene: " + scene->GetName());
        return actorPtr;
    }
}
