#include "Game/SceneSerializer.h"

#include <cstdint>
#include <filesystem>
#include <exception>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <utility>

#include "Core/Logger.h"
#include "Game/Actor.h"
#include "Game/Object.h"
#include "Game/Scene.h"

namespace BixEngine::Game
{
    namespace
    {
        constexpr std::uint32_t kSceneBinaryVersion = 1;

        class BinaryWriter
        {
        public:
            explicit BinaryWriter(std::ostream& stream) : stream_(stream) {}

            bool WriteUint32(std::uint32_t value)
            {
                stream_.write(reinterpret_cast<const char*>(&value), sizeof(value));
                return static_cast<bool>(stream_);
            }

            bool WriteString(const String& value)
            {
                const auto length = value.size();
                return WriteUint32(length) && WriteBytes(value.c_str(), value.size());
            }

            [[nodiscard]] bool Good() const noexcept { return static_cast<bool>(stream_); }

        private:
            bool WriteBytes(const char* data, std::size_t length)
            {
                if (length == 0)
                    return true;

                stream_.write(data, length);
                return static_cast<bool>(stream_);
            }

            std::ostream& stream_;
        };

        class BinaryReader
        {
        public:
            explicit BinaryReader(std::istream& stream) : stream_(stream) {}

            bool ReadUint32(std::uint32_t& value)
            {
                stream_.read(reinterpret_cast<char*>(&value), sizeof(value));
                return static_cast<bool>(stream_);
            }

            bool ReadString(String& value)
            {
                std::uint32_t length = 0;
                if (!ReadUint32(length))
                    return false;

                value.resize(length, '\0');
                if (length == 0)
                    return true;

                stream_.read(value.data(), length);
                return static_cast<bool>(stream_);
            }

            [[nodiscard]] bool Good() const noexcept { return static_cast<bool>(stream_); }

        private:
            std::istream& stream_;
        };

        bool LogStreamFailure(const std::filesystem::path& filePath, const String& action)
        {
            LOG_ERROR(String("Failed to ") + action + String(": ") + filePath.string());
            return false;
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

        BinaryWriter writer(file);

        if (!writer.WriteUint32(kSceneBinaryVersion))
            return LogStreamFailure(filePath, "write scene header");

        if (!writer.WriteString(scene.GetName()))
            return LogStreamFailure(filePath, "write scene name");

        const auto actorCount = static_cast<std::uint32_t>(scene.GetActors().size());
        if (!writer.WriteUint32(actorCount))
            return LogStreamFailure(filePath, "write actor count");

        for (const auto& actor : scene.GetActors())
        {
            if (!writer.WriteString(actor->GetTypeName()))
                return LogStreamFailure(filePath, "write actor type");

            actor->SerializeBinary(file);
            if (!file)
            {
                LOG_ERROR(String("Failed to serialize actor ") + actor->GetTypeName());
                return false;
            }
        }

        return writer.Good();
    }

    bool SceneSerializer::LoadBinary(Scene& scene, const std::filesystem::path& filePath)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open binary scene file for reading: " + filePath.string());
            return false;
        }

        BinaryReader reader(file);

        std::uint32_t version = 0;
        if (!reader.ReadUint32(version))
        {
            LOG_ERROR("Failed to read scene header: " + filePath.string());
            return false;
        }

        if (version != kSceneBinaryVersion)
        {
            LOG_ERROR("Unsupported scene binary version: " + std::to_string(version));
            return false;
        }

        String sceneName;
        if (!reader.ReadString(sceneName))
        {
            LOG_ERROR("Failed to read scene name from: " + filePath.string());
            return false;
        }
        scene.Rename(std::move(sceneName));

        scene.ClearActors();

        std::uint32_t actorCount = 0;
        if (!reader.ReadUint32(actorCount))
        {
            LOG_ERROR("Failed to read actor count from: " + filePath.string());
            return false;
        }

        for (std::uint32_t i = 0; i < actorCount; ++i)
        {
            String type;
            if (!reader.ReadString(type))
            {
                LOG_ERROR(String("Failed to read actor type at index ") + std::to_string(i));
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
                LOG_ERROR(String("Failed to deserialize actor ") + type + String(": ") + String(e.what()));
                return false;
            }

            if (!file)
            {
                LOG_ERROR(String("Stream error while deserializing actor ") + type);
                return false;
            }

            scene.AddActor(std::move(actor));
        }

        return reader.Good();
    }

    void SceneSerializer::RegisterActorFactory(String typeName, ActorFactory factory)
    {
        if (typeName.empty() || !factory)
            return;

        auto& factories = GetFactories();
        factories[std::move(typeName)] = std::move(factory);
    }

    void SceneSerializer::UnregisterActorFactory(const String& typeName)
    {
        auto& factories = GetFactories();
        factories.erase(typeName);
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

    bool SceneSerializer::HasActorFactory(const String& typeName)
    {
        const auto& factories = GetFactories();
        return factories.contains(typeName);
    }

    void SceneSerializer::ClearActorFactories()
    {
        auto& factories = GetFactories();
        factories.clear();
        EnsureDefaultFactories();
    }

    std::unique_ptr<Actor> SceneSerializer::CreateActor(const String& typeName)
    {
        auto& factories = GetFactories();
        const auto it = factories.find(typeName);

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
        if (!factories.contains(String("Actor")))
        {
            factories["Actor"] = []() { return std::make_unique<Actor>(); };
        }
    }
}
