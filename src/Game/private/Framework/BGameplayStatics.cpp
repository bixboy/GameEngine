#include "Framework/BGameplayStatics.h"
#include "Debug/Logger.h"
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include "Framework/SceneManager.h"
#include "Serializer/PrefabSerializer.h"
#include <filesystem>
#include <vector>


namespace
{
    BixEngine::Game::SceneManager* ResolveFromSceneActors(BixEngine::Game::SceneManager& manager, void* worldContext)
    {
        if (!worldContext)
            return &manager;

        BixEngine::Game::Scene* activeScene = manager.GetActiveScene();
        if (!activeScene)
            return nullptr;

        if (worldContext == static_cast<void*>(activeScene))
            return &manager;
        
        const auto& actors = activeScene->GetActors();
        for (const auto& actor : actors)
        {
            if (!actor)
                continue;

            if (actor.get() == worldContext)
                return &manager;

            const auto& components = actor->GetComponents();
            for (const auto& component : components)
            {
                if (component.get() == worldContext)
                    return &manager;
            }
        }
        
        return nullptr;
    }
}

namespace BixEngine::Game
{
    SceneManager* BGameplayStatics::ResolveSceneManager(void* worldContext)
    {
        SceneManager* manager = SceneManager::GetActiveSceneManager();
        if (!manager)
        {
            LOG_ERROR("No active SceneManager available.");
            return nullptr;
        }

        if (!worldContext)
            return manager;

        if (SceneManager* resolved = ResolveFromSceneActors(*manager, worldContext))
            return resolved;

        LOG_ERROR("Unable to resolve SceneManager from provided world context.");
        return nullptr;
    }

    void BGameplayStatics::OpenScene(void* worldContext, const String& sceneName)
    {
        if (sceneName.empty())
            return;
        
        if (auto* manager = ResolveSceneManager(worldContext))
            manager->ChangeSceneByName(sceneName);
    }

    void BGameplayStatics::LoadScene(void* worldContext, const String& sceneName)
    {
        if (sceneName.empty())
            return;
        
        if (auto* manager = ResolveSceneManager(worldContext))
            manager->LoadScene(sceneName);
    }

    void BGameplayStatics::PreloadScene(void* worldContext, const String& sceneName)
    {
        LoadScene(worldContext, sceneName);
    }

    void BGameplayStatics::UnloadScene(void* worldContext, const String& sceneName)
    {
        if (sceneName.empty())
            return;
        
        if (auto* manager = ResolveSceneManager(worldContext))
            manager->UnloadScene(sceneName);
    }

    void BGameplayStatics::ReloadScene()
    {
        if (auto* manager = SceneManager::GetActiveSceneManager())
        {
            if (auto* active = manager->GetActiveScene())
            {
                 manager->ChangeSceneByName(active->GetName());
            }
        }
    }

    Scene* BGameplayStatics::GetActiveScene() noexcept
    {
        return SceneManager::GetActiveScene();
    }

    Actor* BGameplayStatics::SpawnPrefabInternal(Scene* scene, const String& path)
    {
        if (!scene || path.IsEmpty())
            return nullptr;

        std::filesystem::path prefabPath = path.Std();
        
        auto actorUnique = Serialization::PrefabSerializer::LoadPrefab(prefabPath);
    
        if (!actorUnique)
        {
            LOG_ERROR("SpawnPrefab: Failed to load prefab path: " + path);
            return nullptr;
        }

        Actor* rootPtr = actorUnique.get();
        scene->AddActor(std::move(actorUnique));

        return rootPtr;
    }
}