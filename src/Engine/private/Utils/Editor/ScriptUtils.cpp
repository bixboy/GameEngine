#include "Utils/Editor/ScriptUtils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ranges>

namespace BixEngine::ScriptUtils
{

    bool Utilities::IsActorType(std::string_view n)
    {
        return n == "Actor" || n == "BixEngine::Game::Actor" || n.find("::Actor") != std::string::npos;
    }
    
    bool Utilities::IsComponentType(std::string_view n)
    {
        return n == "Component" || n == "BixEngine::Game::Component" || n.find("::Component") != std::string::npos;
    }

    // --- Helper de Parsing ---

    bool Utilities::IsSubclassOf(const Reflection::ClassInfo& type, const Reflection::ClassInfo& base)
    {
        const Reflection::ClassInfo* current = &type;
        while (current)
        {
            if (current == &base)
                return true;
            current = current->SuperClass;
        }
        return false;
    }

    static void ParseFileContent(const std::filesystem::path& file, ScriptNode& outNode)
    {
        std::ifstream stream(file);
        if (!stream.is_open()) return;

        std::string line;
        bool classFound = false;

        while (std::getline(stream, line))
        {
            // 1. Nettoyage des commentaires
            if (auto pos = line.find("//"); pos != std::string::npos)
                line = line.substr(0, pos);
            
            // 2. Recherche de Macro
            if (line.find("BCLASS") != std::string::npos)
                outNode.hasBlueprintMacro = true;

            // 3. Recherche de la définition de classe
            if (!classFound && line.find("class ") != std::string::npos)
            {
                std::istringstream iss(line);
                std::string token;
                
                while (iss >> token)
                {
                    if (token == "class")
                    {
                        std::string nextToken;
                        iss >> nextToken;
                        
                        if (nextToken.find("_API") != std::string::npos)
                        {
                            iss >> outNode.name;
                        }
                        else
                        {
                            outNode.name = nextToken;
                        }
                        
                        if (auto pos = outNode.name.find(':'); pos != std::string::npos)
                        {
                            outNode.name = outNode.name.substr(0, pos);
                        }

                        classFound = true;
                    }
                    else if (classFound && token == ":")
                    {
                        std::string inheritanceMode;
                        iss >> inheritanceMode;
                        
                        if (inheritanceMode == "public" || inheritanceMode == "private" || inheritanceMode == "protected")
                        {
                            iss >> outNode.parentName;
                        }
                        else
                        {
                            outNode.parentName = inheritanceMode;
                        }
                        
                        break;
                    }
                }
            }
        }
    }

    std::vector<ScriptNode> Utilities::BuildScriptTree(const std::filesystem::path& scriptsDir, const std::filesystem::path& contentRoot)
    {
        namespace fs = std::filesystem;
        std::unordered_map<std::string, ScriptNode> map;

        // 1. Parsing de tous les fichiers
        for (auto& entry : fs::recursive_directory_iterator(scriptsDir))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".h")
                continue;

            ScriptNode node;
            node.headerPath = entry.path();
            
            ParseFileContent(entry.path(), node);

            if (node.name.empty())
                continue;

            std::error_code ec;
            node.includePath = fs::relative(entry.path(), scriptsDir, ec).generic_string();
            
            if (ec)
                node.includePath = entry.path().filename().generic_string();

            map[node.name] = std::move(node);
        }

        // 2. Résolution des Flags (Actor/Component) via propagation
        for (auto& [name, node] : map)
        {
            if (IsActorType(node.parentName))
                node.inheritsActor = true;
            
            if (IsComponentType(node.parentName))
                node.inheritsComponent = true;
        }
        
        std::function<void(ScriptNode&)> ResolveFlags = [&](ScriptNode& n)
        {
            if (n.inheritsActor || n.inheritsComponent)
                return;
            
            if (n.parentName.empty())
                return;

            if (auto it = map.find(n.parentName); it != map.end())
            {
                ResolveFlags(it->second);
                
                if (it->second.inheritsActor)
                    n.inheritsActor = true;
                
                if (it->second.inheritsComponent)
                    n.inheritsComponent = true;
            }
        };

        for (auto& [name, node] : map)
        {
            ResolveFlags(node);
        }

        // 3. Construction de l'arbre
        std::vector<ScriptNode> roots;
        for (auto& node : map | std::views::values)
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

        auto CaseInsensitiveLess = [](const std::string& a, const std::string& b)
        {
            return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                [](unsigned char x, unsigned char y)
                {
                    return std::tolower(x) < std::tolower(y);
                });
        };

        std::function<void(std::vector<ScriptNode>&)> SortNodes = [&](std::vector<ScriptNode>& list)
        {
            std::sort(list.begin(), list.end(), [&](const auto& a, const auto& b) { return CaseInsensitiveLess(a.name, b.name); });
            for (auto& child : list) SortNodes(child.children);
        };

        SortNodes(roots);
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
