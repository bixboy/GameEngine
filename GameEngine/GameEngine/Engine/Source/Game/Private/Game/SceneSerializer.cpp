#include "Game/SceneSerializer.h"

#include <cstdint>
#include <exception>
#include <fstream>
#include <memory>
#include <string_view>
#include <utility>
#include <stdexcept>

#include "Core/Logger.h"
#include "Game/Actor.h"
#include "Game/Object.h"
#include "Game/Scene.h"

namespace Engine::Game
{
    namespace
    {
        constexpr std::uint32_t kSceneBinaryVersion = 1;

        void WriteUint32(std::ostream& stream, std::uint32_t value)
        {
            stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }

        void ReadUint32(std::istream& stream, std::uint32_t& value)
        {
            stream.read(reinterpret_cast<char*>(&value), sizeof(value));
            if (!stream)
                throw std::runtime_error("Failed to read uint32_t from stream.");
        }

        void WriteString(std::ostream& stream, std::string_view value)
        {
            const auto length = static_cast<std::uint32_t>(value.size());
            WriteUint32(stream, length);
            stream.write(value.data(), static_cast<std::streamsize>(length));
        }

        String ReadString(std::istream& stream)
        {
            std::uint32_t length = 0;
            ReadUint32(stream, length);
            String result(length, '\0');
            stream.read(result.data(), static_cast<std::streamsize>(length));
            if (!stream)
                throw std::runtime_error("Failed to read string from stream.");
            return result;
        }
    }

    bool SceneSerializer::SaveBinary(const Scene& scene, const std::filesystem::path& filePath)
    {
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open binary scene file for writing: " + filePath.string());
            return false;
        }

        WriteUint32(file, kSceneBinaryVersion);
        WriteString(file, String(scene.Name()));

        const auto actorCount = static_cast<std::uint32_t>(scene.GetActors().size());
        WriteUint32(file, actorCount);

        for (const auto& actor : scene.GetActors())
        {
            WriteString(file, String(actor->GetTypeName()));
            actor->SerializeBinary(file);
        }

        return static_cast<bool>(file);
    }

    bool SceneSerializer::LoadBinary(Scene& scene, const std::filesystem::path& filePath)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open binary scene file for reading: " + filePath.string());
            return false;
        }

        std::uint32_t version = 0;
        try
        {
            ReadUint32(file, version);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(String("Failed to read scene header: ") + e.what());
            return false;
        }

        if (version != kSceneBinaryVersion)
        {
            LOG_ERROR("Unsupported scene binary version: " + std::to_string(version));
            return false;
        }

        try
        {
            scene.Rename(ReadString(file));
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(String("Failed to read scene name: ") + e.what());
            return false;
        }

        scene.ClearActors();

        std::uint32_t actorCount = 0;
        try
        {
            ReadUint32(file, actorCount);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(String("Failed to read actor count: ") + e.what());
            return false;
        }

        for (std::uint32_t i = 0; i < actorCount; ++i)
        {
            String type;
            try
            {
                type = ReadString(file);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR(String("Failed to read actor type: ") + e.what());
                return false;
            }
            auto actor = CreateActor(type);
            if (!actor)
            {
                LOG_ERROR("No actor factory registered for type: " + type);
                return false;
            }

            try
            {
                actor->DeserializeBinary(file);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR(String("Failed to deserialize actor ") + type + ": " + e.what());
                return false;
            }

            scene.AddActor(std::move(actor));
        }

        return true;
    }

    void SceneSerializer::RegisterActorFactory(String typeName, ActorFactory factory)
    {
        if (typeName.empty() || !factory)
            return;

        auto& factories = GetFactories();
        factories[std::move(typeName)] = std::move(factory);
    }

    void SceneSerializer::UnregisterActorFactory(std::string_view typeName)
    {
        auto& factories = GetFactories();
        factories.erase(String(typeName));
    }

    void SceneSerializer::EnsureActorFactory(const Actor& actor)
    {
        String typeName(actor.GetTypeName());
        if (typeName.empty() || HasActorFactory(typeName))
            return;

        std::unique_ptr<Actor> prototype = actor.ClonePrototype();
        if (!prototype)
        {
            LOG_ERROR("Failed to create prototype for actor type: " + typeName);
            return;
        }

        auto sharedPrototype = std::shared_ptr<Actor>(std::move(prototype));

        RegisterActorFactory(std::move(typeName), [sharedPrototype]() -> std::unique_ptr<Actor>
        {
            return sharedPrototype->ClonePrototype();
        });
    }

    bool SceneSerializer::HasActorFactory(std::string_view typeName)
    {
        const auto& factories = GetFactories();
        return factories.find(String(typeName)) != factories.end();
    }

    void SceneSerializer::ClearActorFactories()
    {
        auto& factories = GetFactories();
        factories.clear();
        EnsureDefaultFactories();
    }

    std::unique_ptr<Actor> SceneSerializer::CreateActor(std::string_view typeName)
    {
        auto& factories = GetFactories();
        const auto it = factories.find(String(typeName));
        if (it == factories.end())
            return nullptr;

        return it->second();
    }

    std::unordered_map<String, SceneSerializer::ActorFactory>& SceneSerializer::GetFactories()
    {
        static std::unordered_map<String, ActorFactory> factories;
        static bool defaultsRegistered = false;

        if (!defaultsRegistered)
        {
            factories["Actor"] = []() { return std::make_unique<Actor>(); };
            defaultsRegistered = true;
        }

        return factories;
    }

    void SceneSerializer::EnsureDefaultFactories()
    {
        auto& factories = GetFactories();
        if (!factories.contains("Actor"))
        {
            factories["Actor"] = []() { return std::make_unique<Actor>(); };
        }
    }
}
