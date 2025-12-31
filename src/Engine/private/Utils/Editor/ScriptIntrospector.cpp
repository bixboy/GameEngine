#include "Utils/Editor/ScriptIntrospector.h"
#include "Containers/String.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <functional>
#include "Utils/FileIO/FilesUtils.h"

namespace BixEngine::Editor
{
    const std::vector<ScriptUtils::ParentScriptInfo>& ScriptIntrospector::GetBaseClasses()
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

    std::string ScriptIntrospector::SanitizeAssetName(std::string name)
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

    bool ScriptIntrospector::ValidateMetadata(const std::string& className, const std::string& includePath, String& error)
    {
        auto hasIllegal = [](const std::string& s)
        {
            return s.find_first_of("\r\n\"") != std::string::npos;
        };

        if (hasIllegal(className))
        {
            Utils::FileUtils::LogAndStoreError(error, "Invalid characters in script class name.", false);
            return false;
        }

        if (hasIllegal(includePath))
        {
            Utils::FileUtils::LogAndStoreError(error, "Invalid characters in include path.", false);
            return false;
        }

        return true;
    }

    std::string ScriptIntrospector::EscapeJson(const std::string& value)
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

    std::vector<ScriptIntrospector::ExposedVariableMetadata> ScriptIntrospector::ExtractExposedVariables(const std::filesystem::path& headerPath, const std::string& includePath, const std::filesystem::path& scriptsDir)
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
            std::string trimmed = Trim(line);
            if (trimmed.empty())
                continue;

            if (trimmed.rfind("//", 0) == 0)
                continue; 

            if (trimmed.find("BPROPERTY") != std::string::npos)
            {
                pendingProperty = true;
                
                if (trimmed.length() <= 15)
                    continue; 
                
                size_t macroEnd = trimmed.find(')');
                if (macroEnd != std::string::npos && macroEnd + 1 < trimmed.size())
                {
                    trimmed = Trim(trimmed.substr(macroEnd + 1));
                }
            }

            if (!pendingProperty) continue;

            // --- Parsing de la variable ---

            if (trimmed.back() == ';')
                trimmed.pop_back();

            size_t eqPos = trimmed.find('=');
            std::string declaration = (eqPos == std::string::npos) ? trimmed : trimmed.substr(0, eqPos);
            std::string defaultPart = (eqPos == std::string::npos) ? "" : Trim(trimmed.substr(eqPos + 1));

            size_t bracePos = declaration.find('{');
            if (bracePos != std::string::npos)
            {
                if (defaultPart.empty())
                    defaultPart = Trim(declaration.substr(bracePos));
                
                declaration = declaration.substr(0, bracePos);
            }

            declaration = Trim(declaration);
            size_t spacePos = declaration.find_last_of(" \t");

            if (spacePos != std::string::npos && spacePos + 1 < declaration.size())
            {
                ExposedVariableMetadata meta;
                meta.type = Trim(declaration.substr(0, spacePos));
                meta.name = Trim(declaration.substr(spacePos + 1));
                meta.defaultValue = std::move(defaultPart);

                variables.push_back(std::move(meta));
                pendingProperty = false;
            }
        }

        return variables;
    }

    std::vector<ScriptIntrospector::PrefabScriptCandidate> ScriptIntrospector::GatherPrefabCandidates(
        const std::vector<ScriptUtils::ScriptNode>& roots,
        const std::vector<ScriptUtils::ParentScriptInfo>& baseClasses)
    {
        std::vector<PrefabScriptCandidate> candidates;
        candidates.reserve(roots.size() + baseClasses.size());

        // Add base classes
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
            
            std::transform(al.begin(), al.end(), al.begin(), tolower);
            std::transform(bl.begin(), bl.end(), bl.begin(), tolower);
            
            return al < bl;
        });

        return candidates;
    }
}
