#include "ContentBrowserPanelInternal.h"

#include "imgui.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

namespace BixEngine::Gui
{
    namespace
    {
        constexpr const char* kScriptExtension = ".lua";

        const char* GetScriptTypeLabel(ScriptTemplateType type)
        {
            switch (type)
            {
            case ScriptTemplateType::Actor:
                return "Actor Script";
            case ScriptTemplateType::Component:
                return "Component Script";
            case ScriptTemplateType::Utility:
            default:
                return "Utility Script";
            }
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
                if (path.extension() != kScriptExtension)
                    continue;

                requests.existingScripts.emplace_back(path.filename().generic_string());
            }

            if (iterationError)
                return;

            std::sort(requests.existingScripts.begin(), requests.existingScripts.end(), [](const String& lhs, const String& rhs)
            {
                return CaseInsensitiveLess(lhs, rhs);
            });
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

            ImGui::TextUnformatted("Create a new Lua script in the current directory.");

            if (ImGui::IsWindowAppearing())
                ImGui::SetKeyboardFocusHere();

            ImGui::TextUnformatted("Script name");
            ImGui::SameLine();
            ImGui::TextDisabled("(.lua)");

            bool create = ImGui::InputText("##ScriptName", requests.scriptName, IM_ARRAYSIZE(requests.scriptName), ImGuiInputTextFlags_EnterReturnsTrue);

            const char* scriptTypes[] = {"Actor Script", "Component Script", "Utility Script"};
            int selectedType = static_cast<int>(requests.scriptType);
            ImGui::SetNextItemWidth(200.0f);
            if (ImGui::Combo("Script type", &selectedType, scriptTypes, IM_ARRAYSIZE(scriptTypes)))
                requests.scriptType = static_cast<ScriptTemplateType>(selectedType);

            if (!requests.scriptError.IsEmpty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
                ImGui::TextWrapped("%s", requests.scriptError.c_str());
                ImGui::PopStyleColor();
            }

            if (!requests.existingScripts.empty())
            {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextDisabled("Existing scripts in Content/Scripts:");
                ImGui::Spacing();
                ImGui::BeginChild("ExistingScriptsList", ImVec2(320.0f, 120.0f), true);
                for (const auto& script : requests.existingScripts)
                    ImGui::BulletText("%s", script.c_str());
                ImGui::EndChild();
            }
            else
            {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::TextDisabled("No scripts found in Content/Scripts.");
            }

            if (ImGui::Button("Create"))
                create = true;

            ImGui::SameLine();
            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
                requests.scriptError.Clear();
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
                    const String::size_type nameLength = rawInput.size();
                    if (nameLength > 4 && rawInput.View().substr(nameLength - 4) == kScriptExtension)
                        rawInput = rawInput.Mid(0, nameLength - 4);

                    std::string fileName(rawInput.View());
                    fileName += kScriptExtension;
                    const fs::path scriptFileName(fileName);
                    const fs::path scriptPath = state.current / scriptFileName;

                    if (fs::exists(scriptPath))
                    {
                        LogAndStoreError(requests.scriptError, "A file with this name already exists.", false);
                    }
                    else
                    {
                        std::error_code createDirectoryError;
                        fs::create_directories(scriptPath.parent_path(), createDirectoryError);
                        if (createDirectoryError)
                        {
                            String message = "Unable to ensure directory exists: ";
                            message += createDirectoryError.message();
                            LogAndStoreError(requests.scriptError, std::move(message));
                        }
                        else
                        {
                            std::ofstream scriptFile(scriptPath);
                            if (!scriptFile.is_open())
                            {
                                LogAndStoreError(requests.scriptError, "Failed to create the script file.");
                            }
                            else
                            {
                                scriptFile << "-- Type: " << GetScriptTypeLabel(requests.scriptType) << '\n';
                                scriptFile << "-- Created automatically from the Content Browser\n\n";
                                scriptFile.close();

                                selectedEntry = scriptPath.generic_string();
                                requests.scriptError.Clear();
                                ImGui::CloseCurrentPopup();
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

            const fs::path target = requests.renameTarget;
            if (!target.empty())
            {
                fs::path relativePath = target.lexically_relative(state.root);
                std::string relativeString = relativePath.generic_string();

                String label = relativeString;
                if (label.IsEmpty() || label == ".")
                    label = target.filename().generic_string();

                String message = "Rename: ";
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
                else if (requests.renameTarget.empty())
                {
                    LogAndStoreError(requests.renameError, "No entry selected for rename.", false);
                }
                else
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

            ImGui::EndPopup();
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
