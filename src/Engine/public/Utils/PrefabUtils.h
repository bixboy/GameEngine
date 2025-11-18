#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "ScriptUtils.h"
#include "Logger.h"


namespace BixEngine::PrefabUtils
{
    namespace fs = std::filesystem;
    using path = fs::path;
    using ScriptUtils::ScriptNode;
    using ScriptUtils::ParentScriptInfo;

    // ============================================================================
    //  Data Structures
    // ============================================================================

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

    // ============================================================================
    //  Function Declarations
    // ============================================================================

    const std::vector<ParentScriptInfo>& GetBaseClasses();

    struct ExposedVariableMetadata
    {
        std::string name;
        std::string type;
        std::string defaultValue;
    };

    std::string SanitizeAssetName(std::string name);

    bool ValidateMetadata(const std::string& className, const std::string& includePath, String& error);

    std::string EscapeJson(const std::string& value);

    std::vector<ExposedVariableMetadata> ExtractExposedVariables(const std::filesystem::path& headerPath, const std::string& includePath, const std::filesystem::path& scriptsDir);

    std::vector<PrefabScriptCandidate> GatherPrefabCandidates(const std::vector<ScriptNode>& roots, const std::vector<ParentScriptInfo>& baseClasses);
}
