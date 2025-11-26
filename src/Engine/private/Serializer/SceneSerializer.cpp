#include "Serializer/SceneSerializer.h"
#include "Actor.h"
#include "Scene.h"
#include "Utils/BinaryUtils.h"
#include "Reflection/public/Registry.h"
#include "Reflection/public/ClassInfo.h"
#include "Logger.h"
#include <fstream>

#include "Player.h"


namespace BixEngine::Serialization
{
    using namespace BixEngine::Utils;

    namespace
    {
        constexpr std::uint32_t kSceneBinaryVersion = 1;
    }

    // ==============================================================================
    // SAVE / LOAD
    // ==============================================================================

    bool SceneSerializer::SaveBinary(const Game::Scene& scene, const std::filesystem::path& filePath)
    {
        std::ofstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open binary scene file for writing: " + filePath.string());
            return false;
        }
        return SerializeBinary(scene, file);
    }

    bool SceneSerializer::LoadBinary(Game::Scene& scene, const std::filesystem::path& filePath)
    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open binary scene file for reading: " + filePath.string());
            return false;
        }
        return DeserializeBinary(scene, file);
    }

    // ==============================================================================
    // SERIALIZE
    // ==============================================================================

    bool SceneSerializer::SerializeBinary(const Game::Scene& scene, std::ostream& stream)
    {
        BinaryWriter writer(stream);

        // 1. Header
        if (!writer.WriteUint32(kSceneBinaryVersion))
            return false;
        
        if (!writer.WriteString(scene.GetName()))
            return false;

        // 2. Actors Count
        const auto& actors = scene.GetActors();
        if (!writer.WriteUint32(static_cast<uint32_t>(actors.size())))
            return false;

        // 3. Actors Data
        for (const auto& actor : actors)
        {
            if (!writer.WriteString(actor->GetTypeName()))
                return false;

            actor->SerializeBinary(stream);
            
            if (!stream)
            {
                LOG_ERROR("Stream error while serializing actor: " + actor->GetTypeName());
                return false;
            }
        }

        return writer.Good();
    }

    // ==============================================================================
    // DESERIALIZE
    // ==============================================================================

    bool SceneSerializer::DeserializeBinary(Game::Scene& scene, std::istream& stream)
    {
        BinaryReader reader(stream);

        std::uint32_t version = 0;
        if (!reader.ReadUint32(version))
            return false;

        if (version != kSceneBinaryVersion)
        {
            LOG_ERROR("Unsupported scene binary version: " + std::to_string(version));
            return false;
        }

        String sceneName;
        if (!reader.ReadString(sceneName))
            return false;
        
        scene.Rename(std::move(sceneName));
        scene.ClearActors();

        std::uint32_t actorCount = 0;
        if (!reader.ReadUint32(actorCount))
            return false;

        for (std::uint32_t i = 0; i < actorCount; ++i)
        {
            String typeName;
            if (!reader.ReadString(typeName))
                return false;

            auto actor = CreateActor(typeName);
            if (!actor)
            {
                LOG_ERROR("Failed to create actor of type: " + typeName + " (No Factory/Reflection found)");
                return false;
            }

            // Chargement des données de l'acteur
            try
            {
                actor->DeserializeBinary(stream);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Exception deserializing actor " + typeName + ": " + String(e.what()));
                return false;
            }

            if (!stream)
            {
                LOG_ERROR("Stream corrupted after deserializing actor: " + typeName);
                return false;
            }

            scene.AddActor(std::move(actor));
        }

        return reader.Good();
    }

    // ==============================================================================
    // FACTORY SYSTEM
    // ==============================================================================

    std::unique_ptr<Game::Actor> SceneSerializer::CreateActor(const String& typeName)
    {
        auto& factories = GetFactories();
        const auto it = factories.find(typeName);
        
        if (it != factories.end())
        {
            return it->second();
        }

        auto* classInfo = Bix::Reflection::Registry::Get().Find(typeName.c_str());
        if (!classInfo)
            classInfo = Bix::Reflection::Registry::Get().FindByQualifiedName(typeName.c_str());

        if (classInfo && classInfo->CanConstruct())
        {
            if (auto* instance = classInfo->ConstructTyped<Game::Actor>())
            {
                return std::unique_ptr<Game::Actor>(instance);
            }
        }

        return nullptr;
    }

    void SceneSerializer::RegisterActorFactory(String typeName, ActorFactory factory)
    {
        if (!typeName.empty() && factory)
        {
            GetFactories()[std::move(typeName)] = std::move(factory);
        }
    }

    void SceneSerializer::UnregisterActorFactory(const String& typeName)
    {
        GetFactories().erase(typeName);
    }

    void SceneSerializer::EnsureActorFactory(const Game::Actor& actor)
    {
        String typeName = actor.GetTypeName();
        if (typeName.empty() || HasActorFactory(typeName)) return;

        std::unique_ptr<Game::Actor> prototype = actor.ClonePrototype();
        if (!prototype)
        {
            LOG_ERROR("Failed to create prototype for: " + typeName);
            return;
        }

        std::shared_ptr sharedProto = std::move(prototype);
        RegisterActorFactory(typeName, [sharedProto] { return sharedProto->ClonePrototype(); });
    }

    bool SceneSerializer::HasActorFactory(const String& typeName)
    {
        return GetFactories().contains(typeName);
    }

    void SceneSerializer::ClearActorFactories()
    {
        GetFactories().clear();
        EnsureDefaultFactories();
    }

    std::unordered_map<String, SceneSerializer::ActorFactory>& SceneSerializer::GetFactories()
    {
        static std::unordered_map<String, ActorFactory> factories;
        static bool defaultsRegistered = false;

        if (!defaultsRegistered)
        {
            defaultsRegistered = true;
            EnsureDefaultFactories();
        }

        return factories;
    }

    void SceneSerializer::EnsureDefaultFactories()
    {
        auto& f = GetFactories();
        
        auto RegisterIfMissing = [&](const char* name, auto creator)
        {
            if (!f.contains(name))
                f[name] = creator;
        };

        RegisterIfMissing("Actor", [] { return std::make_unique<Game::Actor>(); });
        RegisterIfMissing("BixEngine::Game::Actor", [] { return std::make_unique<Game::Actor>(); });
        
        RegisterIfMissing("BixEngine::Game::Player",
        []
            { 
                return std::make_unique<Game::Player>(Math::Transform()); 
            });
    }
}
