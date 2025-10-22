#include "Bix/Core/Logger.h"
#include "Bix/Engine/Gui/Utils/GuiHelpers.h"
#include "imgui.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <cctype>
#include "Bix/Engine/Gui/Panels/ContentBrowser/ContentBrowserPanelInternal.h"


namespace BixEngine::Gui
{
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

        struct ParentScriptInfo
        {
            std::string displayName{};
            std::string className{};
            std::string includePath{};

            [[nodiscard]] bool IsValid() const noexcept { return !className.empty(); }
        };

        const std::vector<ParentScriptInfo>& GetBaseClassParents()
        {
            static const std::vector<ParentScriptInfo> baseParents = {
                {"Actor", "BixEngine::Game::Actor", "Bix/Game/Actor.h"},
                {"Component", "BixEngine::Game::Component", "Bix/Game/Components/Component.h"},
            };
            return baseParents;
        }

        std::string TrimWhitespace(std::string value)
        {
            auto isSpace = [](unsigned char ch) { return std::isspace(ch) != 0; };
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char ch) { return !isSpace(ch); }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char ch) { return !isSpace(ch); }).base(), value.end());
            return value;
        }

        struct ScriptNode
        {
            std::string name{};
            std::string parentName{};
            std::string includePath{};
            std::vector<ScriptNode> children{};
        };

        bool CaseInsensitiveLess(const std::string& lhs, const std::string& rhs)
        {
            auto toLower = [](unsigned char ch) { return static_cast<unsigned char>(std::tolower(ch)); };
            return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                [&](unsigned char a, unsigned char b)
                {
                    return toLower(a) < toLower(b);
                });
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

                auto [existing, inserted] = nodes.emplace(node.name, node);
                if (!inserted)
                {
                    if (!node.parentName.empty() && existing->second.parentName.empty())
                        existing->second.parentName = node.parentName;
                    if (!node.includePath.empty())
                        existing->second.includePath = node.includePath;
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
            }
            return info;
        }

        void SetSelectedParent(PopupRequestState& requests, const ParentScriptInfo& info, bool isBaseParent)
        {
            requests.selectedParentClass = info.className;
            requests.selectedParentInclude = info.includePath;
            requests.selectedParentDisplay = info.displayName;
            requests.selectedParentIsBase = isBaseParent;
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
                            ImGui::PushID(static_cast<int>(baseIndex));
                            const auto& baseParent = baseParents[baseIndex];
                            const bool isSelectedBase = requests.selectedParentIsBase && !requests.selectedParentClass.IsEmpty() && requests.selectedParentClass.View() == baseParent.className;
                            if (ImGui::Selectable(baseParent.displayName.c_str(), isSelectedBase))
                            {
                                SetSelectedParent(requests, baseParent, true);
                            }
                            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
                            {
                                ImGui::SetTooltip("%s", baseParent.className.c_str());
                            }
                            ImGui::PopID();
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

                                        headerFile << "#pragma once\n\n";
                                        headerFile << "// Parent: " << (info.IsValid() ? info.className : "(none)") << '\n';
                                        headerFile << "// Created automatically from the Content Browser\n\n";

                                        if (info.IsValid())
                                        {
                                            // Include directive is driven by the selected parent (base class or user script).
                                            headerFile << "#include \"" << info.includePath << "\"\n\n";
                                        }

                                        headerFile << "class " << baseName;
                                        if (info.IsValid())
                                            headerFile << " : public " << info.className;
                                        
                                        headerFile << '\n';
                                        headerFile << "{\n";
                                        headerFile << "public:\n";
                                        
                                        if (info.IsValid())
                                            headerFile << "    using Super = " << info.className << ";\n\n";
                                        
                                        headerFile << "    " << baseName << "();\n";
                                        headerFile << "    void OnCreate();\n";
                                        headerFile << "    void OnUpdate(float deltaTime);\n";
                                        headerFile << "};\n\n";

                                        sourceFile << "#include \"" << baseName << kScriptHeaderExtension << "\"\n\n";
                                        sourceFile << baseName << "::" << baseName << "() = default;\n\n";
                                        sourceFile << "void " << baseName << "::OnCreate()\n";
                                        sourceFile << "{\n";
                                        
                                        if (info.IsValid())
                                            sourceFile << "    // Super::OnCreate();\n";
                                        
                                        sourceFile << "}\n\n";
                                        sourceFile << "void " << baseName << "::OnUpdate(float deltaTime)\n";
                                        sourceFile << "{\n";
                                        
                                        if (info.IsValid())
                                            sourceFile << "    // Super::OnUpdate(deltaTime);\n";
                                        
                                        sourceFile << "}\n\n";

                                        selectedEntry = headerPath.generic_string();
                                        requests.scriptError.Clear();
                                        ClearSelectedParent(requests);
                                        ImGui::CloseCurrentPopup();
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
                    else
                    {
                        std::error_code createError;
                        fs::create_directories(folderPath, createError);
                        if (createError)
                        {
                            String message = "Unable to create folder: ";
                            message += createError.message();
                            LogAndStoreError(requests.folderError, std::move(message));
                        }
                        else
                        {
                            selectedEntry = folderPath.generic_string();
                            requests.folderError.Clear();
                            requests.folderTarget.clear();
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
                Utils::DrawDescriptionText(message.c_str());
            }
            else
            {
                Utils::DrawDescriptionText("Rename the selected entry.");
            }

            const bool shouldAutofocus = ImGui::IsWindowAppearing();
            bool rename = Utils::InputTextWithLabel("New name", requests.renameBuffer, IM_ARRAYSIZE(requests.renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue, shouldAutofocus);

            if (!requests.renameError.IsEmpty())
            {
                Utils::DrawErrorMessage(std::string(requests.renameError.View()));
            }

            const bool confirmPressed = Utils::DrawConfirmButtons("Rename", "Cancel",
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
                            std::error_code renameError;
                            fs::rename(oldPath, newPath, renameError);

                            if (renameError)
                            {
                                String errorMessage = "Unable to rename entry: ";
                                errorMessage += renameError.message();
                                LogAndStoreError(requests.renameError, std::move(errorMessage));
                            }
                            else
                            {
                                if (selectedEntry == oldPath.generic_string())
                                    selectedEntry = newPath.generic_string();

                                requests.renameTarget = newPath;
                                requests.renameError.Clear();
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                }
            }
        }
    }

    void RenderPopups(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups)
    {
        EnsureScriptsDirectoryExists(state);
        RenderCreateScriptPopup(state, selectedEntry, requestPopups);
        RenderCreateFolderPopup(state, selectedEntry, requestPopups);
        RenderRenameEntryPopup(state, selectedEntry, requestPopups);
    }
}
