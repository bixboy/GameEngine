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
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
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
            char scriptName[128] = "NewScript.lua";
            char folderName[128] = "NewFolder";
            String scriptError{};
            String folderError{};
        };

        void LogAndStoreError(String& storage, String message, bool log = true)
        {
            if (log)
                LOG_ERROR(message);

            storage = std::move(message);
        }

        std::string ToLowerCopy(std::string_view value)
        {
            std::string result(value);
            std::transform(result.begin(), result.end(), result.begin(), [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });

            return result;
        }

        bool CaseInsensitiveLess(std::string_view lhs, std::string_view rhs)
        {
            const std::string lhsLower = ToLowerCopy(lhs);
            const std::string rhsLower = ToLowerCopy(rhs);
            if (lhsLower == rhsLower)
                return lhs < rhs;

            return lhsLower < rhsLower;
        }

        std::string TrimCopy(std::string_view value)
        {
            std::string result(value);
            const auto isSpace = [](unsigned char ch)
            {
                return std::isspace(ch) != 0;
            };

            result.erase(result.begin(), std::find_if(result.begin(), result.end(), [&](char ch)
            {
                return !isSpace(static_cast<unsigned char>(ch));
            }));

            result.erase(std::find_if(result.rbegin(), result.rend(), [&](char ch)
            {
                return !isSpace(static_cast<unsigned char>(ch));
            }).base(), result.end());

            return result;
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
            const std::string relativeString = relativePath.generic_string();
            const bool atRoot = relativeString.empty() || relativeString == ".";

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
                    const std::string directoryName = directory == state.root ? "Content" : directory.filename().generic_string();
                    std::error_code equivalentError;
                    const bool isSelected = fs::equivalent(directory, state.current, equivalentError);
                    const ImGuiTreeNodeFlags nodeFlags =
                        ImGuiTreeNodeFlags_OpenOnArrow |
                        ImGuiTreeNodeFlags_OpenOnDoubleClick |
                        ImGuiTreeNodeFlags_SpanFullWidth |
                        (isSelected ? ImGuiTreeNodeFlags_Selected : 0);

                    const bool open = ImGui::TreeNodeEx(directory.generic_string().c_str(), nodeFlags, "%s", directoryName.c_str());
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
                                return CaseInsensitiveLess(lhs.filename().generic_string(), rhs.filename().generic_string());
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

        bool MatchesSearch(std::string_view value, std::string_view query)
        {
            if (query.empty())
                return true;

            const std::string valueLower = ToLowerCopy(value);
            const std::string queryLower = ToLowerCopy(query);
            return valueLower.find(queryLower) != std::string::npos;
        }

        void RenderEntries(ContentState& state, String& selectedEntry, PopupRequestState& requestPopups, std::string_view searchQuery)
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
                const std::string errorText = iterationError.message();
                String displayMessage{};
                LogAndStoreError(displayMessage, String("Failed to enumerate content: ") + String(errorText));
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

                return CaseInsensitiveLess(lhs.path().filename().generic_string(), rhs.path().filename().generic_string());
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
                        if (ImGui::MenuItem("Open"))
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
                                    requestPopups.createScript = true;
                                }},
                                {"Créer un dossier...", [&]()
                                {
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
                    std::string scriptName = TrimCopy(requestPopups.scriptName);

                    if (scriptName.empty())
                    {
                        LogAndStoreError(requestPopups.scriptError, "Le nom du script ne peut pas être vide.", false);
                    }
                    else
                    {
                        fs::path scriptFileName = scriptName;
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
                ImGui::TextUnformatted("Créer un nouveau dossier dans le dossier courant.");

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
                }

                if (create)
                {
                    std::string folderName = TrimCopy(requestPopups.folderName);

                    if (folderName.empty())
                    {
                        LogAndStoreError(requestPopups.folderError, "Le nom du dossier ne peut pas être vide.", false);
                    }
                    else
                    {
                        const fs::path folderPath = state.current / folderName;

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
                                ImGui::CloseCurrentPopup();
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
        }
    }

    GuiPanel& CreateContentBrowserPanel(GuiManager& guiManager, const DefaultEngineGuiContext&)
    {
        GuiPanel& contentPanel = guiManager.CreatePanel("content_browser", "Content Browser");
        contentPanel.SetPosition(340.0f, 50.0f);
        contentPanel.SetSize(900.0f, 540.0f);
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

            const std::string_view searchQuery(searchBuffer);
            RenderEntries(state, selectedEntry, popupRequests, searchQuery);
            ImGui::EndGroup();

            RenderPopups(state, selectedEntry, popupRequests);
        });

        return contentPanel;
    }
}
