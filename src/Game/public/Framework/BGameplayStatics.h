#pragma once
#include <memory>
#include <type_traits>
#include <utility>
#include "Debug/Logger.h"
#include "Containers/String.h"
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include "Framework/SceneManager.h"
#include "Templates/SubclassOf.h"


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

        template <typename T>
        static T* SpawnActor(void* worldContext, const TSubclassOf<T>& subclass);

    private:
        static Actor* SpawnPrefabInternal(Scene* scene, const String& path);
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
        return actorPtr;
    }

    template <typename T>
    T* BGameplayStatics::SpawnActor(void* worldContext, const TSubclassOf<T>& subclass)
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

        if (!subclass.IsValid())
        {
            LOG_WARNING("SpawnActor: Invalid prefab path.");
             return nullptr;
        }

        Actor* spawned = SpawnPrefabInternal(scene, subclass.GetAssetPath());
        if (!spawned) return nullptr;

        T* casted = dynamic_cast<T*>(spawned);
        if (!casted)
        {
             LOG_ERROR("SpawnActor: Spawned prefab is not of type " + String(typeid(T).name()));
             // Should we destroy it? Yes.
             scene->RemoveActor(spawned);
             return nullptr;
        }

        LOG_INFO("🧩 Spawned prefab of type: " + String(typeid(T).name()) + " in scene: " + scene->GetName());
        return casted;
    }
}
