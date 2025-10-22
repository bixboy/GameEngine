#include "Bix/Core/Logger.h"
#include "imgui.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
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

        void RefreshExistingScripts(PopupRequestState& requests, const std::filesystem::path& scriptsDirectory)
        {
            namespace fs = std::filesystem;

            requests.existingScripts.clear();

            std::error_code existsError;
            if (!fs::exists(scriptsDirectory, existsError) || existsError)
                return;

            std::error_code iterationError;
            for (const auto& entry : fs::directory_iterator(scriptsDirectory, iterationError))
            {
                if (!entry.is_regular_file())
                    continue;

                const fs::path& path = entry.path();
                if (path.extension() != kScriptHeaderExtension)
                    continue;

                requests.existingScripts.emplace_back(path.stem().generic_string());
            }

            if (iterationError)
                return;

            std::sort(requests.existingScripts.begin(), requests.existingScripts.end(), [](const String& lhs, const String& rhs)
            {
                return CaseInsensitiveLess(lhs, rhs);
            });

            requests.existingScripts.erase(std::unique(requests.existingScripts.begin(), requests.existingScripts.end(), [](const String& lhs, const String& rhs)
            {
                return ToLowerCopy(lhs) == ToLowerCopy(rhs);
            }), requests.existingScripts.end());
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
            RefreshExistingScripts(requests, scriptsDirectory);

            const auto& baseParents = GetBaseClassParents();

            std::vector<ParentScriptInfo> userParents;
            userParents.reserve(requests.existingScripts.size());
            for (const auto& script : requests.existingScripts)
            {
                ParentScriptInfo info{};
                info.displayName = script.View();
                info.className = info.displayName;
                info.includePath = info.className + kScriptHeaderExtension;
                userParents.emplace_back(std::move(info));
            }

            const int totalParentOptions = static_cast<int>(baseParents.size() + userParents.size());
            if (requests.selectedParentScript >= totalParentOptions)
                requests.selectedParentScript = -1;

            auto resolveSelectedParent = [&](int selection) -> ParentScriptInfo
            {
                if (selection < 0)
                    return {};

                const int baseCount = static_cast<int>(baseParents.size());
                if (selection < baseCount)
                    return baseParents[selection];

                const int userIndex = selection - baseCount;
                if (userIndex >= 0 && userIndex < static_cast<int>(userParents.size()))
                    return userParents[userIndex];

                return {};
            };

            ImGui::TextUnformatted("Create a new C++ script in the current directory.");

            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();

            ImGui::TextUnformatted("Script name");
            ImGui::SameLine();
            ImGui::TextDisabled("(.h / .cpp)");

            bool create = ImGui::InputText("##ScriptName", requests.scriptName, IM_ARRAYSIZE(requests.scriptName), ImGuiInputTextFlags_EnterReturnsTrue);

            if (!requests.scriptError.IsEmpty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
                ImGui::TextWrapped("%s", requests.scriptError.c_str());
                ImGui::PopStyleColor();
            }

            std::string parentPreview = "None";
            if (requests.selectedParentScript >= 0)
            {
                const ParentScriptInfo previewInfo = resolveSelectedParent(requests.selectedParentScript);
                if (previewInfo.IsValid())
                {
                    parentPreview = previewInfo.displayName;
                    if (requests.selectedParentScript >= static_cast<int>(baseParents.size()))
                        parentPreview += kScriptHeaderExtension;
                }
                else
                {
                    parentPreview = "None";
                    requests.selectedParentScript = -1;
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextDisabled("Parent (optional)");
            ImGui::SetNextItemWidth(240.0f);
            if (ImGui::BeginCombo("##ParentScriptSelection", parentPreview.c_str()))
            {
                const bool noneSelected = requests.selectedParentScript == -1;
                if (ImGui::Selectable("None", noneSelected))
                    requests.selectedParentScript = -1;
                if (noneSelected)
                    ImGui::SetItemDefaultFocus();

                if (!baseParents.empty())
                {
                    // Base Classes section mirrors Unreal Engine's layout.
                    ImGui::Separator();
                    ImGui::TextDisabled("Base Classes");
                    for (int i = 0; i < static_cast<int>(baseParents.size()); ++i)
                    {
                        const bool isSelected = requests.selectedParentScript == i;
                        if (ImGui::Selectable(baseParents[i].displayName.c_str(), isSelected))
                            requests.selectedParentScript = i;
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                }

                if (!userParents.empty())
                {
                    // User Scripts section lists headers discovered in Content/Scripts.
                    ImGui::Separator();
                    ImGui::TextDisabled("User Scripts");
                    for (int i = 0; i < static_cast<int>(userParents.size()); ++i)
                    {
                        const int globalIndex = static_cast<int>(baseParents.size()) + i;
                        const bool isSelected = requests.selectedParentScript == globalIndex;
                        std::string label = userParents[i].displayName + kScriptHeaderExtension;
                        if (ImGui::Selectable(label.c_str(), isSelected))
                            requests.selectedParentScript = globalIndex;
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                }

                ImGui::EndCombo();
            }
            ImGui::Spacing();
            ImGui::Separator();

            if (!requests.existingScripts.empty())
            {
                ImGui::TextDisabled("Existing scripts in Content/Scripts:");
                ImGui::Spacing();
                ImGui::BeginChild("ExistingScriptsList", ImVec2(320.0f, 120.0f), true);
                for (const auto& script : requests.existingScripts)
                    ImGui::BulletText("%s.h", script.c_str());
                ImGui::EndChild();
            }
            else
            {
                ImGui::TextDisabled("No scripts found in Content/Scripts.");
            }

            if (ImGui::Button("Create"))
                create = true;

            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
                requests.scriptError.Clear();
                requests.selectedParentScript = -1;
            }

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
                                        // Resolve the inheritance information from the current selection.
                                        const ParentScriptInfo parentInfo = resolveSelectedParent(requests.selectedParentScript);

                                        headerFile << "#pragma once\n\n";
                                        headerFile << "// Parent: " << (parentInfo.IsValid() ? parentInfo.className : "(none)") << '\n';
                                        headerFile << "// Created automatically from the Content Browser\n\n";
                                        if (parentInfo.IsValid())
                                        {
                                            // Include directive is driven by the selected parent (base class or user script).
                                            headerFile << "#include \"" << parentInfo.includePath << "\"\n\n";
                                        }
                                        headerFile << "class " << baseName;
                                        if (parentInfo.IsValid())
                                            headerFile << " : public " << parentInfo.className;
                                        headerFile << '\n';
                                        headerFile << "{\n";
                                        headerFile << "public:\n";
                                        if (parentInfo.IsValid())
                                            headerFile << "    using Super = " << parentInfo.className << ";\n\n";
                                        headerFile << "    " << baseName << "();\n";
                                        headerFile << "    void OnCreate();\n";
                                        headerFile << "    void OnUpdate(float deltaTime);\n";
                                        headerFile << "};\n";

                                        sourceFile << "#include \"" << baseName << kScriptHeaderExtension << "\"\n\n";
                                        sourceFile << baseName << "::" << baseName << "() = default;\n\n";
                                        sourceFile << "void " << baseName << "::OnCreate()\n";
                                        sourceFile << "{\n";
                                        if (parentInfo.IsValid())
                                            sourceFile << "    // Super::OnCreate();\n";
                                        sourceFile << "}\n\n";
                                        sourceFile << "void " << baseName << "::OnUpdate(float deltaTime)\n";
                                        sourceFile << "{\n";
                                        if (parentInfo.IsValid())
                                            sourceFile << "    // Super::OnUpdate(deltaTime);\n";
                                        sourceFile << "}\n";

                                        selectedEntry = headerPath.generic_string();
                                        requests.scriptError.Clear();
                                        requests.selectedParentScript = -1;
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
            ImGui::TextWrapped("%s", description.c_str());

            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();

            bool create = ImGui::InputText("Name", requests.folderName, IM_ARRAYSIZE(requests.folderName), ImGuiInputTextFlags_EnterReturnsTrue);

            if (!requests.folderError.IsEmpty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
                ImGui::TextWrapped("%s", requests.folderError.c_str());
                ImGui::PopStyleColor();
            }

            if (ImGui::Button("Create"))
                create = true;

            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
                requests.folderError.Clear();
                requests.folderTarget.clear();
            }

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
                ImGui::TextWrapped("%s", message.c_str());
            }
            else
            {
                ImGui::TextUnformatted("Rename the selected entry.");
            }

            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();

            bool rename = ImGui::InputText("Name", requests.renameBuffer, IM_ARRAYSIZE(requests.renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);

            if (!requests.renameError.IsEmpty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
                ImGui::TextWrapped("%s", requests.renameError.c_str());
                ImGui::PopStyleColor();
            }

            if (ImGui::Button("Rename"))
                rename = true;

            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
                requests.renameError.Clear();
                requests.renameTarget.clear();
                requests.renameSecondaryTarget.clear();
                requests.renameTargetIsScriptGroup = false;
            }

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
