#include "Utils/PrefabUtils.h"
#include <functional>
#include <filesystem>
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
                path{"src/Game/public/Actor.h"},
                true,
                false,
                true
            },
            {
                "Component (Engine)",
                "BixEngine::Game::Component",
                "Game/Components/Component.h",
                path{"src/Game/public/Components/Component.h"},
                false,
                true,
                true
            },
            {
                "Player (Engine)",
                "BixEngine::Game::Player",
                "Test/Player.h",
                path{"src/Game/public/Test/Player.h"},
                true,
                false,
                true
            },
            {
                "Sprite Component (Engine)",
                "BixEngine::Game::SpriteComponent",
                "Components/SpriteComponent.h",
                path{"src/Game/public/Components/SpriteComponent.h"},
                false,
                true,
                true
            },
            {
                "Sprite Animator Component (Engine)",
                "BixEngine::Game::SpriteAnimatorComponent",
                "Components/SpriteAnimatorComponent.h",
                path{"src/Game/public/Components/SpriteAnimatorComponent.h"},
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

    static std::string Trim(std::string_view input)
    {
        const auto first = input.find_first_not_of(" \t");
        const auto last = input.find_last_not_of(" \t");

        if (first == std::string_view::npos)
            return {};

        return std::string{input.substr(first, last - first + 1)};
    }

    static std::filesystem::path ResolveHeaderPath(const std::filesystem::path& headerPath,
                                                   const std::string& includePath,
                                                   const std::filesystem::path& scriptsDir)
    {
        if (!headerPath.empty() && std::filesystem::exists(headerPath))
            return headerPath;

        if (!includePath.empty())
        {
            const auto candidate = scriptsDir / includePath;
            if (std::filesystem::exists(candidate))
                return candidate;
        }

        return {};
    }

    std::vector<ExposedVariableMetadata> ExtractExposedVariables(const std::filesystem::path& headerPath,
                                                                 const std::string& includePath,
                                                                 const std::filesystem::path& scriptsDir)
    {
        std::vector<ExposedVariableMetadata> variables;

        const auto resolved = ResolveHeaderPath(headerPath, includePath, scriptsDir);
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
                pendingProperty = true;
                continue;
            }

            if (!pendingProperty)
                continue;

            pendingProperty = false;
            std::string trimmed = Trim(line);
            if (trimmed.empty())
                continue;

            if (trimmed.back() == ';')
                trimmed.pop_back();

            const auto eqPos = trimmed.find('=');
            std::string declaration = eqPos == std::string::npos ? trimmed : trimmed.substr(0, eqPos);
            std::string defaultPart = eqPos == std::string::npos ? std::string{} : Trim(trimmed.substr(eqPos + 1));

            const auto bracePos = declaration.find('{');
            if (defaultPart.empty() && bracePos != std::string::npos)
                defaultPart = Trim(declaration.substr(bracePos));

            if (bracePos != std::string::npos)
                declaration = declaration.substr(0, bracePos);

            declaration = Trim(declaration);
            const auto spacePos = declaration.find_last_of(" \t");
            if (spacePos == std::string::npos || spacePos + 1 >= declaration.size())
                continue;

            ExposedVariableMetadata meta{};
            meta.type = Trim(declaration.substr(0, spacePos));
            meta.name = Trim(declaration.substr(spacePos + 1));
            meta.defaultValue = std::move(defaultPart);

            variables.push_back(std::move(meta));
        }

        return variables;
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
