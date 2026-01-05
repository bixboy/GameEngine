#pragma once
#include <memory>
#include <filesystem>
#include "Framework/Actor.h"


namespace BixEngine::Serialization
{
    class PrefabSerializer
    {
    public:
        static bool SavePrefab(const Game::Actor* rootActor, const std::filesystem::path& path);
        static std::unique_ptr<Game::Actor> LoadPrefab(const std::filesystem::path& path);

        static bool SaveComponent(Game::Component* component, const std::filesystem::path& path);
        static std::unique_ptr<Game::Component> LoadComponent(const std::filesystem::path& path);
    };
}
