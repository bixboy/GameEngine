#include "Engine/Utils/ScriptUtils.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

namespace BixEngine::ScriptUtils
{
    using namespace std::string_literals;
    namespace fs = std::filesystem;

    bool IsActorType(std::string_view n)
    {
        return n == "Actor" || n == "BixEngine::Game::Actor" || n == "::BixEngine::Game::Actor";
    }
    bool IsComponentType(std::string_view n)
    {
        return n == "Component" || n == "BixEngine::Game::Component" || n == "::BixEngine::Game::Component";
    }

    static bool CaseInsensitiveLess(const std::string& a, const std::string& b)
    {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
            [](unsigned char x, unsigned char y)
            {
                return std::tolower(x) < std::tolower(y);
            });
    }

    static void ParseHeader(const fs::path& file, ScriptNode& out)
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
                    iss >> out.name; foundClass = true;
                }
                else if (foundClass && token == ":")
                {
                    iss >> token >> out.parentName; break;
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
    }

    std::vector<ScriptNode> BuildScriptTree(const fs::path& scriptsDir, const fs::path& contentRoot)
    {
        std::unordered_map<std::string, ScriptNode> map;
        for (auto& entry : fs::recursive_directory_iterator(scriptsDir))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".h")
                continue;

            ScriptNode node;
            node.headerPath = entry.path();
            ParseHeader(entry.path(), node);

            fs::path rel;
            std::error_code ec;
            rel = fs::relative(entry.path(), scriptsDir, ec);
            if (!ec)
            {
                node.includePath = rel.generic_string();
            }
            else
            {
                node.includePath = entry.path().filename().generic_string();   
            }

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

        std::sort(roots.begin(), roots.end(), [](auto& a, auto& b)
        {
            return CaseInsensitiveLess(a.name, b.name);
        });
        
        return roots;
    }

    std::vector<TreeNodeData> BuildGuiTree(const std::vector<ScriptNode>& nodes, std::unordered_map<std::string, ParentScriptInfo>& outInfo)
    {
        std::vector<TreeNodeData> guiNodes;
        for (const auto& n : nodes)
        {
            ParentScriptInfo info
            {
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
