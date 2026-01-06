#pragma once
#include <memory>
#include <type_traits>
#include <utility>
#include "SceneManager.h"
#include "Debug/Logger.h"
#include "Containers/String.h"
#include "Containers/SubclassOf.h"


namespace BixEngine::Game
{
    class Actor;
    class Scene;
    class SceneManager;
}


namespace BixEngine::Game
{
    class BGameplayStatics
    {
    public:
        static void OpenScene(void* worldContext, const String& sceneName);
        static void LoadScene(void* worldContext, const String& sceneName);
        static void UnloadScene(void* worldContext, const String& sceneName);
        static void PreloadScene(void* worldContext, const String& sceneName);
        static void ReloadScene();

        [[nodiscard]] static Scene* GetActiveScene() noexcept;
        [[nodiscard]] static SceneManager* ResolveSceneManager(void* worldContext);

        template <typename T, typename... Args>
        static T* SpawnActor(void* worldContext, Args&&... args);

        template <typename T>
        static T* SpawnActor(void* worldContext, const TSubclassOf<T>& subclass);

    private:
        static Actor* SpawnPrefabInternal(Scene* scene, const String& path);
    };

    // --- Template Implementations ---

    template <typename T, typename... Args>
    T* BGameplayStatics::SpawnActor(void* worldContext, Args&&... args)
    {
        SceneManager* manager = ResolveSceneManager(worldContext);
        if (!manager)
        {
            LOG_ERROR("Cannot spawn actor: no valid SceneManager.");
            return nullptr;
        }

        Scene* scene = manager->GetActiveScene();
        if (!scene)
        {
            LOG_ERROR("Cannot spawn actor: no active scene.");
            return nullptr;
        }

        auto actor = std::make_unique<T>(std::forward<Args>(args)...);
        T* actorPtr = actor.get();
        
        scene->AddActor(std::move(actor));

        LOG_INFO("Spawned actor C++ in scene: " + scene->GetName());
        return actorPtr;
    }

    template <typename T>
    T* BGameplayStatics::SpawnActor(void* worldContext, const TSubclassOf<T>& subclass)
    {
        SceneManager* manager = ResolveSceneManager(worldContext);
        if (!manager)
            return nullptr;

        Scene* scene = manager->GetActiveScene();
        if (!scene)
            return nullptr;

        if (!subclass.IsValid())
        {
            LOG_WARNING("SpawnActor: Invalid prefab path.");
            return nullptr;
        }

        Actor* spawned = SpawnPrefabInternal(scene, subclass.GetAssetPath());
        if (!spawned)
            return nullptr;

        T* casted = dynamic_cast<T*>(spawned);
        if (!casted)
        {
             scene->RemoveActor(spawned);
             return nullptr;
        }

        return casted;
    }
}