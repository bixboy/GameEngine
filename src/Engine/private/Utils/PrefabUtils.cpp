#include "Utils/PrefabUtils.h"
#include <functional>
#include <algorithm>
#include <cctype>
#include <sstream>
#include "Utils/FilesUtils.h"


namespace BixEngine::PrefabUtils
{
    // ============================================================================
    //  Engine base class list
    // ============================================================================

    const std::vector<ParentScriptInfo>& GetBaseClasses()
    {
        static const std::vector<ParentScriptInfo> BaseClasses = {
            {
                "Actor (Engine)",
                "BixEngine::Game::Actor",
                "Game/Actor.h",
                path{},
                true,
                false,
                true
            },
            {
                "Component (Engine)",
                "BixEngine::Game::Component",
                "Game/Components/Component.h",
                path{},
                false,
                true,
                true
            },
            {
                "Player (Engine)",
                "BixEngine::Game::Player",
                "Test/Player.h",
                path{},
                true,
                false,
                true
            },
            {
                "Sprite Component (Engine)",
                "BixEngine::Game::SpriteComponent",
                "Components/SpriteComponent.h",
                path{},
                false,
                true,
                true
            },
            {
                "Sprite Animator Component (Engine)",
                "BixEngine::Game::SpriteAnimatorComponent",
                "Components/SpriteAnimatorComponent.h",
                path{},
                false,
                true,
                true
            }
        };

        return BaseClasses;
    }

    // ============================================================================
    //  String & metadata utilities
    // ============================================================================

    std::string SanitizeAssetName(std::string name)
    {
        std::replace_if(name.begin(), name.end(),
                        [](unsigned char ch) { return !(std::isalnum(ch) || ch == '_' || ch == '-'); },
                        '_');

        name.erase(name.begin(),
                   std::find_if(name.begin(), name.end(), [](char c) { return c != '_'; }));

        while (!name.empty() && name.back() == '_')
            name.pop_back();

        return name.empty() ? "Prefab" : name;
    }

    bool ValidateMetadata(const std::string& className, const std::string& includePath, String& error)
    {
        auto hasIllegal = [](const std::string& s)
        {
            return s.find_first_of("\r\n\"") != std::string::npos;
        };

        if (hasIllegal(className))
        {
            FileUtils::LogAndStoreError(error, "Invalid characters in script class name.", false);
            return false;
        }

        if (hasIllegal(includePath))
        {
            FileUtils::LogAndStoreError(error, "Invalid characters in include path.", false);
            return false;
        }

        return true;
    }

    std::string EscapeJson(const std::string& value)
    {
        std::ostringstream escaped;

        for (char ch : value)
        {
            switch (ch)
            {
            case '\\': escaped << "\\\\";
                break;
            case '"': escaped << "\\\"";
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

    // ============================================================================
    //  Prefab candidate construction
    // ============================================================================

    std::vector<PrefabScriptCandidate> GatherPrefabCandidates(const std::vector<ScriptNode>& roots,
                                                              const std::vector<ParentScriptInfo>& baseClasses)
    {
        std::vector<PrefabScriptCandidate> candidates;
        candidates.reserve(roots.size() + baseClasses.size());

        // --- Add engine base classes
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

        // --- Recursive traversal of user scripts
        std::function<void(const ScriptNode&)> collect = [&](const ScriptNode& node)
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
                candidate.assetBaseName = SanitizeAssetName
                (
                    node.headerPath.empty() ? node.name : node.headerPath.stem().string()
                );

                candidates.push_back(std::move(candidate));
            }

            for (const auto& child : node.children)
                collect(child);
        };

        for (const auto& root : roots)
            collect(root);

        // --- Sort alphabetically
        std::sort(candidates.begin(), candidates.end(),
                  [](const auto& a, const auto& b)
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
