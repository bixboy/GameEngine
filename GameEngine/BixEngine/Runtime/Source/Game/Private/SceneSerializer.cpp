#include "Bix/Game/SceneSerializer.h"

#include <utility>

#include "Bix/Core/Logger.h"
#include "Bix/Engine/SaveSystem/BixReflection.h"
#include "Bix/Engine/SaveSystem/BixSaveSystem.h"
#include "Bix/Game/Actor.h"
#include "Bix/Game/Scene.h"

namespace BixEngine::Game
{
    bool SceneSerializer::SaveBinary(const Scene& scene, const std::filesystem::path& filePath)
    {
        return Engine::SaveSystem::BixSaveSystem::SavePackage(scene, filePath);
    }

    bool SceneSerializer::LoadBinary(Scene& scene, const std::filesystem::path& filePath)
    {
        auto loaded = Engine::SaveSystem::BixSaveSystem::LoadPackage(filePath);
        if (!loaded)
        {
            LOG_ERROR("SceneSerializer: Failed to load scene from file " + filePath.string());
            return false;
        }

        auto* loadedScene = dynamic_cast<Scene*>(loaded.get());
        if (!loadedScene)
        {
            LOG_ERROR("SceneSerializer: Loaded asset is not a Scene.");
            return false;
        }

        scene.Rename(loadedScene->Name());
        scene.ClearActors();

        auto& destinationActors = scene.GetActors();
        auto& sourceActors = loadedScene->GetActors();
        for (auto& actor : sourceActors)
        {
            if (actor)
                actor->SetOuter(&scene);
            destinationActors.push_back(std::move(actor));
        }
        sourceActors.clear();

        return true;
    }

    void SceneSerializer::RegisterActorFactory(String typeName, ActorFactory factory)
    {
        (void)typeName;
        (void)factory;
        LOG_WARN("SceneSerializer::RegisterActorFactory is deprecated. Classes are registered automatically by BixSaveSystem.");
    }

    void SceneSerializer::UnregisterActorFactory(const String& typeName)
    {
        (void)typeName;
        LOG_WARN("SceneSerializer::UnregisterActorFactory is deprecated. Classes are registered automatically by BixSaveSystem.");
    }

    void SceneSerializer::EnsureActorFactory(const Actor& actor)
    {
        (void)actor;
        // No-op in the new save system. Class registration happens automatically.
    }

    bool SceneSerializer::HasActorFactory(const String& typeName)
    {
        const auto* cls = Engine::SaveSystem::ClassRegistry::Get().FindClass(typeName.Std());
        return cls != nullptr;
    }

    void SceneSerializer::ClearActorFactories()
    {
        // No longer necessary; included for backward compatibility.
    }
}

