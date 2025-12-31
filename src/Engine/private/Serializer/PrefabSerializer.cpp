#include "Serializer/PrefabSerializer.h"
#include "Containers/String.h"
#include <fstream>
#include <vector>
#include "Serializer/SceneSerializer.h"
#include "Utils/FileIO/BinaryUtils.h"
#include "Debug/Logger.h"


namespace BixEngine::Serialization
{
    static void CollectDescendants(const Game::Actor* actor, std::vector<const Game::Actor*>& outList)
    {
        if (!actor)
            return;
        
        for (auto* child : actor->GetChildren())
        {
            outList.push_back(child);
            CollectDescendants(child, outList);
        }
    }

    bool PrefabSerializer::SavePrefab(const Game::Actor* rootActor, const std::filesystem::path& path)
    {
        if (!rootActor)
            return false;

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open file for writing prefab: " + path.string());
            return false;
        }

        Utils::BinaryWriter writer(file);
        
        writer.WriteString("PREFAB_V2");
        
        // 1. Sauvegarde de la racine
        writer.WriteString(rootActor->GetTypeName());
        rootActor->SerializeBinary(file);

        // 2. Collecte et sauvegarde de tous les descendants
        std::vector<const Game::Actor*> descendants;
        CollectDescendants(rootActor, descendants);

        writer.WriteUint32(static_cast<uint32_t>(descendants.size()));
        
        for (const auto* child : descendants)
        {
            writer.WriteString(child->GetTypeName());
            child->SerializeBinary(file);
        }

        return true;
    }

    std::unique_ptr<Game::Actor> PrefabSerializer::LoadPrefab(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
             LOG_ERROR("Failed to open file for reading prefab: " + path.string());
             return nullptr;
        }

        Utils::BinaryReader reader(file);
        
        String header;
        if (!reader.ReadString(header) || header != "PREFAB_V2")
        {
             LOG_ERROR("Invalid prefab format or version mismatch: " + path.string());
             return nullptr;
        }

        std::unique_ptr<Game::Actor> root = nullptr;

        // 1. Chargement Racine
        String typeName;
        if (!reader.ReadString(typeName))
            return nullptr;
        
        root = SceneSerializer::CreateActor(typeName);
        if (!root)
            root = std::make_unique<Game::Actor>("Root(Fallback)");
        
        root->DeserializeBinary(file);

        // 2. Chargement Enfants
        uint32_t count = 0;
        if (reader.ReadUint32(count))
        {
            std::vector<Game::Actor*> loadedRawActors;
            loadedRawActors.reserve(count);

            for (uint32_t i = 0; i < count; ++i)
            {
                String childType;
                reader.ReadString(childType);
                
                auto child = SceneSerializer::CreateActor(childType);
                
                if (!child)
                    child = std::make_unique<Game::Actor>("Child(Fallback)");
                
                child->DeserializeBinary(file);
                
                loadedRawActors.push_back(child.release());
            }

            // 3. Reconstruction Hiérarchie
            auto FindActorByUUID = [&](const String& uuid) -> Game::Actor*
            {
                if (root->GetUUID() == uuid)
                    return root.get();
                
                for (auto* a : loadedRawActors)
                {
                    if (a->GetUUID() == uuid)
                        return a;   
                }
                
                return nullptr;
            };

            for (auto* child : loadedRawActors)
            {
                String pUUID = child->GetLoadedParentUUID();
                if (!pUUID.empty())
                {
                    if (auto* parent = FindActorByUUID(pUUID))
                    {
                        child->SetParent(parent);
                    }
                    else
                    {
                        child->SetParent(root.get());
                    }
                }
                else
                {
                    child->SetParent(root.get());
                }
            }
        }
        return root;
    }
}