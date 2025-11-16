#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>


namespace BixEngine::ScriptUtils
{
    struct ScriptNode
    {
        std::string name;
        std::string parentName;
        std::string includePath;

        std::filesystem::path headerPath;

        bool inheritsActor = false;
        bool inheritsComponent = false;
        bool hasBlueprintMacro = false;

        std::vector<ScriptNode> children;
    };

    struct ParentScriptInfo
    {
        std::string displayName;
        std::string className;
        std::string includePath;

        std::filesystem::path headerPath;

        bool isActor = false;
        bool isComponent = false;
        bool hasBlueprintMacro = false;

        bool IsValid() const noexcept { return !className.empty(); }
    };

    struct TreeNodeData
    {
        std::string name;
        std::vector<TreeNodeData> children;
        bool isLeaf = false;
    };

    // Parsing helpers
    std::vector<ScriptNode> BuildScriptTree(const std::filesystem::path& scriptsDir, const std::filesystem::path& contentRoot);

    // UI helpers
    std::vector<TreeNodeData> BuildGuiTree(const std::vector<ScriptNode>& nodes, std::unordered_map<std::string, ParentScriptInfo>& outInfo);

    // String utils
    bool IsActorType(std::string_view name);
    bool IsComponentType(std::string_view name);
}
