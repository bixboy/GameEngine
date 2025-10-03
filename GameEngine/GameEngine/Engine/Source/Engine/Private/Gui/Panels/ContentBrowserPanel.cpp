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

            struct ContentState
            {
                fs::path root{};
                fs::path current{};
                String error{};
                bool initialized{false};
            };

            static ContentState state{};
            static char searchBuffer[256] = "";
            static std::string selectedEntry{};
            static bool requestCreateScriptPopup = false;
            static bool requestCreateFolderPopup = false;
            static char createScriptName[128] = "NewScript.lua";
            static char createFolderName[128] = "NewFolder";
            static String createScriptError{};
            static String createFolderError{};

            const auto toLowerCopy = [](std::string value)
            {
                std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch)
                {
                    return static_cast<char>(std::tolower(ch));
                });
                
                return value;
            };

            const auto caseInsensitiveLess = [&](const std::string& lhs, const std::string& rhs)
            {
                const std::string lhsLower = toLowerCopy(lhs);
                const std::string rhsLower = toLowerCopy(rhs);
                if (lhsLower == rhsLower)
                    return lhs < rhs;
                return lhsLower < rhsLower;
            };

            if (!state.initialized)
            {
                std::error_code cwdError;
                const fs::path basePath = fs::current_path(cwdError);
                if (cwdError)
                {
                    Engine::String message = Engine::String("Failed to determine working directory: ") + cwdError.message();
                    LOG_ERROR(message);
                    state.error = message;
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
                        message += std::string_view{createError.message()};
                        message += ')';
                        
                        LOG_ERROR(message);
                        state.error = message;
                    }
                    else if (!fs::exists(state.root))
                    {
                        String message = String("Content directory is not available: ") + state.root.string();
                        LOG_ERROR(message);
                        state.error = message;
                    }
                    else
                    {
                        state.current = state.root;
                    }
                }

                state.initialized = true;
            }

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

            fs::path relativePath = state.current.lexically_relative(state.root);
            const std::string relativeString = relativePath.generic_string();
            const bool atRoot = relativeString.empty() || relativeString == ".";

            const std::string_view searchQuery(searchBuffer);
            const bool hasSearch = !searchQuery.empty();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, kContentHeaderBackground);
            if (ImGui::BeginChild("ContentBrowserHeader", ImVec2(0.0f, kContentHeaderHeight), true, ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 6.0f));
                if (ImGui::Button("Content"))
                {
                    state.current = state.root;
                    selectedEntry.clear();
                }
                
                ImGui::PopStyleVar();
                ImGui::SameLine();
                ImGui::BeginDisabled(atRoot);
                
                if (ImGui::Button("Up"))
                {
                    fs::path parent = state.current.parent_path();
                    fs::path parentRelative = parent.lexically_relative(state.root);
                    const String parentString = parentRelative.generic_string();

                    if (parentString.empty() || parentString == "." || parentString.rfind("..", 0) == 0)
                    {
                     state.current = state.root;   
                    }
                    else
                    {
                        state.current = parent;
                    }
                    
                    selectedEntry.clear();
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
                        selectedEntry.clear();
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
                                return caseInsensitiveLess(lhs.filename().generic_string(), rhs.filename().generic_string());
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

            ImGui::SameLine();

            ImGui::BeginChild("ContentBrowserGrid", ImVec2(0.0f, 0.0f), true);

            if (ImGui::BeginPopupContextWindow("ContentBrowserBackgroundContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                if (ImGui::MenuItem("Créer un script..."))
                {
                    std::snprintf(createScriptName, IM_ARRAYSIZE(createScriptName), "%s", "NewScript.lua");
                    createScriptError.Clear();
                    requestCreateScriptPopup = true;
                }

                if (ImGui::MenuItem("Créer un dossier..."))
                {
                    std::snprintf(createFolderName, IM_ARRAYSIZE(createFolderName), "%s", "NewFolder");
                    createFolderError.Clear();
                    requestCreateFolderPopup = true;
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
                String message = String("Failed to enumerate content: ") + iterationError.message();
                LOG_ERROR(message);
                ImGui::TextDisabled("Unable to read directory contents.");
                ImGui::EndChild();
                return;
            }

            std::sort(entries.begin(), entries.end(), [&](const fs::directory_entry& lhs, const fs::directory_entry& rhs)
            {
                if (lhs.is_directory() && !rhs.is_directory())
                    return true;
                
                if (!lhs.is_directory() && rhs.is_directory())
                    return false;
                
                return caseInsensitiveLess(lhs.path().filename().generic_string(), rhs.path().filename().generic_string());
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
                    const std::string entryName = entryPath.filename().generic_string();
                    const std::string entryPathString = entryPath.generic_string();
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
                                selectedEntry.clear();
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
                        selectedEntry.clear();
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
                                    selectedEntry.clear();
                                }},
                                {"Créer un script...", [&]()
                                {
                                    requestCreateScriptPopup = true;
                                }},
                                {"Créer un dossier...", [&]()
                                {
                                    requestCreateFolderPopup = true;
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

            if (requestCreateScriptPopup)
            {
                ImGui::OpenPopup("ContentBrowserCreateScript");
                requestCreateScriptPopup = false;
            }

            if (ImGui::BeginPopupModal("ContentBrowserCreateScript", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextUnformatted("Créer un nouveau script dans le dossier courant.");

                if (ImGui::IsWindowAppearing())
                    ImGui::SetKeyboardFocusHere();

                bool create = ImGui::InputText("Nom", createScriptName, IM_ARRAYSIZE(createScriptName), ImGuiInputTextFlags_EnterReturnsTrue);

                if (!createScriptError.IsEmpty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
                    ImGui::TextWrapped("%s", createScriptError.c_str());
                    ImGui::PopStyleColor();
                }

                if (ImGui::Button("Créer"))
                    create = true;

                ImGui::SameLine();

                if (ImGui::Button("Annuler"))
                {
                    ImGui::CloseCurrentPopup();
                    createScriptError.Clear();
                }

                if (create)
                {
                    std::string scriptName = createScriptName;
                    scriptName.erase(scriptName.begin(), std::find_if(scriptName.begin(), scriptName.end(), [](unsigned char ch)
                    {
                        return !std::isspace(ch);
                    }));
                    scriptName.erase(std::find_if(scriptName.rbegin(), scriptName.rend(), [](unsigned char ch)
                    {
                        return !std::isspace(ch);
                    }).base(), scriptName.end());

                    if (scriptName.empty())
                    {
                        createScriptError = "Le nom du script ne peut pas être vide.";
                    }
                    else
                    {
                        fs::path scriptFileName = scriptName;
                        if (scriptFileName.extension().empty())
                            scriptFileName.replace_extension(".lua");

                        const fs::path scriptPath = state.current / scriptFileName;

                        if (fs::exists(scriptPath))
                        {
                            createScriptError = "Un fichier avec ce nom existe déjà.";
                        }
                        else
                        {
                            std::ofstream scriptFile(scriptPath);
                            if (!scriptFile.is_open())
                            {
                                createScriptError = "Impossible de créer le script.";
                            }
                            else
                            {
                                scriptFile << "// Nouveau script généré depuis le Content Browser\n";
                                scriptFile.close();

                                selectedEntry = scriptPath.generic_string();
                                createScriptError.Clear();
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                }

                ImGui::EndPopup();
            }

            if (requestCreateFolderPopup)
            {
                ImGui::OpenPopup("ContentBrowserCreateFolder");
                requestCreateFolderPopup = false;
            }

            if (ImGui::BeginPopupModal("ContentBrowserCreateFolder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::TextUnformatted("Créer un nouveau dossier dans le dossier courant.");

                if (ImGui::IsWindowAppearing())
                    ImGui::SetKeyboardFocusHere();

                bool create = ImGui::InputText("Nom", createFolderName, IM_ARRAYSIZE(createFolderName), ImGuiInputTextFlags_EnterReturnsTrue);

                if (!createFolderError.IsEmpty())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.45f, 1.0f));
                    ImGui::TextWrapped("%s", createFolderError.c_str());
                    ImGui::PopStyleColor();
                }

                if (ImGui::Button("Créer"))
                    create = true;

                ImGui::SameLine();

                if (ImGui::Button("Annuler"))
                {
                    ImGui::CloseCurrentPopup();
                    createFolderError.Clear();
                }

                if (create)
                {
                    std::string folderName = createFolderName;
                    folderName.erase(folderName.begin(), std::find_if(folderName.begin(), folderName.end(), [](unsigned char ch)
                    {
                        return !std::isspace(ch);
                    }));
                    folderName.erase(std::find_if(folderName.rbegin(), folderName.rend(), [](unsigned char ch)
                    {
                        return !std::isspace(ch);
                    }).base(), folderName.end());

                    if (folderName.empty())
                    {
                        createFolderError = "Le nom du dossier ne peut pas être vide.";
                    }
                    else
                    {
                        const fs::path folderPath = state.current / folderName;

                        if (fs::exists(folderPath))
                        {
                            createFolderError = "Un dossier avec ce nom existe déjà.";
                        }
                        else
                        {
                            std::error_code createError;
                            fs::create_directories(folderPath, createError);
                            if (createError)
                            {
                                createFolderError = Engine::String("Impossible de créer le dossier : ") + createError.message();
                            }
                            else
                            {
                                selectedEntry = folderPath.generic_string();
                                createFolderError.Clear();
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                }

                ImGui::EndPopup();
            }
        });

        return contentPanel;
    }
}
