#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Utils/Editor/ScriptUtils.h"
#include "Debug/Logger.h"

namespace BixEngine::Game { class Actor; }

namespace BixEngine::PrefabUtils
{
    class Utilities
    {
    public:
        using path = std::filesystem::path;

        
        
        

        struct PrefabScriptCandidate
        {
            std::string displayName;
            std::string className;
            std::string includePath;
            path headerPath;

            bool isActor = false;
            bool isComponent = false;
            bool hasBlueprintMacro = false;

            std::string assetBaseName;
        };

        struct ExposedVariableMetadata
        {
            std::string name;
            std::string type;
            std::string defaultValue;
        };

        
        
        

        static const std::vector<BixEngine::ScriptUtils::ParentScriptInfo>& GetBaseClasses();

        
        
        

        static std::string SanitizeAssetName(std::string name);
        static bool ValidateMetadata(const std::string& className, const std::string& includePath, String& error);
        static std::string EscapeJson(const std::string& value);
        static std::vector<ExposedVariableMetadata> ExtractExposedVariables(const std::filesystem::path& headerPath, const std::string& includePath, const std::filesystem::path& scriptsDir);

        
        
        

        static std::vector<PrefabScriptCandidate> GatherPrefabCandidates(const std::vector<BixEngine::ScriptUtils::ScriptNode>& roots, const std::vector<BixEngine::ScriptUtils::ParentScriptInfo>& baseClasses);
    };

    class PrefabSerializer
    {
    public:
        static bool SavePrefab(const BixEngine::Game::Actor* rootActor, const std::filesystem::path& path);
        static std::unique_ptr<BixEngine::Game::Actor> LoadPrefab(const std::filesystem::path& path);
    };
}
