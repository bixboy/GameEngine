#include "Utils/ScriptUtils.h"
#include <fstream>
#include <sstream>
#include <algorithm>


namespace BixEngine::ScriptUtils
{
    // -----------------------
    // Type checks
    // -----------------------
    bool Utilities::IsActorType(std::string_view n)
    {
        return n == "Actor" || n == "BixEngine::Game::Actor" || n == "::BixEngine::Game::Actor";
    }

    bool Utilities::IsComponentType(std::string_view n)
    {
        return n == "Component" || n == "BixEngine::Game::Component" || n == "::BixEngine::Game::Component";
    }

    // -----------------------
    // Reflection helpers
    // -----------------------
    bool Utilities::AreEquivalent(const Bix::Reflection::ClassInfo& lhs, const Bix::Reflection::ClassInfo& rhs)
    {
        if (&lhs == &rhs)
            return true;
        
        if (!lhs.QualifiedName.empty() && !rhs.QualifiedName.empty())
            return lhs.QualifiedName == rhs.QualifiedName;
        
        if (!lhs.Name.empty() && !rhs.Name.empty())
            return lhs.Name == rhs.Name;
        
        return false;
    }

    bool Utilities::IsSubclassOf(const Bix::Reflection::ClassInfo& type, const Bix::Reflection::ClassInfo& base)
    {
        const Bix::Reflection::ClassInfo* current = &type;
        while (current)
        {
            if (AreEquivalent(*current, base))
                return true;
            
            current = current->SuperClass;
        }
        
        return false;
    }

    // -----------------------
    // Parsing / tree helpers
    // -----------------------
    std::vector<ScriptNode> Utilities::BuildScriptTree(const std::filesystem::path& scriptsDir, const std::filesystem::path& contentRoot)
    {
        namespace fs = std::filesystem;
        std::unordered_map<std::string, ScriptNode> map;
        auto CaseInsensitiveLess = [](const std::string& a, const std::string& b)
        {
            return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                [](unsigned char x, unsigned char y)
                {
                    return std::tolower(x) < std::tolower(y);
                });
        };

        auto ParseHeader = [](const fs::path& file, ScriptNode& out)
        {
            std::ifstream stream(file);
            if (!stream.is_open()) return;

            std::string line;
            while (std::getline(stream, line))
            {
                if (auto pos = line.find("//"); pos != std::string::npos)
                    line = line.substr(0, pos);

                if (line.find("class ") == std::string::npos)
                    continue;

                std::istringstream iss(line);
                std::string token;
                
                bool foundClass = false;
                while (iss >> token)
                {
                    if (token == "class")
                    {
                        iss >> out.name;
                        foundClass = true;
                    }
                    else if (foundClass && token == ":")
                    {
                        iss >> token >> out.parentName;
                        break;
                    }
                }
                
                break;
            }

            std::ifstream macroStream(file);
            std::string content;
            while (std::getline(macroStream, content))
            {
                if (content.find("BCLASS") != std::string::npos)
                {
                    out.hasBlueprintMacro = true;
                    break;
                }
            }
        };

        for (auto& entry : fs::recursive_directory_iterator(scriptsDir))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".h")
                continue;

            ScriptNode node;
            node.headerPath = entry.path();
            ParseHeader(entry.path(), node);

            std::error_code ec;
            node.includePath = fs::relative(entry.path(), scriptsDir, ec).generic_string();
            
            if (ec)
                node.includePath = entry.path().filename().generic_string();

            map[node.name] = std::move(node);
        }

        std::vector<ScriptNode> roots;
        for (auto& [name, node] : map)
        {
            if (auto it = map.find(node.parentName); it != map.end())
            {
                it->second.children.push_back(node);
            }
            else
            {
                roots.push_back(node);
            }
        }

        std::sort(roots.begin(), roots.end(), [&](const auto& a, const auto& b)
        {
            return CaseInsensitiveLess(a.name, b.name);
        });
        
        return roots;
    }

    std::vector<TreeNodeData> Utilities::BuildGuiTree(const std::vector<ScriptNode>& nodes, std::unordered_map<std::string, ParentScriptInfo>& outInfo)
    {
        std::vector<TreeNodeData> guiNodes;
        for (const auto& n : nodes)
        {
            ParentScriptInfo info {
                n.name,
                n.name,
                n.includePath,
                n.headerPath,
                n.inheritsActor,
                n.inheritsComponent,
                n.hasBlueprintMacro
            };

            outInfo[info.className] = info;

            TreeNodeData tree;
            tree.name = n.name;
            tree.children = BuildGuiTree(n.children, outInfo);
            tree.isLeaf = tree.children.empty();

            guiNodes.push_back(std::move(tree));
        }
        return guiNodes;
    }
}
