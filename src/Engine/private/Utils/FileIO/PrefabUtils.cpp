#include "Utils/FileIO/PrefabUtils.h"
#include "Containers/String.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <functional>
#include "Utils/FileIO/FilesUtils.h"
#include "Serializer/SceneSerializer.h"
#include "Utils/FileIO/BinaryUtils.h"
#include "Framework/Actor.h"


namespace BixEngine::PrefabUtils
{
    
    
    
    const std::vector<ScriptUtils::ParentScriptInfo>& Utilities::GetBaseClasses()
    {
        static const std::vector<ScriptUtils::ParentScriptInfo> BaseClasses = {
            {
                .displayName = "Actor (Engine)",
                .className = "BixEngine::Game::Actor",
                .includePath = "Game/Actor.h",
                .headerPath = "src/Game/public/Actor.h",
                .isActor = true,
                .isComponent = false,
                .hasBlueprintMacro = true
            },
            {
                .displayName = "Component (Engine)",
                .className = "BixEngine::Game::Component",
                .includePath = "Game/Components/Component.h",
                .headerPath = "src/Game/public/Components/Component.h",
                .isActor = false,
                .isComponent = true,
                .hasBlueprintMacro = true
            },
            {
                .displayName = "Player (Engine)",
                .className = "BixEngine::Game::Player",
                .includePath = "Test/Player.h",
                .headerPath = "src/Game/public/Test/Player.h",
                .isActor = true,
                .isComponent = false,
                .hasBlueprintMacro = true
            },
            {
                .displayName = "Sprite Component (Engine)",
                .className = "BixEngine::Game::SpriteComponent",
                .includePath = "Components/SpriteComponent.h",
                .headerPath = "src/Game/public/Components/SpriteComponent.h",
                .isActor = false,
                .isComponent = true,
                .hasBlueprintMacro = true
            },
            {
                .displayName = "Sprite Animator Component (Engine)",
                .className = "BixEngine::Game::SpriteAnimatorComponent",
                .includePath = "Components/SpriteAnimatorComponent.h",
                .headerPath = "src/Game/public/Components/SpriteAnimatorComponent.h",
                .isActor = false,
                .isComponent = true,
                .hasBlueprintMacro = true
            }
        };
        return BaseClasses;
    }

    
    
    
    std::string Utilities::SanitizeAssetName(std::string name)
    {
        std::replace_if(name.begin(), name.end(),
            [](unsigned char ch)
            {
                return !(std::isalnum(ch) || ch == '_' || ch == '-');
            },
            '_');

        name.erase(name.begin(), std::find_if(name.begin(), name.end(),
            [](char c)
            {
                return c != '_';
            }));

        while (!name.empty() && name.back() == '_')
            name.pop_back();

        return name.empty() ? "Prefab" : name;
    }

    bool Utilities::ValidateMetadata(const std::string& className, const std::string& includePath, String& error)
    {
        auto hasIllegal = [](const std::string& s) { return s.find_first_of("\r\n\"") != std::string::npos; };

        if (hasIllegal(className))
        {
            FilesUtils::Utilities::LogAndStoreError(error, "Invalid characters in script class name.", false);
            return false;
        }

        if (hasIllegal(includePath))
        {
            FilesUtils::Utilities::LogAndStoreError(error, "Invalid characters in include path.", false);
            return false;
        }

        return true;
    }

    std::string Utilities::EscapeJson(const std::string& value)
    {
        std::ostringstream escaped;
        for (char ch : value)
        {
            switch (ch)
            {
            case '\\': escaped << "\\\\";
                break;
                
            case '"':  escaped << "\\\"";
                break;
                
            case '\n': escaped << "\\n";
                
                break;
            case '\r': escaped << "\\r";
                break;
                
            case '\t': escaped << "\\t";
                break;
                
            default:
                if (static_cast<unsigned char>(ch) < 0x20)
                    escaped << "\\u" << std::hex << static_cast<int>(ch);
                else
                    escaped << ch;
                break;
            }
        }
        return escaped.str();
    }

    
    
    
    namespace
    {
        std::string Trim(std::string_view input)
        {
            const auto first = input.find_first_not_of(" \t");
            const auto last = input.find_last_not_of(" \t");
            
            return first == std::string_view::npos ? "" : std::string{input.substr(first, last - first + 1)};
        }

        std::filesystem::path ResolveHeaderPath(const std::filesystem::path& headerPath, const std::string& includePath, const std::filesystem::path& scriptsDir)
        {
            auto tryResolve = [&](const std::filesystem::path& relativePath) -> std::filesystem::path
            {
                if (relativePath.empty())
                    return {};
                
                if (relativePath.is_absolute() && std::filesystem::exists(relativePath))
                    return relativePath;

                for (auto search = scriptsDir; !search.empty(); search = search.parent_path())
                {
                    const auto candidate = search / relativePath;
                    if (std::filesystem::exists(candidate))
                        return candidate;
                    
                    if (search == search.parent_path()) 
                        break;
                }
                
                return {};
            };

            auto resolvedHeader = tryResolve(headerPath);
            if (!resolvedHeader.empty())
                return resolvedHeader;
            
            return tryResolve(includePath);
        }
    }

    std::vector<Utilities::ExposedVariableMetadata> Utilities::ExtractExposedVariables(const std::filesystem::path& headerPath, const std::string& includePath, const std::filesystem::path& scriptsDir)
    {
        std::vector<ExposedVariableMetadata> variables;

        auto resolved = ResolveHeaderPath(headerPath, includePath, scriptsDir);
        if (resolved.empty())
            return variables;

        std::ifstream stream(resolved);
        if (!stream.is_open())
            return variables;

        std::string line;
        bool pendingProperty = false;
        while (std::getline(stream, line))
        {
            if (line.find("BPROPERTY") != std::string::npos)
            {
                pendingProperty = true; continue;
            }
            
            if (!pendingProperty)
                continue;

            pendingProperty = false;
            std::string trimmed = Trim(line);
            
            if (trimmed.empty())
                continue;
            
            if (trimmed.back() == ';')
                trimmed.pop_back();

            size_t eqPos = trimmed.find('=');
            std::string declaration = eqPos == std::string::npos ? trimmed : trimmed.substr(0, eqPos);
            std::string defaultPart = eqPos == std::string::npos ? "" : Trim(trimmed.substr(eqPos + 1));

            size_t bracePos = declaration.find('{');
            
            if (defaultPart.empty() && bracePos != std::string::npos)
                defaultPart = Trim(declaration.substr(bracePos));
            
            if (bracePos != std::string::npos)
                declaration = declaration.substr(0, bracePos);

            declaration = Trim(declaration);
            size_t spacePos = declaration.find_last_of(" \t");
            
            if (spacePos == std::string::npos || spacePos + 1 >= declaration.size())
                continue;

            ExposedVariableMetadata meta;
            meta.type = Trim(declaration.substr(0, spacePos));
            meta.name = Trim(declaration.substr(spacePos + 1));
            meta.defaultValue = std::move(defaultPart);

            variables.push_back(std::move(meta));
        }

        return variables;
    }

    
    
    
    std::vector<Utilities::PrefabScriptCandidate> Utilities::GatherPrefabCandidates(
        const std::vector<ScriptUtils::ScriptNode>& roots,
        const std::vector<ScriptUtils::ParentScriptInfo>& baseClasses)
    {
        std::vector<PrefabScriptCandidate> candidates;
        candidates.reserve(roots.size() + baseClasses.size());

        
        for (const auto& base : baseClasses)
        {
            candidates.push_back({
                base.displayName,
                base.className,
                base.includePath,
                base.headerPath,
                base.isActor,
                base.isComponent,
                base.hasBlueprintMacro,
                base.isActor ? "ActorPrefab" : "ComponentPrefab"
            });
        }

        
        std::function<void(const ScriptUtils::ScriptNode&)> collect = [&](const ScriptUtils::ScriptNode& node)
        {
            if (node.inheritsActor || node.inheritsComponent || node.hasBlueprintMacro)
            {
                PrefabScriptCandidate candidate;
                candidate.displayName = node.name;
                candidate.className = node.name;
                candidate.includePath = node.includePath;
                candidate.headerPath = node.headerPath;
                candidate.isActor = node.inheritsActor || !node.inheritsComponent;
                candidate.isComponent = node.inheritsComponent;
                candidate.hasBlueprintMacro = node.hasBlueprintMacro;
                candidate.assetBaseName = SanitizeAssetName(node.headerPath.empty() ? node.name : node.headerPath.stem().string());

                candidates.push_back(std::move(candidate));
            }

            for (const auto& child : node.children) collect(child);
        };

        for (const auto& root : roots) collect(root);

        
        std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b)
        {
            std::string al = a.displayName;
            std::string bl = b.displayName;
            
            std::transform(
                al.begin(),
                al.end(),
                al.begin(), tolower
                );
            
            std::transform(
                bl.begin(),
                bl.end(),
                bl.begin(), tolower
                );
            
            return al < bl;
        });

        return candidates;
    }

    
    
    

    
    static void CollectDescendants(const BixEngine::Game::Actor* actor, std::vector<const BixEngine::Game::Actor*>& outList)
    {
        if (!actor) return;
        for (auto* child : actor->GetChildren())
        {
            outList.push_back(child);
            CollectDescendants(child, outList);
        }
    }

    bool PrefabSerializer::SavePrefab(const BixEngine::Game::Actor* rootActor, const std::filesystem::path& path)
    {
        if (!rootActor) return false;

        std::ofstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open file for writing prefab: " + path.string());
            return false;
        }

        BixEngine::Utils::BinaryWriter writer(file);
        
        
        writer.WriteString("PREFAB_V2");
        
        
        writer.WriteString(rootActor->GetTypeName());
        rootActor->SerializeBinary(file);

        
        std::vector<const BixEngine::Game::Actor*> descendants;
        CollectDescendants(rootActor, descendants);

        writer.WriteUint32(static_cast<uint32_t>(descendants.size()));
        
        for (const auto* child : descendants)
        {
            writer.WriteString(child->GetTypeName());
            child->SerializeBinary(file);
        }

        return true;
    }

    std::unique_ptr<BixEngine::Game::Actor> PrefabSerializer::LoadPrefab(const std::filesystem::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
             LOG_ERROR("Failed to open file for reading prefab: " + path.string());
             return nullptr;
        }

        BixEngine::Utils::BinaryReader reader(file);
        
        
        String header;
        long long startPos = file.tellg();
        
        bool isV2 = false;
        if (reader.ReadString(header))
        {
            if (header == "PREFAB_V2")
            {
                isV2 = true;
            }
            else
            {
                isV2 = false;
            }
        }
        else
        {
             return nullptr;
        }

        std::unique_ptr<BixEngine::Game::Actor> root = nullptr;

        if (isV2)
        {
            
            String typeName;
            if (!reader.ReadString(typeName)) return nullptr;
            
            root = BixEngine::Serialization::SceneSerializer::CreateActor(typeName);
            if (!root) root = std::make_unique<BixEngine::Game::Actor>("Root(Fallback)");
            
            root->DeserializeBinary(file);

            
            uint32_t count = 0;
            if (reader.ReadUint32(count))
            {
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                std::vector<BixEngine::Game::Actor*> loadedRawActors;
                loadedRawActors.reserve(count);

                for (uint32_t i = 0; i < count; ++i)
                {
                    String childType;
                    reader.ReadString(childType);
                    auto child = BixEngine::Serialization::SceneSerializer::CreateActor(childType);
                    if (!child) child = std::make_unique<BixEngine::Game::Actor>("Child(Fallback)");
                    
                    child->DeserializeBinary(file);
                    
                    
                    loadedRawActors.push_back(child.release());
                }

                
                auto FindActorByUUID = [&](const String& uuid) -> BixEngine::Game::Actor*
                {
                    if (root->GetUUID() == uuid) return root.get();
                    for (auto* a : loadedRawActors)
                        if (a->GetUUID() == uuid) return a;
                    return nullptr;
                };

                for (auto* child : loadedRawActors)
                {
                    String pUUID = child->GetLoadedParentUUID();
                    if (!pUUID.IsEmpty())
                    {
                        if (auto* parent = FindActorByUUID(pUUID))
                        {
                            child->SetParent(parent);
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
        else
        {
            file.clear();
            file.seekg(startPos, std::ios::beg);
            
             String typeName;
             if (!reader.ReadString(typeName)) return nullptr;
             
             auto actor = BixEngine::Serialization::SceneSerializer::CreateActor(typeName);
             if (!actor) return nullptr;
             actor->DeserializeBinary(file);
             return actor;
        }
    }
}
