#include "BGameplayStatics.h"
#include "Logger.h"
#include "Actor.h"
#include "Scene.h"
#include "SceneManager.h"


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
            LOG_ERROR("No active SceneManager available to resolve world context.");
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
        {
            LOG_ERROR("Cannot open a scene with an empty name.");
            return;
        }

        SceneManager* manager = ResolveSceneManager(worldContext);
        if (!manager)
            return;

        manager->ChangeSceneByName(sceneName);
    }

    void BGameplayStatics::LoadScene(void* worldContext, const String& sceneName)
    {
        if (sceneName.empty())
        {
            LOG_ERROR("Cannot load a scene with an empty name.");
            return;
        }

        SceneManager* manager = ResolveSceneManager(worldContext);
        if (!manager)
            return;

        manager->LoadScene(sceneName);
    }

    void BGameplayStatics::PreloadScene(void* worldContext, const String& sceneName)
    {
        LoadScene(worldContext, sceneName);
    }

    void BGameplayStatics::UnloadScene(void* worldContext, const String& sceneName)
    {
        if (sceneName.empty())
        {
            LOG_ERROR("Cannot unload a scene with an empty name.");
            return;
        }

        SceneManager* manager = ResolveSceneManager(worldContext);
        if (!manager)
            return;

        manager->UnloadScene(sceneName);
    }

    Scene* BGameplayStatics::GetActiveScene() noexcept
    {
        return SceneManager::GetActiveScene();
    }
}
