#include "Framework/BGameplayStatics.h"
#include "Debug/Logger.h"
#include "Framework/Actor.h"
#include "Framework/Scene.h"
#include "Framework/SceneManager.h"
#include "Utils/FileIO/PrefabUtils.h"
#include <filesystem>
#include <vector>
#include <functional>


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

    Actor* BGameplayStatics::SpawnPrefabInternal(Scene* scene, const String& path)
    {
        if (!scene || path.IsEmpty()) return nullptr;

         std::filesystem::path prefabPath = path.ToStdString();
                
        // Use PrefabSerializer to load entire hierarchy
        auto actorUnique = BixEngine::PrefabUtils::PrefabSerializer::LoadPrefab(prefabPath);
        
        if (!actorUnique)
        {
            LOG_ERROR("SpawnPrefab: Failed to load prefab path: " + path);
            return nullptr;
        }

        // Correctly register hierarchy into Scene
        // 1. Snapshot Hierarchy
        struct HierarchyNode { Actor* actor; Actor* parent; };
        std::vector<HierarchyNode> hierarchy;
        std::vector<Actor*> descendants;

        std::function<void(Actor*)> collect = [&](Actor* node)
        {
            for(auto* child : node->GetChildren())
            {
                hierarchy.push_back({child, node});
                descendants.push_back(child);
                collect(child);
            }
        };
        collect(actorUnique.get());

        Actor* rootPtr = actorUnique.get();

        // 2. Add Root
        scene->AddActor(std::move(actorUnique));

        // 3. Add Descendants (Must re-wrap in unique_ptr because LoadPrefab released them)
        for(auto* child : descendants)
        {
            scene->AddActor(std::unique_ptr<Actor>(child));
        }

        // 4. Restore Hierarchy (Scene::AddActor clears parents)
        for(const auto& node : hierarchy)
        {
            if (node.actor && node.parent)
                node.actor->SetParent(node.parent);
        }

        return rootPtr;
    }
}
