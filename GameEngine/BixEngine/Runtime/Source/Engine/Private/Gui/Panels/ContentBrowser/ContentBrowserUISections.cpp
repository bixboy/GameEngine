#include "Bix/Core/Logger.h"
#include "Bix/Engine/Gui/Widgets/ActionTooltip.h"
#include "imgui_internal.h"
#include <algorithm>
#include <filesystem>
#include <vector>
#include "Bix/Engine/Gui/Panels/ContentBrowser/ContentBrowserPanelInternal.h"


namespace BixEngine::Gui
{
    namespace
    {
        ImVec4 AdjustColor(const ImVec4& color, float delta)
        {
            auto clamp = [](float value)
            {
                if (value < 0.0f)
                    return 0.0f;
                if (value > 1.0f)
                    return 1.0f;
                return value;
            };

            return ImVec4(
                clamp(color.x + delta),
                clamp(color.y + delta),
                clamp(color.z + delta),
                clamp(color.w));
        }
    }

    void RenderHeader(ContentBrowserState& state, String& selectedEntry, char (&searchBuffer)[256])
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
            ImGui::TextDisabled("Right click to create new scripts or folders.");
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    void RenderDirectoryTree(ContentBrowserState& state, String& selectedEntry)
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
                renderDirectoryTree(renderDirectoryTree, state.root, 0);
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::EndChild();
    }

    void RenderEntries(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups, const String& searchQuery)
    {
        namespace fs = std::filesystem;

        ImGui::BeginChild("ContentBrowserGrid", ImVec2(0.0f, 0.0f), true);

        if (ImGui::BeginPopupContextWindow("ContentBrowserBackgroundContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Create script..."))
            {
                std::snprintf(requestPopups.scriptName, IM_ARRAYSIZE(requestPopups.scriptName), "%s", "NewScript");
                requestPopups.scriptType = ScriptTemplateType::Actor;
                requestPopups.scriptError.Clear();
                requestPopups.createScript = true;
                requestPopups.selectedParentScript = -1;
            }

            if (ImGui::MenuItem("Create folder..."))
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
            entries.emplace_back(entry);

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
                const bool isScript = !isDirectory && (entryPath.extension() == ".h" || entryPath.extension() == ".cpp");
                const char* icon = isDirectory ? "\xef\x81\xbb" : (isScript ? "\xF0\x9F\x93\x9C" : "\xef\x81\x96");

                ImGui::TableNextColumn();
                ImGui::PushID(entryName.c_str());
                ImGui::BeginGroup();

                const bool isSelected = selectedEntry == entryPathString;
                const ImVec4 baseColor = isSelected ? ImVec4(0.20f, 0.35f, 0.60f, 0.95f) : ImGui::GetStyleColorVec4(ImGuiCol_Button);
                const ImVec4 hoverColor = AdjustColor(baseColor, 0.10f);
                const ImVec4 activeColor = AdjustColor(baseColor, 0.20f);

                ImGui::PushStyleColor(ImGuiCol_Button, baseColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 12.0f));
                const ImVec2 buttonSize(kContentThumbnailSize, kContentThumbnailSize);
                const bool clicked = ImGui::Button(icon, buttonSize);
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(3);

                if (ImGui::BeginPopupContextItem("ContentBrowserEntryContext"))
                {
                    ImGui::TextDisabled("Actions");
                    ImGui::Separator();

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

                    if (ImGui::MenuItem("Rename..."))
                    {
                        std::snprintf(requestPopups.renameBuffer, IM_ARRAYSIZE(requestPopups.renameBuffer), "%s", entryName.c_str());
                        requestPopups.renameError.Clear();
                        requestPopups.renameTarget = entryPath;
                        requestPopups.renameEntry = true;
                    }

                    if (ImGui::MenuItem("Delete"))
                    {
                        std::error_code removeError;
                        if (isDirectory)
                            std::filesystem::remove_all(entryPath, removeError);
                        else
                            std::filesystem::remove(entryPath, removeError);

                        if (removeError)
                        {
                            String message = "Unable to delete entry: ";
                            message += removeError.message();
                            LOG_ERROR(message);
                        }
                        else if (selectedEntry == entryPathString)
                        {
                            selectedEntry.Clear();
                        }
                    }

                    ImGui::Separator();
                    ImGui::TextDisabled("Utilities");
                    ImGui::Separator();

                    if (ImGui::MenuItem("Reveal in Explorer"))
                        ShowPathInExplorer(entryPath, isDirectory);

                    if (ImGui::MenuItem("New folder here..."))
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
                            {"Open", [&, entryPath]()
                            {
                                state.current = entryPath;
                                selectedEntry.Clear();
                            }},
                            {"Create script...", [&]()
                            {
                                std::snprintf(requestPopups.scriptName, IM_ARRAYSIZE(requestPopups.scriptName), "%s", "NewScript");
                                requestPopups.scriptError.Clear();
                                requestPopups.scriptType = ScriptTemplateType::Actor;
                                requestPopups.createScript = true;
                                requestPopups.selectedParentScript = -1;
                            }},
                            {"Create folder...", [&, entryPath]()
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
                            {"Open", [&, entryPathString]()
                            {
                                selectedEntry = entryPathString;
                            }}
                        });
                    }
                }

                bool pushedTextColor = false;
                if (isSelected)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.85f, 0.40f, 1.0f));
                    pushedTextColor = true;
                }
                else if (ImGui::IsItemHovered())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
                    pushedTextColor = true;
                }

                ImGui::TextWrapped("%s", entryName.c_str());

                if (pushedTextColor)
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
}
