#pragma once
#include "Containers/String.h"
#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>

namespace BixEngine::Game
{
    class Actor;
    class Scene;
}


namespace BixEngine::Serialization
{
    class SceneSerializer
    {
    public:
        using ActorFactory = std::function<std::unique_ptr<Game::Actor>()>;

        static bool SaveBinary(const Game::Scene& scene, const std::filesystem::path& filePath);
        static bool LoadBinary(Game::Scene& scene, const std::filesystem::path& filePath);

        static bool SerializeBinary(const Game::Scene& scene, std::ostream& stream);
        static bool DeserializeBinary(Game::Scene& scene, std::istream& stream);

        static void RegisterActorFactory(String typeName, ActorFactory factory);
        static void UnregisterActorFactory(const String& typeName);
        
        static void EnsureActorFactory(const Game::Actor& actor);
        
        [[nodiscard]] static bool HasActorFactory(const String& typeName);
        static void ClearActorFactories();
        
        static std::unique_ptr<Game::Actor> CreateActor(const String& typeName);

    private:
        static std::unordered_map<String, ActorFactory>& GetFactories();
        static void EnsureDefaultFactories();
    };
}