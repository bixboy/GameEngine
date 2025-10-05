#include "Gui/Panels/ContentBrowserPanel.h"

#include "Core/Logger.h"
#include "Core/String.h"
#include "Gui/GuiManager.h"
#include "Gui/GuiPanel.h"
#include "Gui/Widgets/ActionTooltip.h"

#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace Engine::Gui
{
    namespace
    {
        constexpr ImVec4 kContentBackground{0.09f, 0.09f, 0.09f, 0.95f};
        constexpr ImVec4 kContentTreeBackground{0.13f, 0.13f, 0.13f, 0.95f};
        constexpr ImVec4 kContentHeaderBackground{0.16f, 0.16f, 0.16f, 1.0f};
        constexpr float kContentTreeWidth = 240.0f;
        constexpr float kContentHeaderHeight = 72.0f;
        constexpr float kContentThumbnailSize = 72.0f;
        constexpr float kContentThumbnailPadding = 28.0f;
        
    #ifdef ImGuiHoveredFlags_ForTooltip
        constexpr ImGuiHoveredFlags kEntryTooltipHoverFlags = ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_ForTooltip;
    #else
        constexpr ImGuiHoveredFlags kEntryTooltipHoverFlags = ImGuiHoveredFlags_DelayNormal;
    #endif

        struct ContentState
        {
            std::filesystem::path root{};
            std::filesystem::path current{};
            String error{};
            bool initialized{false};
        };

        struct PopupRequestState
        {
            bool createScript{false};
            bool createFolder{false};
            bool renameEntry{false};
            char scriptName[128] = "NewScript.lua";
            char folderName[128] = "NewFolder";
            char renameBuffer[256] = "";
            String scriptError{};
            String folderError{};
            String renameError{};
            std::filesystem::path folderTarget{};
            std::filesystem::path renameTarget{};
        };

        void LogAndStoreError(String& storage, String message, bool log = true)
        {
            if (log)
                LOG_ERROR(message);

            storage = std::move(message);
        }

        String ToLowerCopy(const String& value)
        {
            String result(value);
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });

            return result;
        }

        bool CaseInsensitiveLess(const String& lhs, const String& rhs)
        {
            const String lhsLower = ToLowerCopy(lhs);
            const String rhsLower = ToLowerCopy(rhs);
            if (lhsLower == rhsLower)
                return lhs.View() < rhs.View();

            return lhsLower.View() < rhsLower.View();
        }

        String TrimCopy(String value)
        {
            const auto isSpace = [](unsigned char ch)
            {
                return std::isspace(ch) != 0;
            };

            String::size_type start = 0;
            const String::size_type length = value.size();

            while (start < length && isSpace(static_cast<unsigned char>(value[start])))
                ++start;

            String::size_type end = length;
            while (end > start && isSpace(static_cast<unsigned char>(value[end - 1])))
                --end;

            return value.Mid(start, end - start);
        }

        bool ContainsPathSeparator(const String& value)
        {
            const std::string_view view = value.View();
            return view.find('/') != std::string::npos || view.find('\\') != std::string::npos;
        }

        void ShowPathInExplorer(const std::filesystem::path& path, bool isDirectory)
        {
            namespace fs = std::filesystem;

            if (path.empty())
                return;

#ifdef _WIN32
            std::string command = "explorer ";
            if (isDirectory)
            {
                command += '"';
                command += path.string();
                command += '"';
            }
            else
            {
                command += "/select,\"";
                command += path.string();
                command += "\"";
            }
            std::system(command.c_str());
#elif defined(__APPLE__)
            const fs::path target = isDirectory ? path : path.parent_path();
            if (target.empty())
                return;

            std::string command = "open \"";
            command += target.string();
            command += "\"";
            std::system(command.c_str());
#else
            const fs::path target = isDirectory ? path : path.parent_path();
            if (target.empty())
                return;

            std::string command = "xdg-open \"";
            command += target.string();
            command += "\"";
            std::system(command.c_str());
#endif
        }

        void EnsureInitialized(ContentState& state)
        {
            if (state.initialized)
                return;

            namespace fs = std::filesystem;

            std::error_code cwdError;
            const fs::path basePath = fs::current_path(cwdError);
            if (cwdError)
            {
                const String errorText = cwdError.message();
                String message = String("Failed to determine working directory: ");
                message += errorText;
                LogAndStoreError(state.error, std::move(message));
            }
            else
            {
                state.root = basePath / "Content";
                std::error_code createError;
                fs::create_directories(state.root, createError);
                if (createError)
                {
                    String message = String("Failed to create content directory: ") + state.root.string();
                    message += " (";
                    const String errorText = createError.message();
                    message += errorText;
                    message += ')';
                    LogAndStoreError(state.error, std::move(message));
                }
                else if (!fs::exists(state.root))
                {
                    String message = String("Content directory is not available: ") + state.root.string();
                    LogAndStoreError(state.error, std::move(message));
                }
                else
                {
                    state.current = state.root;
                }
            }

            state.initialized = true;
        }

        void RenderHeader(ContentState& state, String& selectedEntry, char (&searchBuffer)[256])
        {
            namespace fs = std::filesystem;

            const fs::path relativePath = state.current.lexically_relative(state.root);
            const String relativeString = relativePath.generic_string();
            const bool atRoot = relativeString.IsEmpty() || relativeString == ".";

            ImGui::PushStyleColor(ImGuiCol_ChildBg, kContentHeaderBackground);
            if (ImGui::BeginChild("ContentBrowserHeader", ImVec2(0.0f, kContentHeaderHeight), true, ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));
                if (ImGui::Button("Content"))
                {
                    state.current = state.root;
                    selectedEntry.Clear();
                }

                ImGui::PopStyleVar();
                ImGui::SameLine();
                ImGui::BeginDisabled(atRoot);

                if (ImGui::Button("Up"))
                {
                    fs::path parent = state.current.parent_path();
                    fs::path parentRelative = parent.lexically_relative(state.root);
                    const String parentString = parentRelative.generic_string();

                    if (parentString.IsEmpty() || parentString == "." || parentString.View().rfind("..", 0) == 0)
                    {
                        state.current = state.root;
                    }
                    else
                    {
                        state.current = parent;
                    }

                    selectedEntry.Clear();
                }

                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::TextDisabled("%s", atRoot ? "Content" : relativeString.c_str());
                ImGui::SameLine();
                ImGui::SetNextItemWidth(220.0f);
                ImGui::InputTextWithHint("##ContentSearch", "Search content...", searchBuffer, IM_ARRAYSIZE(searchBuffer));
                ImGui::Spacing();
                ImGui::TextDisabled("Clique droit dans la zone de contenu pour créer des scripts ou des dossiers.");
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        void RenderDirectoryTree(ContentState& state, String& selectedEntry)
        {
            namespace fs = std::filesystem;

            ImGui::BeginChild("ContentBrowserTree", ImVec2(kContentTreeWidth, 0.0f), true);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, kContentTreeBackground);

            if (ImGui::BeginChild("ContentBrowserTreeInner", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
            {
                const auto renderDirectoryTree = [&](auto&& self, const fs::path& directory, int depth) -> void
                {
                    const String directoryName = directory == state.root ? String("Content") : String(directory.filename().generic_string());
                    const String directoryId = directory.generic_string();
                    std::error_code equivalentError;
                    const bool isSelected = fs::equivalent(directory, state.current, equivalentError);
                    const ImGuiTreeNodeFlags nodeFlags =
                        ImGuiTreeNodeFlags_OpenOnArrow |
                        ImGuiTreeNodeFlags_OpenOnDoubleClick |
                        ImGuiTreeNodeFlags_SpanFullWidth |
                        (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

                    const bool open = ImGui::TreeNodeEx(directoryId.c_str(), nodeFlags, "%s", directoryName.c_str());
                    if (ImGui::IsItemClicked())
                    {
                        state.current = directory;
                        selectedEntry.Clear();
                    }

                    if (open)
                    {
                        std::vector<fs::path> children{};
                        std::error_code childError;
                        for (const auto& entry : fs::directory_iterator(directory, childError))
                        {
                            if (!entry.is_directory())
                                continue;

                            children.push_back(entry.path());
                        }

                        if (childError)
                        {
                            ImGui::TextDisabled("Unable to open directory.");
                        }
                        else
                        {
                            std::sort(children.begin(), children.end(), [&](const fs::path& lhs, const fs::path& rhs)
                            {
                                const String lhsName = lhs.filename().generic_string();
                                const String rhsName = rhs.filename().generic_string();
                                return CaseInsensitiveLess(lhsName, rhsName);
                            });

                            for (const auto& child : children)
                            {
                                const int nextDepth = depth + 1;
                                if (nextDepth > 64)
                                {
                                    ImGui::TextDisabled("...");
                                    break;
                                }

                                self(self, child, nextDepth);
                            }
                        }

                        ImGui::TreePop();
                    }
                };

                if (fs::exists(state.root))
                {
                    renderDirectoryTree(renderDirectoryTree, state.root, 0);
                }
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::EndChild();
        }

        bool MatchesSearch(const String& value, const String& query)
        {
            if (query.IsEmpty())
                return true;

            const String valueLower = ToLowerCopy(value);
            const String queryLower = ToLowerCopy(query);
            return valueLower.find(queryLower.View()) != std::string::npos;
        }

        void RenderEntries(ContentState& state, String& selectedEntry, PopupRequestState& requestPopups, const String& searchQuery)
        {
            namespace fs = std::filesystem;

            ImGui::BeginChild("ContentBrowserGrid", ImVec2(0.0f, 0.0f), true);

            if (ImGui::BeginPopupContextWindow("ContentBrowserBackgroundContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                if (ImGui::MenuItem("Créer un script..."))
                {
                    std::snprintf(requestPopups.scriptName, IM_ARRAYSIZE(requestPopups.scriptName), "%s", "NewScript.lua");
                    requestPopups.scriptError.Clear();
                    requestPopups.createScript = true;
                }

                if (ImGui::MenuItem("Créer un dossier..."))
                {
                    std::snprintf(requestPopups.folderName, IM_ARRAYSIZE(requestPopups.folderName), "%s", "NewFolder");
                    requestPopups.folderError.Clear();
                    requestPopups.folderTarget = state.current;
                    requestPopups.createFolder = true;
                }

                ImGui::EndPopup();
            }

            std::vector<fs::directory_entry> entries{};
            std::error_code iterationError;
            for (const auto& entry : fs::directory_iterator(state.current, iterationError))
            {
                entries.emplace_back(entry);
            }

            if (iterationError)
            {
                const String errorText = iterationError.message();
                String displayMessage{};
                LogAndStoreError(displayMessage, String("Failed to enumerate content: ") + errorText);
                ImGui::TextDisabled("%s", displayMessage.c_str());
                ImGui::EndChild();
                return;
            }

            std::sort(entries.begin(), entries.end(), [&](const fs::directory_entry& lhs, const fs::directory_entry& rhs)
            {
                if (lhs.is_directory() && !rhs.is_directory())
                    return true;

                if (!lhs.is_directory() && rhs.is_directory())
                    return false;

                const String lhsName = lhs.path().filename().generic_string();
                const String rhsName = rhs.path().filename().generic_string();
                return CaseInsensitiveLess(lhsName, rhsName);
            });

            const float cellSize = kContentThumbnailSize + kContentThumbnailPadding;
            const float panelWidth = ImGui::GetContentRegionAvail().x;

            int columns = static_cast<int>(panelWidth / cellSize);
            columns = std::max(columns, 1);

            if (ImGui::BeginTable("ContentBrowserEntries", columns))
            {
                ImGui::TableSetupScrollFreeze(0, 1);

                for (const auto& entry : entries)
                {
                    const fs::path entryPath = entry.path();
                    const String entryName = entryPath.filename().generic_string();
                    if (!MatchesSearch(entryName, searchQuery))
                        continue;

                    const String entryPathString = entryPath.generic_string();
                    const bool isDirectory = entry.is_directory();

                    ImGui::TableNextColumn();
                    ImGui::PushID(entryName.c_str());
                    ImGui::BeginGroup();

                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 12.0f));
                    const ImVec2 buttonSize(kContentThumbnailSize, kContentThumbnailSize);
                    const bool clicked = ImGui::Button(isDirectory ? "\xef\x81\xbb" : "\xef\x81\x96", buttonSize);
                    ImGui::PopStyleVar();

                    if (ImGui::BeginPopupContextItem("ContentBrowserEntryContext"))
                    {
                        ImGui::TextDisabled("Actions");
                        ImGui::Separator();

                        if (ImGui::MenuItem("Ouvrir"))
                        {
                            if (isDirectory)
                            {
                                state.current = entryPath;
                                selectedEntry.Clear();
                            }
                            else
                            {
                                selectedEntry = entryPathString;
                            }
                        }

                        if (ImGui::MenuItem("Renommer..."))
                        {
                            std::snprintf(requestPopups.renameBuffer, IM_ARRAYSIZE(requestPopups.renameBuffer), "%s", entryName.c_str());
                            requestPopups.renameError.Clear();
                            requestPopups.renameTarget = entryPath;
                            requestPopups.renameEntry = true;
                        }

                        if (ImGui::MenuItem("Supprimer"))
                        {
                            std::error_code removeError;
                            if (isDirectory)
                                std::filesystem::remove_all(entryPath, removeError);
                            else
                                std::filesystem::remove(entryPath, removeError);

                            if (removeError)
                            {
                                const String errorText = removeError.message();
                                String message = "Impossible de supprimer l'entrée : ";
                                message += errorText;
                                LOG_ERROR(message);
                            }
                            else
                            {
                                if (selectedEntry == entryPathString)
                                    selectedEntry.Clear();
                            }
                        }

                        ImGui::Separator();
                        ImGui::TextDisabled("Utilitaires");
                        ImGui::Separator();

                        if (ImGui::MenuItem("Afficher dans l'explorateur"))
                        {
                            ShowPathInExplorer(entryPath, isDirectory);
                        }

                        if (ImGui::MenuItem("Nouveau dossier..."))
                        {
                            std::snprintf(requestPopups.folderName, IM_ARRAYSIZE(requestPopups.folderName), "%s", "NewFolder");
                            requestPopups.folderError.Clear();
                            if (isDirectory)
                            {
                                requestPopups.folderTarget = entryPath;
                            }
                            else
                            {
                                std::filesystem::path parent = entryPath.parent_path();
                                if (parent.empty())
                                    parent = state.current;
                                requestPopups.folderTarget = std::move(parent);
                            }
                            requestPopups.createFolder = true;
                        }

                        ImGui::EndPopup();
                    }

                    if (clicked && isDirectory)
                    {
                        state.current = entryPath;
                        selectedEntry.Clear();
                    }
                    else if (clicked)
                    {
                        selectedEntry = entryPathString;
                    }

                    if (ImGui::IsItemHovered(kEntryTooltipHoverFlags))
                    {
                        if (isDirectory)
                        {
                            ShowActionTooltip(entryName, {
                                {"Ouvrir", [&, entryPath]()
                                {
                                    state.current = entryPath;
                                    selectedEntry.Clear();
                                }},
                                {"Créer un script...", [&]()
                                {
                                    std::snprintf(requestPopups.scriptName, IM_ARRAYSIZE(requestPopups.scriptName), "%s", "NewScript.lua");
                                    requestPopups.scriptError.Clear();
                                    requestPopups.createScript = true;
                                }},
                                {"Créer un dossier...", [&]()
                                {
                                    std::snprintf(requestPopups.folderName, IM_ARRAYSIZE(requestPopups.folderName), "%s", "NewFolder");
                                    requestPopups.folderError.Clear();
                                    requestPopups.folderTarget = entryPath;
                                    requestPopups.createFolder = true;
                                }}
                            });
                        }
                        else
                        {
                            ShowActionTooltip(entryName, {
                                {"Ouvrir", [&, entryPathString]()
                                {
                                    selectedEntry = entryPathString;
                                }}
                            });
                        }
                    }

                    const bool isSelected = selectedEntry == entryPathString;
                    if (isSelected)
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.85f, 0.4f, 1.0f));

                    ImGui::TextWrapped("%s", entryName.c_str());
                    if (isSelected)
                        ImGui::PopStyleColor();

                    ImGui::EndGroup();
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
            else
            {
                ImGui::TextDisabled("Nothing to show.");
            }

            ImGui::EndChild();
        }

        void RenderCreateScriptPopup(ContentState& state, String& selectedEntry, PopupRequestState& requestPopups)
        {
            namespace fs = std::filesystem;

            if (requestPopups.createScript)
            {
                ImGui::OpenPopup("ContentBrowserCreateScript");
                requestPopups.createScript = false;
            }

            if (ImGui::BeginPopupModal("ContentBrowserCreateScript", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextUnformatted("Créer un nouveau script dans le dossier courant.");

                if (ImGui::IsWindowAppearing())
                    ImGui::SetKeyboardFocusHere();

                bool create = ImGui::InputText("Nom", requestPopups.scriptName, IM_ARRAYSIZE(requestPopups.scriptName), ImGuiInputTextFlags_EnterReturnsTrue);

                if (!requestPopups.scriptError.IsEmpty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
                    ImGui::TextWrapped("%s", requestPopups.scriptError.c_str());
                    ImGui::PopStyleColor();
                }

                if (ImGui::Button("Créer"))
                    create = true;

                ImGui::SameLine();

                if (ImGui::Button("Annuler"))
                {
                    ImGui::CloseCurrentPopup();
                    requestPopups.scriptError.Clear();
                }

                if (create)
                {
                    String scriptName = TrimCopy(String(requestPopups.scriptName));

                    if (scriptName.IsEmpty())
                    {
                        LogAndStoreError(requestPopups.scriptError, "Le nom du script ne peut pas être vide.", false);
                    }
                    else
                    {
                        fs::path scriptFileName = scriptName.View();
                        if (scriptFileName.extension().empty())
                            scriptFileName.replace_extension(".lua");

                        const fs::path scriptPath = state.current / scriptFileName;

                        if (fs::exists(scriptPath))
                        {
                            LogAndStoreError(requestPopups.scriptError, "Un fichier avec ce nom existe déjà.", false);
                        }
                        else
                        {
                            std::ofstream scriptFile(scriptPath);
                            if (!scriptFile.is_open())
                            {
                                LogAndStoreError(requestPopups.scriptError, "Impossible de créer le script.");
                            }
                            else
                            {
                                scriptFile << "// Nouveau script généré depuis le Content Browser\n";
                                scriptFile.close();

                                selectedEntry = scriptPath.generic_string();
                                requestPopups.scriptError.Clear();
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                }

                ImGui::EndPopup();
            }
        }

        void RenderCreateFolderPopup(ContentState& state, String& selectedEntry, PopupRequestState& requestPopups)
        {
            namespace fs = std::filesystem;

            if (requestPopups.createFolder)
            {
                ImGui::OpenPopup("ContentBrowserCreateFolder");
                requestPopups.createFolder = false;
            }

            if (ImGui::BeginPopupModal("ContentBrowserCreateFolder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                const fs::path targetDirectory = requestPopups.folderTarget.empty() ? state.current : requestPopups.folderTarget;
                fs::path relativeTarget = targetDirectory.lexically_relative(state.root);
                std::string relativeString = relativeTarget.generic_string();

                String displayLabel = "Content";
                if (!relativeString.empty() && relativeString != ".")
                {
                    displayLabel += '/';
                    displayLabel += relativeString;
                }

                String description = "Créer un nouveau dossier dans : ";
                description += displayLabel;
                ImGui::TextWrapped("%s", description.c_str());

                if (ImGui::IsWindowAppearing())
                    ImGui::SetKeyboardFocusHere();

                bool create = ImGui::InputText("Nom", requestPopups.folderName, IM_ARRAYSIZE(requestPopups.folderName), ImGuiInputTextFlags_EnterReturnsTrue);

                if (!requestPopups.folderError.IsEmpty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
                    ImGui::TextWrapped("%s", requestPopups.folderError.c_str());
                    ImGui::PopStyleColor();
                }

                if (ImGui::Button("Créer"))
                    create = true;

                ImGui::SameLine();

                if (ImGui::Button("Annuler"))
                {
                    ImGui::CloseCurrentPopup();
                    requestPopups.folderError.Clear();
                    requestPopups.folderTarget.clear();
                }

                if (create)
                {
                    String folderName = TrimCopy(String(requestPopups.folderName));

                    if (folderName.IsEmpty())
                    {
                        LogAndStoreError(requestPopups.folderError, "Le nom du dossier ne peut pas être vide.", false);
                    }
                    else if (ContainsPathSeparator(folderName))
                    {
                        LogAndStoreError(requestPopups.folderError, "Le nom du dossier ne peut pas contenir de séparateurs.", false);
                    }
                    else
                    {
                        const fs::path baseDirectory = requestPopups.folderTarget.empty() ? state.current : requestPopups.folderTarget;
                        const fs::path folderPath = baseDirectory / folderName.View();

                        if (fs::exists(folderPath))
                        {
                            LogAndStoreError(requestPopups.folderError, "Un dossier avec ce nom existe déjà.", false);
                        }
                        else
                        {
                            std::error_code createError;
                            fs::create_directories(folderPath, createError);
                            if (createError)
                            {
                                const String errorText = createError.message();
                                String message = "Impossible de créer le dossier : ";
                                message += errorText;
                                LogAndStoreError(requestPopups.folderError, std::move(message));
                            }
                            else
                            {
                                selectedEntry = folderPath.generic_string();
                                requestPopups.folderError.Clear();
                                requestPopups.folderTarget.clear();
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                }

                ImGui::EndPopup();
            }
        }

        void RenderRenameEntryPopup(ContentState& state, String& selectedEntry, PopupRequestState& requestPopups)
        {
            namespace fs = std::filesystem;

            if (requestPopups.renameEntry)
            {
                ImGui::OpenPopup("ContentBrowserRenameEntry");
                requestPopups.renameEntry = false;
            }

            if (ImGui::BeginPopupModal("ContentBrowserRenameEntry", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                const fs::path target = requestPopups.renameTarget;
                if (!target.empty())
                {
                    fs::path relativePath = target.lexically_relative(state.root);
                    std::string relativeString = relativePath.generic_string();

                    String label = relativeString;
                    if (label.IsEmpty() || label == ".")
                        label = target.filename().generic_string();

                    String message = "Renommer : ";
                    message += label;
                    ImGui::TextWrapped("%s", message.c_str());
                }
                else
                {
                    ImGui::TextUnformatted("Renommer l'entrée sélectionnée.");
                }

                if (ImGui::IsWindowAppearing())
                    ImGui::SetKeyboardFocusHere();

                bool rename = ImGui::InputText("Nom", requestPopups.renameBuffer, IM_ARRAYSIZE(requestPopups.renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue);

                if (!requestPopups.renameError.IsEmpty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
                    ImGui::TextWrapped("%s", requestPopups.renameError.c_str());
                    ImGui::PopStyleColor();
                }

                if (ImGui::Button("Renommer"))
                    rename = true;

                ImGui::SameLine();

                if (ImGui::Button("Annuler"))
                {
                    ImGui::CloseCurrentPopup();
                    requestPopups.renameError.Clear();
                    requestPopups.renameTarget.clear();
                }

                if (rename)
                {
                    String newName = TrimCopy(String(requestPopups.renameBuffer));

                    if (newName.IsEmpty())
                    {
                        LogAndStoreError(requestPopups.renameError, "Le nom ne peut pas être vide.", false);
                    }
                    else if (ContainsPathSeparator(newName))
                    {
                        LogAndStoreError(requestPopups.renameError, "Le nom ne peut pas contenir de séparateurs.", false);
                    }
                    else if (requestPopups.renameTarget.empty())
                    {
                        LogAndStoreError(requestPopups.renameError, "Aucune entrée à renommer n'a été trouvée.", false);
                    }
                    else
                    {
                        const fs::path oldPath = requestPopups.renameTarget;
                        const String oldName = oldPath.filename().generic_string();

                        if (oldName == newName)
                        {
                            requestPopups.renameError.Clear();
                            ImGui::CloseCurrentPopup();
                        }
                        else
                        {
                            const fs::path parent = oldPath.parent_path();
                            const fs::path newPath = parent / newName.View();

                            if (fs::exists(newPath))
                            {
                                LogAndStoreError(requestPopups.renameError, "Un élément avec ce nom existe déjà.", false);
                            }
                            else
                            {
                                std::error_code renameError;
                                fs::rename(oldPath, newPath, renameError);

                                if (renameError)
                                {
                                    const String errorText = renameError.message();
                                    String errorMessage = "Impossible de renommer l'entrée : ";
                                    errorMessage += errorText;
                                    LogAndStoreError(requestPopups.renameError, std::move(errorMessage));
                                }
                                else
                                {
                                    if (selectedEntry == oldPath.generic_string())
                                        selectedEntry = newPath.generic_string();

                                    requestPopups.renameTarget = newPath;
                                    requestPopups.renameError.Clear();
                                    ImGui::CloseCurrentPopup();
                                }
                            }
                        }
                    }
                }

                ImGui::EndPopup();
            }
        }

        void RenderPopups(ContentState& state, String& selectedEntry, PopupRequestState& requestPopups)
        {
            RenderCreateScriptPopup(state, selectedEntry, requestPopups);
            RenderCreateFolderPopup(state, selectedEntry, requestPopups);
            RenderRenameEntryPopup(state, selectedEntry, requestPopups);
        }
    }

    GuiPanel& CreateContentBrowserPanel(GuiManager& guiManager, const DefaultEngineGuiContext&)
    {
        GuiPanel& contentPanel = guiManager.CreatePanel("content_browser", "Content Browser");
        guiManager.SetPanelDockingArea(contentPanel, DockSpaceRegion::Bottom);
        contentPanel.SetResizable(true);
        contentPanel.SetMovable(true);
        contentPanel.SetCollapsable(true);
        contentPanel.SetClosable(true);
        contentPanel.SetBackgroundColor(kContentBackground);
        contentPanel.AddWindowFlags(ImGuiWindowFlags_NoCollapse);
        contentPanel.SetDrawFunction([]()
        {
            namespace fs = std::filesystem;

            static ContentState state{};
            static char searchBuffer[256] = "";
            static String selectedEntry{};
            static PopupRequestState popupRequests{};

            EnsureInitialized(state);

            if (!state.error.IsEmpty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
                ImGui::TextWrapped("%s", state.error.c_str());
                ImGui::PopStyleColor();
                ImGui::Spacing();
                ImGui::TextDisabled("The Content Browser requires access to the Content directory.");
                return;
            }

            if (state.current.empty())
                state.current = state.root;

            if (!fs::exists(state.current))
                state.current = state.root;

            RenderHeader(state, selectedEntry, searchBuffer);

            ImGui::BeginGroup();
            RenderDirectoryTree(state, selectedEntry);
            ImGui::SameLine();

            const String searchQuery(searchBuffer);
            RenderEntries(state, selectedEntry, popupRequests, searchQuery);
            ImGui::EndGroup();

            RenderPopups(state, selectedEntry, popupRequests);
        });

        return contentPanel;
    }
}
