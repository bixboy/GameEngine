#include "Core/FileUtils.h"
#include "Core/Logger.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserPanelInternal.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserPopups.h"
#include "Engine/Ressources/SpriteAtlasFactory.h"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#if defined(_WIN32)
#    ifndef NOMINMAX
#        define NOMINMAX
#    endif
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#endif


namespace BixEngine::Gui
{
    using namespace Theme;
    using namespace Utils;

    namespace
    {
        constexpr const char* kScriptHeaderExtension = ".h";
        constexpr const char* kScriptSourceExtension = ".cpp";

        void RemoveExtensionIfPresent(std::string& value, const char* extension)
        {
            const size_t extLength = std::strlen(extension);
            if (extLength == 0)
                return;

            if (value.length() >= extLength && value.compare(value.length() - extLength, extLength, extension) == 0)
                value.erase(value.length() - extLength, extLength);
        }

        struct ScriptNode
        {
            std::string name{};
            std::string parentName{};
            std::string includePath{};
            std::filesystem::path headerPath{};
            bool inheritsActor{false};
            bool inheritsComponent{false};
            bool hasBlueprintMacro{false};
            std::vector<ScriptNode> children{};
        };

        struct ParentScriptInfo
        {
            std::string displayName{};
            std::string className{};
            std::string includePath{};
            std::filesystem::path headerPath{};
            bool isActor{false};
            bool isComponent{false};
            bool hasBlueprintMacro{false};

            [[nodiscard]] bool IsValid() const noexcept { return !className.empty(); }
        };

        struct PrefabScriptCandidate
        {
            std::string displayName{};
            std::string className{};
            std::string includePath{};
            std::filesystem::path headerPath{};
            bool isActor{false};
            bool isComponent{false};
            bool hasBlueprintMacro{false};
            std::string assetBaseName{};
        };

        const std::vector<ParentScriptInfo>& GetBaseClassParents()
        {
            static const std::vector<ParentScriptInfo> baseParents = {
                {"Actor", "BixEngine::Game::Actor", "Game/Actor.h", std::filesystem::path{}, true, false, true},
                {"Component", "BixEngine::Game::Component", "Game/Components/Component.h", std::filesystem::path{}, false, true, true},
            };
            return baseParents;
        }

        std::string EscapeJsonString(std::string_view value)
        {
            std::string escaped{};
            escaped.reserve(value.size());

            for (unsigned char ch : value)
            {
                switch (ch)
                {
                case '\\':
                    escaped += "\\\\";
                    break;
                case '"':
                    escaped += "\\\"";
                    break;
                case '\n':
                    escaped += "\\n";
                    break;
                case '\r':
                    escaped += "\\r";
                    break;
                case '\t':
                    escaped += "\\t";
                    break;
                default:
                    if (ch < 0x20)
                    {
                        char buffer[7]{};
                        std::snprintf(buffer, sizeof(buffer), "\\u%04x", ch);
                        escaped += buffer;
                    }
                    else
                    {
                        escaped += static_cast<char>(ch);
                    }
                    break;
                }
            }

            return escaped;
        }

        bool ValidatePrefabMetadata(const PopupRequestState& requests, String& errorStorage)
        {
            auto hasInvalidCharacters = [](std::string_view value)
            {
                return value.find_first_of("\r\n\"") != std::string::npos;
            };

            if (!requests.selectedPrefabClass.IsEmpty())
            {
                const std::string className = std::string(requests.selectedPrefabClass.View());
                if (hasInvalidCharacters(className))
                {
                    LogAndStoreError(errorStorage, "Selected script contains unsupported characters.", false);
                    return false;
                }
            }

            if (!requests.selectedPrefabInclude.IsEmpty())
            {
                const std::string includePath = std::string(requests.selectedPrefabInclude.View());
                if (hasInvalidCharacters(includePath))
                {
                    LogAndStoreError(errorStorage, "Include path contains unsupported characters.", false);
                    return false;
                }
            }

            return true;
        }

        bool MatchesActorType(std::string_view typeName)
        {
            return typeName == "Actor" || typeName == "BixEngine::Game::Actor" || typeName == "::BixEngine::Game::Actor";
        }

        bool MatchesComponentType(std::string_view typeName)
        {
            return typeName == "Component" || typeName == "BixEngine::Game::Component" || typeName == "::BixEngine::Game::Component";
        }

        template<typename TPredicate>
        bool InheritsFrom(const std::unordered_map<std::string, ScriptNode>& nodes, const std::string& startType, TPredicate predicate)
        {
            if (startType.empty())
                return false;

            std::unordered_set<std::string> visited{};
            std::string current = startType;

            while (!current.empty())
            {
                if (predicate(current))
                    return true;

                auto it = nodes.find(current);
                if (it == nodes.end())
                    break;

                if (!visited.insert(it->first).second)
                    break;

                current = it->second.parentName;
            }

            return false;
        }

        std::string TrimWhitespace(std::string value)
        {
            auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char ch) { return !isSpace(ch); }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) { return !isSpace(ch); }).base(), value.end());
            return value;
        }

        bool CaseInsensitiveLess(const std::string& lhs, const std::string& rhs)
        {
            auto toLower = [](unsigned char ch) { return static_cast<unsigned char>(std::tolower(ch)); };
            return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                [&](unsigned char a, unsigned char b)
                {
                    return toLower(a) < toLower(b);
                });
        }

        bool ContainsBlueprintMacro(const std::filesystem::path& headerPath)
        {
            std::ifstream file(headerPath);
            if (!file.is_open())
                return false;

            std::string line;
            while (std::getline(file, line))
            {
                if (line.find("BCLASS") != std::string::npos)
                    return true;
            }

            return false;
        }

        std::string SanitizeForFileName(std::string value)
        {
            auto isValidChar = [](unsigned char ch)
            {
                return std::isalnum(ch) != 0 || ch == '_' || ch == '-';
            };

            for (char& ch : value)
            {
                if (!isValidChar(static_cast<unsigned char>(ch)))
                    ch = '_';
            }

            while (!value.empty() && value.front() == '_')
                value.erase(value.begin());
            while (!value.empty() && value.back() == '_')
                value.pop_back();

            return value;
        }

        void CollectPrefabCandidatesFromNode(const ScriptNode& node, std::vector<PrefabScriptCandidate>& out)
        {
            const bool qualifies = node.inheritsActor || node.inheritsComponent || node.hasBlueprintMacro;
            if (qualifies)
            {
                PrefabScriptCandidate candidate{};
                candidate.displayName = node.name;
                candidate.className = node.name;
                candidate.includePath = node.includePath;
                candidate.headerPath = node.headerPath;
                candidate.isActor = node.inheritsActor;
                candidate.isComponent = node.inheritsComponent;
                candidate.hasBlueprintMacro = node.hasBlueprintMacro;
                if (candidate.assetBaseName.empty())
                {
                    if (!node.headerPath.empty())
                        candidate.assetBaseName = node.headerPath.stem().generic_string();
                    else
                        candidate.assetBaseName = node.name;
                }

                candidate.assetBaseName = SanitizeForFileName(std::move(candidate.assetBaseName));
                if (candidate.assetBaseName.empty())
                    candidate.assetBaseName = "Prefab";

                if (!candidate.isActor && !candidate.isComponent)
                    candidate.isActor = true;

                out.emplace_back(std::move(candidate));
            }

            for (const ScriptNode& child : node.children)
                CollectPrefabCandidatesFromNode(child, out);
        }

        std::vector<PrefabScriptCandidate> BuildPrefabCandidateList(const std::vector<ScriptNode>& nodes, const std::vector<ParentScriptInfo>& baseParents)
        {
            std::vector<PrefabScriptCandidate> candidates{};
            candidates.reserve(nodes.size() + baseParents.size());

            for (const ParentScriptInfo& base : baseParents)
            {
                PrefabScriptCandidate candidate{};
                candidate.displayName = base.displayName + " (Engine)";
                candidate.className = base.className;
                candidate.includePath = base.includePath;
                candidate.headerPath = base.headerPath;
                candidate.isActor = base.isActor;
                candidate.isComponent = base.isComponent;
                candidate.hasBlueprintMacro = base.hasBlueprintMacro;
                candidate.assetBaseName = SanitizeForFileName(base.displayName);
                if (candidate.assetBaseName.empty())
                    candidate.assetBaseName = "Prefab";
                if (!candidate.isActor && !candidate.isComponent)
                    candidate.isActor = true;
                candidates.emplace_back(std::move(candidate));
            }

            for (const ScriptNode& node : nodes)
                CollectPrefabCandidatesFromNode(node, candidates);

            std::sort(candidates.begin(), candidates.end(), [](const PrefabScriptCandidate& lhs, const PrefabScriptCandidate& rhs)
            {
                return CaseInsensitiveLess(lhs.displayName, rhs.displayName);
            });

            return candidates;
        }

        void SetSelectedPrefab(PopupRequestState& requests, const PrefabScriptCandidate& candidate)
        {
            requests.selectedPrefabClass = candidate.className;
            requests.selectedPrefabInclude = candidate.includePath;
            requests.selectedPrefabAssetBase = candidate.assetBaseName;
            requests.selectedPrefabScript = candidate.headerPath;
            requests.selectedPrefabIsActor = candidate.isActor;
            requests.selectedPrefabIsComponent = candidate.isComponent;

            if (!requests.selectedPrefabIsActor && !requests.selectedPrefabIsComponent)
                requests.selectedPrefabIsActor = true;
        }

        std::string BuildIncludePath(const std::filesystem::path& headerPath, const std::filesystem::path& scriptsDirectory, const std::filesystem::path& contentRoot)
        {
            std::error_code relativeError;
            std::filesystem::path relativeScripts = std::filesystem::relative(headerPath, scriptsDirectory, relativeError);
            if (!relativeError)
                return relativeScripts.generic_string();

            relativeError.clear();
            std::filesystem::path relativeContent = std::filesystem::relative(headerPath, contentRoot, relativeError);
            if (!relativeError)
                return relativeContent.generic_string();

            return headerPath.filename().generic_string();
        }

        void ParseClassDeclaration(const std::filesystem::path& headerPath, std::string& outClassName, std::string& outParentName)
        {
            std::ifstream file(headerPath);
            if (!file.is_open())
                return;

            std::string line;
            while (std::getline(file, line))
            {
                std::string sanitized = line;
                const size_t commentPos = sanitized.find("//");
                if (commentPos != std::string::npos)
                    sanitized = sanitized.substr(0, commentPos);

                sanitized = TrimWhitespace(std::move(sanitized));
                if (sanitized.empty())
                    continue;

                const size_t classPos = sanitized.find("class");
                if (classPos == std::string::npos)
                    continue;

                if (classPos > 0)
                {
                    const char prev = sanitized[classPos - 1];
                    if (std::isalnum(static_cast<unsigned char>(prev)) || prev == '_')
                        continue;
                }

                std::istringstream declarationStream(sanitized.substr(classPos + 5));
                std::string token;
                auto isSkippableToken = [](const std::string& value)
                {
                    if (value.empty())
                        return true;
                    if (value == "final" || value == "abstract")
                        return true;
                    if (value.find('(') != std::string::npos)
                        return true;
                    return std::all_of(value.begin(), value.end(), [](unsigned char ch)
                    {
                        return std::isupper(ch) || std::isdigit(ch) || ch == '_';
                    });
                };

                while (declarationStream >> token)
                {
                    if (!isSkippableToken(token))
                    {
                        outClassName = token;
                        break;
                    }
                }

                const size_t colonPos = sanitized.find(':', classPos);
                if (colonPos != std::string::npos)
                {
                    std::string inheritanceClause = sanitized.substr(colonPos + 1);
                    const size_t publicPos = inheritanceClause.find("public");
                    if (publicPos != std::string::npos)
                    {
                        size_t parentStart = publicPos + 6;
                        while (parentStart < inheritanceClause.size() && std::isspace(static_cast<unsigned char>(inheritanceClause[parentStart])))
                            ++parentStart;

                        size_t parentEnd = parentStart;
                        int templateDepth = 0;
                        while (parentEnd < inheritanceClause.size())
                        {
                            const char ch = inheritanceClause[parentEnd];
                            if (ch == '<')
                            {
                                ++templateDepth;
                            }
                            else if (ch == '>')
                            {
                                --templateDepth;
                            }
                            else if (templateDepth == 0)
                            {
                                if (ch == ',' || ch == '{')
                                    break;
                                if (std::isspace(static_cast<unsigned char>(ch)))
                                    break;
                            }

                            ++parentEnd;
                        }

                        std::string parentName = TrimWhitespace(inheritanceClause.substr(parentStart, parentEnd - parentStart));
                        if (!parentName.empty())
                            outParentName = std::move(parentName);
                    }
                }

                break;
            }
        }

        std::vector<ScriptNode> ParseScriptHierarchy(const std::filesystem::path& scriptsDirectory, const std::filesystem::path& contentRoot)
        {
            namespace fs = std::filesystem;

            std::vector<ScriptNode> roots{};
            std::error_code existsError;
            if (!fs::exists(scriptsDirectory, existsError) || existsError)
                return roots;

            std::unordered_map<std::string, ScriptNode> nodes{};
            nodes.reserve(32);

            std::error_code iterationError;
            fs::recursive_directory_iterator it(scriptsDirectory, fs::directory_options::skip_permission_denied, iterationError);
            fs::recursive_directory_iterator end{};

            for (; it != end; it.increment(iterationError))
            {
                if (iterationError)
                {
                    iterationError.clear();
                    continue;
                }

                const fs::directory_entry& entry = *it;
                if (!entry.is_regular_file())
                    continue;

                const fs::path& path = entry.path();
                if (path.extension() != kScriptHeaderExtension)
                    continue;

                std::string className = path.stem().generic_string();
                std::string parentName{};
                ParseClassDeclaration(path, className, parentName);

                ScriptNode node{};
                node.name = className;
                node.parentName = parentName;
                node.includePath = BuildIncludePath(path, scriptsDirectory, contentRoot);
                node.headerPath = path;
                node.hasBlueprintMacro = ContainsBlueprintMacro(path);

                auto [existing, inserted] = nodes.emplace(node.name, node);
                if (!inserted)
                {
                    if (!node.parentName.empty() && existing->second.parentName.empty())
                        existing->second.parentName = node.parentName;
                    if (!node.includePath.empty())
                        existing->second.includePath = node.includePath;
                    if (!node.headerPath.empty())
                        existing->second.headerPath = node.headerPath;
                    existing->second.hasBlueprintMacro = existing->second.hasBlueprintMacro || node.hasBlueprintMacro;
                }
            }

            if (nodes.empty())
                return roots;

            std::unordered_map<std::string, std::vector<std::string>> children{};
            children.reserve(nodes.size());
            for (const auto& [name, node] : nodes)
            {
                if (!node.parentName.empty())
                    children[node.parentName].push_back(name);
            }

            for (auto& [_, childList] : children)
                std::sort(childList.begin(), childList.end(), CaseInsensitiveLess);

            auto buildTree = [&](const auto& self, const std::string& name, std::unordered_set<std::string>& visited) -> ScriptNode
            {
                ScriptNode result = nodes.at(name);
                result.inheritsActor = InheritsFrom(nodes, result.parentName, MatchesActorType);
                result.inheritsComponent = InheritsFrom(nodes, result.parentName, MatchesComponentType);

                auto childIt = children.find(name);
                if (childIt != children.end())
                {
                    for (const auto& childName : childIt->second)
                    {
                        if (visited.insert(childName).second)
                            result.children.push_back(self(self, childName, visited));
                    }
                }

                return result;
            };

            std::vector<std::string> rootNames{};
            rootNames.reserve(nodes.size());
            for (const auto& [name, node] : nodes)
            {
                if (node.parentName.empty() || nodes.find(node.parentName) == nodes.end())
                    rootNames.push_back(name);
            }

            std::sort(rootNames.begin(), rootNames.end(), CaseInsensitiveLess);

            std::unordered_set<std::string> visited{};
            visited.reserve(nodes.size());

            for (const auto& rootName : rootNames)
            {
                if (visited.insert(rootName).second)
                    roots.push_back(buildTree(buildTree, rootName, visited));
            }

            for (const auto& [name, _] : nodes)
            {
                if (visited.insert(name).second)
                    roots.push_back(buildTree(buildTree, name, visited));
            }

            auto sortChildren = [&](const auto& self, ScriptNode& node) -> void
            {
                std::sort(node.children.begin(), node.children.end(), [](const ScriptNode& lhs, const ScriptNode& rhs)
                {
                    return CaseInsensitiveLess(lhs.name, rhs.name);
                });
                for (auto& child : node.children)
                    self(self, child);
            };

            for (auto& root : roots)
                sortChildren(sortChildren, root);

            std::sort(roots.begin(), roots.end(), [](const ScriptNode& lhs, const ScriptNode& rhs)
            {
                return CaseInsensitiveLess(lhs.name, rhs.name);
            });

            return roots;
        }

        std::vector<Utils::TreeNodeData> BuildGuiTreeNodes(const std::vector<ScriptNode>& nodes, std::unordered_map<std::string, ParentScriptInfo>& outInfo)
        {
            std::vector<Utils::TreeNodeData> guiNodes{};
            guiNodes.reserve(nodes.size());

            for (const ScriptNode& node : nodes)
            {
                ParentScriptInfo info{};
                info.displayName = node.name;
                info.className = node.name;
                info.includePath = node.includePath;
                info.headerPath = node.headerPath;
                info.isActor = node.inheritsActor;
                info.isComponent = node.inheritsComponent;
                info.hasBlueprintMacro = node.hasBlueprintMacro;
                outInfo.emplace(info.className, info);

                Utils::TreeNodeData guiNode{};
                guiNode.name = node.name;
                guiNode.children = BuildGuiTreeNodes(node.children, outInfo);
                guiNode.isLeaf = guiNode.children.empty();

                guiNodes.emplace_back(std::move(guiNode));
            }

            return guiNodes;
        }

        ParentScriptInfo GetSelectedParentInfo(const PopupRequestState& requests)
        {
            ParentScriptInfo info{};
            if (!requests.selectedParentClass.IsEmpty())
            {
                info.displayName = requests.selectedParentDisplay.View();
                info.className = requests.selectedParentClass.View();
                info.includePath = requests.selectedParentInclude.View();
                info.isActor = requests.selectedParentIsActor;
                info.isComponent = requests.selectedParentIsComponent;
            }
            return info;
        }

        void SetSelectedParent(PopupRequestState& requests, const ParentScriptInfo& info, bool isBaseParent)
        {
            requests.selectedParentClass = info.className;
            requests.selectedParentInclude = info.includePath;
            requests.selectedParentDisplay = info.displayName;
            requests.selectedParentIsBase = isBaseParent;
            requests.selectedParentIsActor = info.isActor;
            requests.selectedParentIsComponent = info.isComponent;

            if (info.isComponent)
                requests.scriptType = ScriptTemplateType::Component;
            else if (info.isActor)
                requests.scriptType = ScriptTemplateType::Actor;
        }

        void RenderCreatePrefabPopup(ContentBrowserState& state, PopupRequestState& requests)
        {
            namespace fs = std::filesystem;

            if (requests.createPrefab)
            {
                ImGui::OpenPopup("ContentBrowserCreatePrefab");
                requests.createPrefab = false;
            }

            if (!ImGui::BeginPopupModal("ContentBrowserCreatePrefab", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                return;

            Utils::DrawDescriptionText("Create a prefab asset bound to an existing gameplay script.");
            ImGui::Spacing();

            const fs::path scriptsDirectory = state.root / "Scripts";
            const auto& baseParents = GetBaseClassParents();
            const std::vector<ScriptNode> userScriptRoots = ParseScriptHierarchy(scriptsDirectory, state.root);
            const std::vector<PrefabScriptCandidate> candidates = BuildPrefabCandidateList(userScriptRoots, baseParents);

            static char searchBuffer[128] = "";
            ImGui::InputTextWithHint("##PrefabSearch", "Search scripts...", searchBuffer, IM_ARRAYSIZE(searchBuffer));

            const std::string filter = [&]()
            {
                std::string result(searchBuffer);
                std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                return result;
            }();

            ImGui::Spacing();

            const float listHeight = ImGui::GetTextLineHeightWithSpacing() * 12.0f;
            if (ImGui::BeginChild("PrefabCandidateList", ImVec2(420.0f, listHeight), true))
            {
                if (candidates.empty())
                {
                    ImGui::TextDisabled("No eligible scripts were found.");
                }
                else
                {
                    for (const PrefabScriptCandidate& candidate : candidates)
                    {
                        std::string displayLower = candidate.displayName;
                        std::transform(displayLower.begin(), displayLower.end(), displayLower.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                        if (!filter.empty() && displayLower.find(filter) == std::string::npos)
                            continue;

                        std::string label = candidate.displayName;
                        if (candidate.isActor && candidate.isComponent)
                            label += " [Actor/Component]";
                        else if (candidate.isActor)
                            label += " [Actor]";
                        else if (candidate.isComponent)
                            label += " [Component]";
                        else if (candidate.hasBlueprintMacro)
                            label += " [Blueprint]";

                        const bool isSelected = !requests.selectedPrefabClass.IsEmpty() && requests.selectedPrefabClass.View() == candidate.className;
                        if (ImGui::Selectable(label.c_str(), isSelected))
                        {
                            SetSelectedPrefab(requests, candidate);
                            requests.prefabError.Clear();
                        }

                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort) && ImGui::BeginTooltip())
                        {
                            ImGui::TextUnformatted(candidate.className.c_str());
                            if (!candidate.includePath.empty())
                                ImGui::Text("Include: %s", candidate.includePath.c_str());
                            if (!candidate.headerPath.empty())
                                ImGui::Text("Header: %s", candidate.headerPath.generic_string().c_str());
                            ImGui::EndTooltip();
                        }
                    }
                }
            }
            ImGui::EndChild();

            ImGui::Spacing();

            if (!requests.prefabError.IsEmpty())
                Utils::DrawErrorMessage(std::string(requests.prefabError.View()));

            Utils::DrawSeparatorText("Prefab details");

            const bool isComponentPrefab = requests.selectedPrefabIsComponent && !requests.selectedPrefabIsActor ? true : false;
            const char* prefabTypeLabel = isComponentPrefab ? "Component" : "Actor";
            const char* prefabExtension = isComponentPrefab ? ".bixcomponent" : ".bixactor";

            Utils::DrawLabelValue("Script", requests.selectedPrefabClass.IsEmpty() ? "None" : requests.selectedPrefabClass.View().data(), "None");
            Utils::DrawLabelValue("Type", prefabTypeLabel, "Actor");

            std::string baseNamePreview = requests.selectedPrefabAssetBase.IsEmpty() ? std::string{} : std::string(requests.selectedPrefabAssetBase.View());
            if (baseNamePreview.empty() && !requests.selectedPrefabClass.IsEmpty())
                baseNamePreview = SanitizeForFileName(std::string(requests.selectedPrefabClass.View()));
            if (baseNamePreview.empty())
                baseNamePreview = "Prefab";

            const fs::path relativeDirectory = state.current.lexically_relative(state.root);
            std::string locationDisplay = "Content";
            if (!relativeDirectory.empty() && relativeDirectory.generic_string() != ".")
            {
                locationDisplay += '/';
                locationDisplay += relativeDirectory.generic_string();
            }

            Utils::DrawLabelValue("Location", locationDisplay, "Content");
            Utils::DrawLabelValue("File", baseNamePreview + prefabExtension, "Prefab.bixactor");

            ImGui::Spacing();

            const bool confirm = Utils::DrawConfirmButtons("Create", "Cancel",
                []() {},
                [&]()
                {
                    ImGui::CloseCurrentPopup();
                    ClearSelectedPrefab(requests);
                    requests.prefabError.Clear();
                });

            if (confirm)
            {
                if (requests.selectedPrefabClass.IsEmpty())
                {
                    LogAndStoreError(requests.prefabError, "Please select a script to instantiate.", false);
                }
                else
                {
                    const fs::path targetFile = state.current / (baseNamePreview + prefabExtension);
                    if (fs::exists(targetFile))
                    {
                        LogAndStoreError(requests.prefabError, "An asset with this name already exists.", false);
                    }
                    else if (ValidatePrefabMetadata(requests, requests.prefabError))
                    {
                        std::ostringstream content;
                        content << "{\n";
                        content << "    \"type\": \"" << prefabTypeLabel << "\",\n";
                        const std::string className = EscapeJsonString(std::string(requests.selectedPrefabClass.View()));
                        content << "    \"class\": \"" << className << "\"";
                        if (!requests.selectedPrefabInclude.IsEmpty())
                        {
                            const std::string includeValue = EscapeJsonString(std::string(requests.selectedPrefabInclude.View()));
                            content << ",\n    \"include\": \"" << includeValue << "\"";
                        }
                        content << "\n}\n";

                        if (TryWriteFile(targetFile, content.str(), requests.prefabError))
                        {
                            ImGui::CloseCurrentPopup();
                            ClearSelectedPrefab(requests);
                            requests.prefabError.Clear();
                            state.cache.dirty = true;
                        }
                    }
                }
            }

            ImGui::EndPopup();
        }

        void RenderCreateScriptPopup(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requests)
        {
            namespace fs = std::filesystem;

            if (requests.createScript)
            {
                ImGui::OpenPopup("ContentBrowserCreateScript");
                requests.createScript = false;
            }

            if (!ImGui::BeginPopupModal("ContentBrowserCreateScript", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                return;

            const fs::path scriptsDirectory = state.root / "Scripts";
            const auto& baseParents = GetBaseClassParents();
            const std::vector<ScriptNode> userScriptRoots = ParseScriptHierarchy(scriptsDirectory, state.root);
            std::unordered_map<std::string, ParentScriptInfo> userScriptInfo{};
            const std::vector<Utils::TreeNodeData> userScriptTree = BuildGuiTreeNodes(userScriptRoots, userScriptInfo);

            Utils::DrawDescriptionText("Create a new C++ script in the current directory.");
            ImGui::Spacing();

            Utils::DrawSeparatorText("Script details");

            fs::path relativeLocation = state.current.lexically_relative(state.root);
            std::string relativeLocationString = relativeLocation.generic_string();
            if (relativeLocationString == ".")
                relativeLocationString.clear();

            std::string locationDisplay = "Content";
            if (!relativeLocationString.empty())
            {
                locationDisplay += '/';
                locationDisplay += relativeLocationString;
            }

            Utils::DrawLabelValue("Location", locationDisplay, "Content");

            const bool shouldAutofocusName = ImGui::IsWindowAppearing();
            bool create = Utils::InputTextWithLabel("Script name (.h / .cpp)", requests.scriptName, IM_ARRAYSIZE(requests.scriptName), ImGuiInputTextFlags_EnterReturnsTrue, shouldAutofocusName);

            if (!requests.scriptError.IsEmpty())
            {
                Utils::DrawErrorMessage(std::string(requests.scriptError.View()));
            }

            String trimmedInput = TrimCopy(String(requests.scriptName));
            std::string baseNamePreview = trimmedInput.IsEmpty() ? std::string{} : std::string(trimmedInput.View());
            RemoveExtensionIfPresent(baseNamePreview, kScriptHeaderExtension);
            RemoveExtensionIfPresent(baseNamePreview, kScriptSourceExtension);

            ImGui::Spacing();
            Utils::DrawSeparatorText("Parent (optional)");
            ImGui::TextDisabled("Pick an inheritance target or leave empty for a standalone script.");

            const float parentListHeight = ImGui::GetTextLineHeightWithSpacing() * 7.0f;
            if (ImGui::BeginTable("ParentSelectionTable", 2, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter))
            {
                ImGui::TableSetupColumn("Base classes");
                ImGui::TableSetupColumn("Existing scripts");
                ImGui::TableHeadersRow();

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                if (ImGui::BeginChild("BaseClassList", ImVec2(0.0f, parentListHeight), true))
                {
                    if (baseParents.empty())
                    {
                        ImGui::TextDisabled("No base classes available.");
                    }
                    else
                    {
                        for (size_t baseIndex = 0; baseIndex < baseParents.size(); ++baseIndex)
                        {
                            ScopedID baseId(static_cast<int>(baseIndex));
                            const auto& baseParent = baseParents[baseIndex];
                            const bool isSelectedBase = requests.selectedParentIsBase && !requests.selectedParentClass.IsEmpty() && requests.selectedParentClass.View() == baseParent.className;
                            if (ImGui::Selectable(baseParent.displayName.c_str(), isSelectedBase))
                            {
                                SetSelectedParent(requests, baseParent, true);
                            }
                            ShowTooltip(baseParent.className.c_str(), ImGuiHoveredFlags_DelayNormal);
                        }
                    }
                }
                ImGui::EndChild();

                ImGui::TableSetColumnIndex(1);
                if (ImGui::BeginChild("UserScriptList", ImVec2(0.0f, parentListHeight), true, ImGuiWindowFlags_HorizontalScrollbar))
                {
                    std::string selectedScript = (!requests.selectedParentIsBase && !requests.selectedParentClass.IsEmpty()) ? std::string(requests.selectedParentClass.View()) : std::string{};
                    const std::string previousSelection = selectedScript;
                    Utils::DrawScriptHierarchyTree(userScriptTree, selectedScript, "No user scripts detected.");
                    if (selectedScript != previousSelection)
                    {
                        const auto infoIt = userScriptInfo.find(selectedScript);
                        if (infoIt != userScriptInfo.end())
                        {
                            SetSelectedParent(requests, infoIt->second, false);
                        }
                    }
                }
                ImGui::EndChild();

                ImGui::EndTable();
            }

            ParentScriptInfo parentInfo = GetSelectedParentInfo(requests);

            ImGui::Spacing();
            const std::string parentPreview = parentInfo.IsValid() ? parentInfo.displayName : std::string("None");
            Utils::DrawLabelValue("Selected", parentPreview, "None");
            ImGui::SameLine();
            if (Utils::IconButton("x", "Clear parent selection"))
            {
                ClearSelectedParent(requests);
                parentInfo = ParentScriptInfo{};
            }

            if (parentInfo.IsValid())
            {
                ImGui::TextDisabled("Class: %s", parentInfo.className.c_str());
                if (!parentInfo.includePath.empty())
                    ImGui::TextDisabled("Include: %s", parentInfo.includePath.c_str());
            }
            else
            {
                ImGui::TextDisabled("This script will not inherit from another class.");
            }

            ImGui::Spacing();
            Utils::DrawSeparatorText("Preview");
            if (baseNamePreview.empty())
            {
                ImGui::TextDisabled("Enter a script name to preview the generated files.");
            }
            else
            {
                std::string relativeDirectory = relativeLocationString;
                if (!relativeDirectory.empty() && relativeDirectory.back() != '/')
                    relativeDirectory += '/';

                const std::string headerPreview = relativeDirectory + baseNamePreview + kScriptHeaderExtension;
                const std::string sourcePreview = relativeDirectory + baseNamePreview + kScriptSourceExtension;

                ImGui::BulletText("Content/%s", headerPreview.c_str());
                ImGui::BulletText("Content/%s", sourcePreview.c_str());
                if (parentInfo.IsValid())
                {
                    ImGui::BulletText("Inherits from %s", parentInfo.className.c_str());
                }
                else
                {
                    ImGui::BulletText("Standalone class");
                }
            }

            ImGui::Spacing();
            const bool confirmPressed = Utils::DrawConfirmButtons("Create", "Cancel",
                []() {},
                [&]()
                {
                    ImGui::CloseCurrentPopup();
                    requests.scriptError.Clear();
                    ClearSelectedParent(requests);
                });
            create = create || confirmPressed;

            if (create)
            {
                String rawInput = TrimCopy(String(requests.scriptName));
                if (rawInput.IsEmpty())
                {
                    LogAndStoreError(requests.scriptError, "Script name cannot be empty.", false);
                }
                else if (ContainsPathSeparator(rawInput))
                {
                    LogAndStoreError(requests.scriptError, "Script name cannot contain path separators.", false);
                }
                else
                {
                    std::string baseName(rawInput.View());
                    RemoveExtensionIfPresent(baseName, kScriptHeaderExtension);
                    RemoveExtensionIfPresent(baseName, kScriptSourceExtension);

                    if (baseName.empty())
                    {
                        LogAndStoreError(requests.scriptError, "Script name cannot be empty.", false);
                    }
                    else
                    {
                        const fs::path headerFileName = fs::path(baseName + kScriptHeaderExtension);
                        const fs::path sourceFileName = fs::path(baseName + kScriptSourceExtension);
                        const fs::path headerPath = state.current / headerFileName;
                        const fs::path sourcePath = state.current / sourceFileName;

                        if (fs::exists(headerPath) || fs::exists(sourcePath))
                        {
                            LogAndStoreError(requests.scriptError, "A file with this name already exists.", false);
                        }
                        else
                        {
                            std::error_code createDirectoryError;
                            fs::create_directories(headerPath.parent_path(), createDirectoryError);
                            if (createDirectoryError)
                            {
                                String message = "Unable to ensure directory exists: ";
                                message += createDirectoryError.message();
                                LogAndStoreError(requests.scriptError, std::move(message));
                            }
                            else
                            {
                                std::ofstream headerFile(headerPath);
                                if (!headerFile.is_open())
                                {
                                    LogAndStoreError(requests.scriptError, "Failed to create the header file.");
                                }
                                else
                                {
                                    std::ofstream sourceFile(sourcePath);
                                    if (!sourceFile.is_open())
                                    {
                                        headerFile.close();
                                        std::error_code removeHeaderError;
                                        fs::remove(headerPath, removeHeaderError);
                                        LogAndStoreError(requests.scriptError, "Failed to create the source file.");
                                    }
                                    else
                                    {
                                        const ParentScriptInfo info = GetSelectedParentInfo(requests);
                                        const bool hasParent = info.IsValid();
                                        const bool hasParentInclude = hasParent && !info.includePath.empty();
                                        const bool defaultComponent = requests.scriptType == ScriptTemplateType::Component;
                                        const bool inheritsComponent = requests.selectedParentIsComponent || info.isComponent || defaultComponent;
                                        const std::string defaultBase = inheritsComponent ? "::BixEngine::Game::Component" : "::BixEngine::Game::Actor";
                                        const std::string baseType = hasParent ? info.className : defaultBase;
                                        const std::string baseInclude = inheritsComponent ? "Game/Components/Component.h" : "Game/Actor.h";

                                        headerFile << "#pragma once\n\n";
                                        headerFile << "// Parent: " << (hasParent ? info.className : "(none)") << '\n';
                                        headerFile << "// Created automatically from the Content Browser\n\n";
                                        headerFile << "#include \"" << baseInclude << "\"\n";
                                        if (hasParentInclude)
                                            headerFile << "#include \"" << info.includePath << "\"\n";
                                        headerFile << "#include \"" << baseName << ".generated.h\"\n\n";
                                        headerFile << "namespace BixEngine::Game\n";
                                        headerFile << "{\n";
                                        headerFile << "    BCLASS()\n";
                                        headerFile << "    class " << baseName << " : public " << baseType << '\n';
                                        headerFile << "    {\n";
                                        headerFile << "        GENERATED_BODY()\n";
                                        headerFile << "\n";
                                        headerFile << "    public:\n";
                                        headerFile << "        using Super = " << baseType << ";\n\n";
                                        if (inheritsComponent)
                                            headerFile << "        explicit " << baseName << "(Actor* owner);\n";
                                        else
                                            headerFile << "        " << baseName << "();\n";
                                        headerFile << "\n";
                                        headerFile << "        void BeginPlay() override;\n";
                                        headerFile << "        void Update(float deltaTime) override;\n";
                                        headerFile << "    };\n";
                                        headerFile << "}\n\n";

                                        sourceFile << "#include \"" << baseName << kScriptHeaderExtension << "\"\n\n";
                                        sourceFile << "namespace BixEngine::Game\n";
                                        sourceFile << "{\n";
                                        if (inheritsComponent)
                                        {
                                            sourceFile << "    " << baseName << "::" << baseName << "(Actor* owner)\n";
                                            sourceFile << "        : " << baseType << "(owner)\n";
                                            sourceFile << "    {\n";
                                            sourceFile << "    }\n\n";
                                        }
                                        else
                                        {
                                            sourceFile << "    " << baseName << "::" << baseName << "() = default;\n\n";
                                        }
                                        sourceFile << "    void " << baseName << "::BeginPlay()\n";
                                        sourceFile << "    {\n";
                                        sourceFile << "        Super::BeginPlay();\n";
                                        sourceFile << "    }\n\n";
                                        sourceFile << "    void " << baseName << "::Update(float deltaTime)\n";
                                        sourceFile << "    {\n";
                                        sourceFile << "        Super::Update(deltaTime);\n";
                                        sourceFile << "    }\n";
                                        sourceFile << "}\n\n";

                                        headerFile.flush();
                                        sourceFile.flush();

                                        if (!headerFile.good() || !sourceFile.good())
                                        {
                                            headerFile.close();
                                            sourceFile.close();

                                            std::error_code removeHeaderError;
                                            std::error_code removeSourceError;
                                            fs::remove(headerPath, removeHeaderError);
                                            fs::remove(sourcePath, removeSourceError);

                                            LogAndStoreError(requests.scriptError, "Failed to write the script files to disk.");
                                        }
                                        else
                                        {
                                            headerFile.close();
                                            sourceFile.close();

                                            selectedEntry = headerPath.generic_string();
                                            requests.scriptError.Clear();
                                            ClearSelectedParent(requests);
                                            state.cache.dirty = true;
                                            ImGui::CloseCurrentPopup();

                                            std::filesystem::path toolPath = Core::FindToolExecutable("BixHeaderTool.exe");
                                            std::filesystem::path headerPathAbs = std::filesystem::weakly_canonical(headerPath);

                                            RunBixHeaderTool(toolPath, headerPathAbs);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            ImGui::EndPopup();
        }
        void RenderCreateFolderPopup(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requests)
        {
            namespace fs = std::filesystem;

            if (requests.createFolder)
            {
                ImGui::OpenPopup("ContentBrowserCreateFolder");
                requests.createFolder = false;
            }

            if (!ImGui::BeginPopupModal("ContentBrowserCreateFolder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                return;

            const fs::path targetDirectory = requests.folderTarget.empty() ? state.current : requests.folderTarget;
            fs::path relativeTarget = targetDirectory.lexically_relative(state.root);
            std::string relativeString = relativeTarget.generic_string();

            String displayLabel = "Content";
            if (!relativeString.empty() && relativeString != ".")
            {
                displayLabel += '/';
                displayLabel += relativeString;
            }

            String description = "Create a new folder in: ";
            description += displayLabel;
            Utils::DrawDescriptionText(description.c_str());

            const bool shouldAutofocus = ImGui::IsWindowAppearing();
            bool create = Utils::InputTextWithLabel("Folder name", requests.folderName, IM_ARRAYSIZE(requests.folderName), ImGuiInputTextFlags_EnterReturnsTrue, shouldAutofocus);

            if (!requests.folderError.IsEmpty())
            {
                Utils::DrawErrorMessage(std::string(requests.folderError.View()));
            }

            const bool confirmPressed = Utils::DrawConfirmButtons("Create", "Cancel",
                []() {},
                [&]()
                {
                    ImGui::CloseCurrentPopup();
                    requests.folderError.Clear();
                    requests.folderTarget.clear();
                });
            create = create || confirmPressed;

            if (create)
            {
                String folderName = TrimCopy(String(requests.folderName));

                if (folderName.IsEmpty())
                {
                    LogAndStoreError(requests.folderError, "Folder name cannot be empty.", false);
                }
                else if (ContainsPathSeparator(folderName))
                {
                    LogAndStoreError(requests.folderError, "Folder name cannot contain path separators.", false);
                }
                else
                {
                    const fs::path baseDirectory = requests.folderTarget.empty() ? state.current : requests.folderTarget;
                    const fs::path folderPath = baseDirectory / folderName.View();

                    if (fs::exists(folderPath))
                    {
                        LogAndStoreError(requests.folderError, "A folder with this name already exists.", false);
                    }
                    else if (TryCreateDir(folderPath, requests.folderError))
                    {
                        selectedEntry = folderPath.generic_string();
                        requests.folderError.Clear();
                        requests.folderTarget.clear();
                        state.cache.dirty = true;
                        ImGui::CloseCurrentPopup();
                    }
                }
            }

            ImGui::EndPopup();
        }

        void RenderCreateSpriteAtlasPopup(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requests)
        {
            namespace fs = std::filesystem;

            if (requests.createSpriteAtlas)
            {
                ImGui::OpenPopup("ContentBrowserCreateSpriteAtlas");
                requests.createSpriteAtlas = false;
            }

            ImGui::SetNextWindowSize(ImVec2(480, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSizeConstraints(ImVec2(400, 0), ImVec2(600, 600));
            if (!ImGui::BeginPopupModal("ContentBrowserCreateSpriteAtlas", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
                return;

            const fs::path targetDirectory = requests.spriteAtlasTarget.empty() ? state.current : requests.spriteAtlasTarget;
            fs::path relativeTarget = targetDirectory.lexically_relative(state.root);
            std::string relativeString = relativeTarget.generic_string();

            String locationDisplay = "Content";
            if (!relativeString.empty() && relativeString != ".")
            {
                locationDisplay += '/';
                locationDisplay += relativeString;
            }

            DrawDescriptionText("Create a sprite atlas file next to an existing texture.");
            DrawLabelValue("Location", locationDisplay.View().data(), "Content");
            ImGui::Separator();

            const bool shouldAutofocus = ImGui::IsWindowAppearing();
            InputTextWithLabel("Texture Source", requests.spriteAtlasTexturePath, IM_ARRAYSIZE(requests.spriteAtlasTexturePath), ImGuiInputTextFlags_None, shouldAutofocus);

            ImGui::SameLine();
            if (ImGui::Button("Browse..."))
                requests.spriteAtlasBrowseTextures = true;

            if (requests.spriteAtlasBrowseTextures)
            {
                ImGui::OpenPopup("ContentBrowserSpriteAtlasTexturePicker");
                requests.spriteAtlasBrowseTextures = false;
            }

            if (ImGui::BeginPopup("ContentBrowserSpriteAtlasTexturePicker"))
            {
                ImGui::Text("Select texture to use:");
                ImGui::Separator();

                ImGui::BeginChild("TextureList", ImVec2(300, 200), true);
                int textureCount = 0;
                std::error_code iteratorError;

                for (const auto& entry : fs::directory_iterator(targetDirectory, iteratorError))
                {
                    if (!entry.is_regular_file()) continue;

                    std::string ext = entry.path().extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext != ".png") continue;

                    textureCount++;
                    const std::string displayName = entry.path().filename().string();
                    if (ImGui::Selectable(displayName.c_str()))
                    {
                        std::snprintf(requests.spriteAtlasTexturePath, IM_ARRAYSIZE(requests.spriteAtlasTexturePath), "%s", displayName.c_str());
                        ImGui::CloseCurrentPopup();
                    }
                }

                if (iteratorError)
                    ImGui::TextDisabled("Error: %s", iteratorError.message().c_str());
                else if (textureCount == 0)
                    ImGui::TextDisabled("No .png textures found.");

                ImGui::EndChild();
                ImGui::EndPopup();
            }

            requests.spriteAtlasColumns = std::max(1, requests.spriteAtlasColumns);
            requests.spriteAtlasRows = std::max(1, requests.spriteAtlasRows);
            requests.spriteAtlasPadding = std::max(0, requests.spriteAtlasPadding);
            requests.spriteAtlasMargin = std::max(0, requests.spriteAtlasMargin);

            ImGui::InputInt("Columns", &requests.spriteAtlasColumns);
            ImGui::InputInt("Rows", &requests.spriteAtlasRows);
            ImGui::InputInt("Padding", &requests.spriteAtlasPadding);
            ImGui::InputInt("Margin", &requests.spriteAtlasMargin);

            if (!requests.spriteAtlasError.IsEmpty())
            {
                DrawErrorMessage(std::string(requests.spriteAtlasError.View()));
            }

            const bool confirmed = DrawConfirmButtons("✅ Create", "Cancel",
                []() {},
                [&]()
                {
                    ImGui::CloseCurrentPopup();
                    requests.spriteAtlasError.Clear();
                    requests.spriteAtlasTarget.clear();
                    requests.spriteAtlasBrowseTextures = false;
                });

            if (confirmed)
            {
                String textureInput = TrimCopy(String(requests.spriteAtlasTexturePath));
                if (textureInput.IsEmpty())
                {
                    LogAndStoreError(requests.spriteAtlasError, "Texture path cannot be empty.", false);
                }
                else
                {
                    fs::path texturePath = fs::path(textureInput.View());
                    if (!texturePath.is_absolute())
                        texturePath = targetDirectory / texturePath;

                    if (!fs::exists(texturePath))
                    {
                        LogAndStoreError(requests.spriteAtlasError, String("Texture not found: ") + texturePath.generic_string(), false);
                    }
                    else
                    {
                        resources::SpriteAtlasCreationParams params{};
                        params.texturePath = texturePath;
                        params.columns = requests.spriteAtlasColumns;
                        params.rows = requests.spriteAtlasRows;
                        params.padding = requests.spriteAtlasPadding;
                        params.margin = requests.spriteAtlasMargin;

                        fs::path atlasPath = texturePath;
                        atlasPath.replace_extension(".atlas");

                        if (resources::SpriteAtlasFactory::CreateAtlasFile(atlasPath, params, requests.spriteAtlasError))
                        {
                            requests.spriteAtlasError.Clear();
                            selectedEntry = atlasPath.generic_string();
                            state.cache.dirty = true;
                            requests.spriteAtlasTarget.clear();
                            requests.spriteAtlasBrowseTextures = false;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
            }

            ImGui::EndPopup();
        }

        void RenderRenameEntryPopup(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requests)
        {
            namespace fs = std::filesystem;

            if (requests.renameEntry)
            {
                ImGui::OpenPopup("ContentBrowserRenameEntry");
                requests.renameEntry = false;
            }

            if (!ImGui::BeginPopupModal("ContentBrowserRenameEntry", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                return;

            const bool renamingScriptGroup = requests.renameTargetIsScriptGroup;
            const fs::path target = renamingScriptGroup && requests.renameTarget.empty() ? requests.renameSecondaryTarget : requests.renameTarget;
            if (!target.empty())
            {
                fs::path relativePath = target.lexically_relative(state.root);
                std::string relativeString = relativePath.generic_string();

                String label = relativeString;
                if (label.IsEmpty() || label == ".")
                    label = target.filename().generic_string();

                String message = renamingScriptGroup ? String("Rename script: ") : String("Rename: ");
                message += label;
                DrawDescriptionText(message.c_str());
            }
            else
            {
                DrawDescriptionText("Rename the selected entry.");
            }

            const bool shouldAutofocus = ImGui::IsWindowAppearing();
            bool rename = InputTextWithLabel("New name", requests.renameBuffer, IM_ARRAYSIZE(requests.renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue, shouldAutofocus);

            if (!requests.renameError.IsEmpty())
            {
                DrawErrorMessage(std::string(requests.renameError.View()));
            }

            const bool confirmPressed = DrawConfirmButtons("Rename", "Cancel",
                []() {},
                [&]()
                {
                    ImGui::CloseCurrentPopup();
                    requests.renameError.Clear();
                    requests.renameTarget.clear();
                    requests.renameSecondaryTarget.clear();
                    requests.renameTargetIsScriptGroup = false;
                });
            rename = rename || confirmPressed;

            if (rename)
            {
                String newName = TrimCopy(String(requests.renameBuffer));

                if (newName.IsEmpty())
                {
                    LogAndStoreError(requests.renameError, "Name cannot be empty.", false);
                }
                else if (ContainsPathSeparator(newName))
                {
                    LogAndStoreError(requests.renameError, "Name cannot contain path separators.", false);
                }
                else if (!renamingScriptGroup && requests.renameTarget.empty())
                {
                    LogAndStoreError(requests.renameError, "No entry selected for rename.", false);
                }
                else if (renamingScriptGroup)
                {
                    const fs::path headerOldPath = requests.renameTarget;
                    const fs::path sourceOldPath = requests.renameSecondaryTarget;

                    if (headerOldPath.empty() && sourceOldPath.empty())
                    {
                        LogAndStoreError(requests.renameError, "No entry selected for rename.", false);
                    }
                    else
                    {
                        fs::path parent = !headerOldPath.empty() ? headerOldPath.parent_path() : sourceOldPath.parent_path();
                        if (parent.empty())
                            parent = state.current;

                        std::string newBaseName = std::string(newName.View());
                        const auto stripExtensionIfNeeded = [&](const fs::path& path)
                        {
                            if (path.empty())
                                return;

                            const std::string extension = path.extension().string();
                            if (extension.empty())
                                return;

                            if (newBaseName.length() >= extension.length() && newBaseName.compare(newBaseName.length() - extension.length(), extension.length(), extension) == 0)
                                newBaseName.erase(newBaseName.length() - extension.length());
                        };

                        stripExtensionIfNeeded(headerOldPath);
                        stripExtensionIfNeeded(sourceOldPath);

                        if (newBaseName.empty())
                        {
                            LogAndStoreError(requests.renameError, "Name cannot be empty.", false);
                        }
                        else
                        {
                            const std::string currentBaseName = !headerOldPath.empty()
                                ? headerOldPath.stem().generic_string()
                                : (!sourceOldPath.empty() ? sourceOldPath.stem().generic_string() : std::string{});

                            if (currentBaseName == newBaseName)
                            {
                                requests.renameError.Clear();
                                requests.renameTargetIsScriptGroup = false;
                                requests.renameSecondaryTarget.clear();
                                ImGui::CloseCurrentPopup();
                            }
                            else
                            {
                                const auto makeNewPath = [&](const fs::path& oldPath)
                                {
                                    if (oldPath.empty())
                                        return fs::path{};

                                    std::string newFileName = newBaseName;
                                    newFileName += oldPath.extension().string();
                                    return parent / newFileName;
                                };

                                const fs::path newHeaderPath = makeNewPath(headerOldPath);
                                const fs::path newSourcePath = makeNewPath(sourceOldPath);

                                const auto hasConflict = [&](const fs::path& candidate)
                                {
                                    if (candidate.empty())
                                        return false;

                                    std::error_code existsError;
                                    const bool exists = fs::exists(candidate, existsError);
                                    if (existsError)
                                    {
                                        String errorMessage = "Unable to verify entry during rename: ";
                                        errorMessage += existsError.message();
                                        LogAndStoreError(requests.renameError, std::move(errorMessage));
                                        return true;
                                    }

                                    if (exists)
                                    {
                                        LogAndStoreError(requests.renameError, "An entry with this name already exists.", false);
                                        return true;
                                    }

                                    return false;
                                };

                                if (hasConflict(newHeaderPath) || hasConflict(newSourcePath))
                                {
                                    // Conflict handled inside hasConflict.
                                }
                                else
                                {
                                    std::vector<std::pair<fs::path, fs::path>> renamePairs{};
                                    if (!headerOldPath.empty())
                                        renamePairs.emplace_back(headerOldPath, newHeaderPath);
                                    if (!sourceOldPath.empty())
                                        renamePairs.emplace_back(sourceOldPath, newSourcePath);

                                    std::vector<std::pair<fs::path, fs::path>> completedRenames{};
                                    bool renameFailed = false;

                                    for (const auto& pair : renamePairs)
                                    {
                                        std::error_code renameError;
                                        fs::rename(pair.first, pair.second, renameError);
                                        if (renameError)
                                        {
                                            String errorMessage = "Unable to rename entry: ";
                                            errorMessage += renameError.message();
                                            LogAndStoreError(requests.renameError, std::move(errorMessage));
                                            renameFailed = true;
                                            break;
                                        }

                                        completedRenames.push_back(pair);
                                    }

                                    if (renameFailed)
                                    {
                                        for (auto it = completedRenames.rbegin(); it != completedRenames.rend(); ++it)
                                        {
                                            std::error_code revertError;
                                            fs::rename(it->second, it->first, revertError);
                                            if (revertError)
                                            {
                                                String revertMessage = "Unable to restore entry after rename failure: ";
                                                revertMessage += revertError.message();
                                                LOG_ERROR(revertMessage);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        fs::path selectionPath = parent / newBaseName;
                                        selectedEntry = selectionPath.generic_string();
                                        requests.renameError.Clear();
                                        requests.renameTarget = newHeaderPath;
                                        requests.renameSecondaryTarget = newSourcePath;
                                        requests.renameTargetIsScriptGroup = false;
                                        state.cache.dirty = true;
                                        ImGui::CloseCurrentPopup();
                                    }
                                }
                            }
                        }
                    }
                }
                else // <-- ce else s’applique si renamingScriptGroup == false
                {
                    const fs::path oldPath = requests.renameTarget;
                    const String oldName = oldPath.filename().generic_string();

                    if (oldName == newName)
                    {
                        requests.renameError.Clear();
                        ImGui::CloseCurrentPopup();
                    }
                    else
                    {
                        const fs::path parent = oldPath.parent_path();
                        const fs::path newPath = parent / newName.View();

                        if (fs::exists(newPath))
                        {
                            LogAndStoreError(requests.renameError, "An entry with this name already exists.", false);
                        }
                        else
                        {
                            if (!TryRename(oldPath, newPath, requests.renameError))
                            {
                                // Error already stored.
                            }
                            else
                            {
                                if (selectedEntry == oldPath.generic_string())
                                    selectedEntry = newPath.generic_string();

                                requests.renameTarget = newPath;
                                requests.renameError.Clear();
                                state.cache.dirty = true;
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                }
            }
        }
    }

    void RunBixHeaderTool(const std::filesystem::path& toolPath, const std::filesystem::path& headerPath)
    {
#if defined(_WIN32)
        std::wstring tool = L"\"" + toolPath.wstring() + L"\"";
        std::wstring header = L"\"" + headerPath.wstring() + L"\"";
        std::wstring cmdLine = tool + L" --single " + header;

        LOG_INFO("Launching BixHeaderTool: " + String(std::string(cmdLine.begin(), cmdLine.end()).c_str()));

        STARTUPINFOW si{};
        PROCESS_INFORMATION pi{};
        si.cb = sizeof(si);

        if (!CreateProcessW(
            nullptr,
            cmdLine.data(),
            nullptr, nullptr, FALSE, 0,
            nullptr, nullptr,
            &si, &pi))
        {
            LOG_ERROR("Failed to launch BixHeaderTool.exe");
            return;
        }

        // Attend la fin du processus
        WaitForSingleObject(pi.hProcess, INFINITE);

        DWORD exitCode = 0;
        GetExitCodeProcess(pi.hProcess, &exitCode);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (exitCode != 0)
            LOG_WARNING("BixHeaderTool exited with code " + String::FromInt(exitCode));
        else
            LOG_INFO("BixHeaderTool finished successfully.");
#else
        (void)toolPath;
        (void)headerPath;
        LOG_WARNING("BixHeaderTool execution is only supported on Windows platforms.");
#endif
    }

    void RenderPopups(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups)
    {
        EnsureScriptsDirectoryExists(state);
        RenderCreateScriptPopup(state, selectedEntry, requestPopups);
        RenderCreatePrefabPopup(state, requestPopups);
        RenderCreateFolderPopup(state, selectedEntry, requestPopups);
        RenderCreateSpriteAtlasPopup(state, selectedEntry, requestPopups);
        RenderRenameEntryPopup(state, selectedEntry, requestPopups);
    }
}
