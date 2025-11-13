#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>
#include "Containers/String.h"


namespace BixEngine::Game
{
    class Actor;
    class Scene;

    class SceneSerializer
    {
    public:
        using ActorFactory = std::function<std::unique_ptr<Actor>()>;

        static bool SaveBinary(const Scene& scene, const std::filesystem::path& filePath);
        static bool LoadBinary(Scene& scene, const std::filesystem::path& filePath);

        static void RegisterActorFactory(String typeName, ActorFactory factory);
        static void UnregisterActorFactory(const String& typeName);
        static void EnsureActorFactory(const Actor& actor);
        [[nodiscard]] static bool HasActorFactory(const String& typeName);
        static void ClearActorFactories();

    private:
        static std::unique_ptr<Actor> CreateActor(const String& typeName);
        static std::unordered_map<String, ActorFactory>& GetFactories();
        static void EnsureDefaultFactories();
    };
}
