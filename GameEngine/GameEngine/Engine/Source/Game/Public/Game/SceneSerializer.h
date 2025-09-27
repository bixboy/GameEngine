#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "Core/String.h"

namespace Engine::Game
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
        static void UnregisterActorFactory(std::string_view typeName);
        static void EnsureActorFactory(const Actor& actor);
        [[nodiscard]] static bool HasActorFactory(std::string_view typeName);
        static void ClearActorFactories();

    private:
        static std::unique_ptr<Actor> CreateActor(std::string_view typeName);
        static std::unordered_map<String, ActorFactory>& GetFactories();
        static void EnsureDefaultFactories();
    };
}
