#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Utils/Editor/ScriptUtils.h"
#include "Containers/String.h"

namespace BixEngine::Editor
{
    class ScriptIntrospector
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

        static const std::vector<ScriptUtils::ParentScriptInfo>& GetBaseClasses();

        static std::string SanitizeAssetName(std::string name);
        static bool ValidateMetadata(const std::string& className, const std::string& includePath, String& error);
        static std::string EscapeJson(const std::string& value);
        static std::vector<ExposedVariableMetadata> ExtractExposedVariables(const std::filesystem::path& headerPath, const std::string& includePath, const std::filesystem::path& scriptsDir);

        static std::vector<PrefabScriptCandidate> GatherPrefabCandidates(const std::vector<ScriptUtils::ScriptNode>& roots, const std::vector<ScriptUtils::ParentScriptInfo>& baseClasses);
    };
}
