#pragma once

#include <filesystem>
#include <functional>
#include <memory>

#include "Bix/Core/String.h"

namespace BixEngine::Game
{
    class Actor;
    class Scene;

    class SceneSerializer
    {
    public:
        using ActorFactory = std::function<std::unique_ptr<Actor>()>;

        [[deprecated("Use BixSaveSystem::SavePackage instead.")]]
        static bool SaveBinary(const Scene& scene, const std::filesystem::path& filePath);

        [[deprecated("Use BixSaveSystem::LoadPackage instead.")]]
        static bool LoadBinary(Scene& scene, const std::filesystem::path& filePath);

        [[deprecated("Actor factories are handled by BixSaveSystem class registry.")]]
        static void RegisterActorFactory(String typeName, ActorFactory factory);

        [[deprecated("Actor factories are handled by BixSaveSystem class registry.")]]
        static void UnregisterActorFactory(const String& typeName);

        [[deprecated("Actor factories are handled by BixSaveSystem class registry.")]]
        static void EnsureActorFactory(const Actor& actor);

        [[deprecated("Actor factories are handled by BixSaveSystem class registry.")]]
        [[nodiscard]] static bool HasActorFactory(const String& typeName);

        [[deprecated("Actor factories are handled by BixSaveSystem class registry.")]]
        static void ClearActorFactories();
    };
}
