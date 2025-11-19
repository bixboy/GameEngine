#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "ScriptUtils.h"
#include "Logger.h"

namespace BixEngine::PrefabUtils
{
    class Utilities
    {
    public:
        using path = std::filesystem::path;

        // ─────────────────────────────────────────────
        // Data Structures
        // ─────────────────────────────────────────────

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

        // ─────────────────────────────────────────────
        // Engine base class list
        // ─────────────────────────────────────────────

        static const std::vector<BixEngine::ScriptUtils::ParentScriptInfo>& GetBaseClasses();

        // ─────────────────────────────────────────────
        // String & metadata utilities
        // ─────────────────────────────────────────────

        static std::string SanitizeAssetName(std::string name);
        static bool ValidateMetadata(const std::string& className, const std::string& includePath, String& error);
        static std::string EscapeJson(const std::string& value);
        static std::vector<ExposedVariableMetadata> ExtractExposedVariables(const std::filesystem::path& headerPath, const std::string& includePath, const std::filesystem::path& scriptsDir);

        // ─────────────────────────────────────────────
        // Prefab candidate construction
        // ─────────────────────────────────────────────

        static std::vector<PrefabScriptCandidate> GatherPrefabCandidates(const std::vector<BixEngine::ScriptUtils::ScriptNode>& roots, const std::vector<BixEngine::ScriptUtils::ParentScriptInfo>& baseClasses);
    };
}
