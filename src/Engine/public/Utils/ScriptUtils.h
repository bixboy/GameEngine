#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include "ClassInfo.h"


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

    class Utilities
    {
    public:
        // Type checks
        static bool IsActorType(std::string_view name);
        static bool IsComponentType(std::string_view name);

        // Reflection helpers
        static bool AreEquivalent(const Bix::Reflection::ClassInfo& lhs, const Bix::Reflection::ClassInfo& rhs);
        static bool IsSubclassOf(const Bix::Reflection::ClassInfo& type, const Bix::Reflection::ClassInfo& base);

        // Parsing / tree helpers
        static std::vector<ScriptNode> BuildScriptTree(const std::filesystem::path& scriptsDir, const std::filesystem::path& contentRoot);
        static std::vector<TreeNodeData> BuildGuiTree(const std::vector<ScriptNode>& nodes, std::unordered_map<std::string, ParentScriptInfo>& outInfo);
    };
}
