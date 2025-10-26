#include "Core/Logger.h"
#include "Engine/Gui/Widgets/ActionTooltip.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "imgui_internal.h"
#include <algorithm>
#include <filesystem>
#include <unordered_map>
#include <utility>
#include <vector>
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserPanelInternal.h"


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

        namespace Utils = BixEngine::Gui::Utils;

        struct ScriptEntryInfo
        {
            String name{};
            std::filesystem::path directory{};
            std::filesystem::path headerPath{};
            std::filesystem::path sourcePath{};

            [[nodiscard]] bool HasHeader() const noexcept { return !headerPath.empty(); }
            [[nodiscard]] bool HasSource() const noexcept { return !sourcePath.empty(); }
        };

        enum class ContentEntryType
        {
            Directory = 0,
            Script,
            File,
        };

        struct DisplayEntry
        {
            ContentEntryType type{ContentEntryType::File};
            std::filesystem::path path{};
            ScriptEntryInfo script{};
        };

        [[nodiscard]] String BuildScriptSelectionId(const ScriptEntryInfo& script)
        {
            std::filesystem::path selectionKey = script.directory / script.name.View();
            return selectionKey.generic_string();
        }

        void RequestOpenScriptFiles(const ContentBrowserState& state, const ScriptEntryInfo& script, bool openHeader, bool openSource)
        {
            namespace fs = std::filesystem;

            if (!openHeader && !openSource)
                return;

            std::vector<fs::path> paths{};
            paths.reserve(2);

            if (openHeader && script.HasHeader())
                paths.push_back(script.headerPath);

            if (openSource && script.HasSource())
                paths.push_back(script.sourcePath);

            if (paths.empty())
                return;

            if (state.openScriptFilesCallback)
            {
                state.openScriptFilesCallback(paths);
            }
            else
            {
                String message = "Code editor integration is not available to open script: ";
                message += script.name;
                LOG_WARNING(message);
            }
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
            Utils::DrawEmptyStateMessage(atRoot ? "Content" : relativeString.c_str());

            ImGui::SameLine();
            Utils::SearchInput("ContentSearch", searchBuffer, IM_ARRAYSIZE(searchBuffer), "Search content...", 220.0f);

            ImGui::Spacing();
            Utils::DrawEmptyStateMessage("Right click to create new scripts or folders.");
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
                        Utils::DrawEmptyStateMessage("Unable to open directory.");
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
                                Utils::DrawEmptyStateMessage("...");
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
                ClearSelectedParent(requestPopups);
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
            Utils::DrawEmptyStateMessage(displayMessage.c_str());
            ImGui::EndChild();
            return;
        }

        std::unordered_map<String, ScriptEntryInfo> scriptGroups{};
        std::vector<DisplayEntry> displayEntries{};
        displayEntries.reserve(entries.size());

        for (const auto& entry : entries)
        {
            const fs::path entryPath = entry.path();

            if (entry.is_directory())
            {
                DisplayEntry directoryEntry{};
                directoryEntry.type = ContentEntryType::Directory;
                directoryEntry.path = entryPath;
                displayEntries.push_back(std::move(directoryEntry));
                continue;
            }

            const fs::path extension = entryPath.extension();
            const bool isHeader = extension == ".h";
            const bool isSource = extension == ".cpp";
            if (isHeader || isSource)
            {
                String groupKey = ToLowerCopy(entryPath.stem().generic_string());
                auto& group = scriptGroups[groupKey];
                if (group.name.IsEmpty())
                    group.name = entryPath.stem().generic_string();
                group.directory = entryPath.parent_path();
                if (isHeader)
                    group.headerPath = entryPath;
                else
                    group.sourcePath = entryPath;
                continue;
            }

            DisplayEntry fileEntry{};
            fileEntry.type = ContentEntryType::File;
            fileEntry.path = entryPath;
            displayEntries.push_back(std::move(fileEntry));
        }

        for (auto& [_, script] : scriptGroups)
        {
            DisplayEntry scriptEntry{};
            scriptEntry.type = ContentEntryType::Script;
            scriptEntry.script = std::move(script);
            displayEntries.push_back(std::move(scriptEntry));
        }

        std::sort(displayEntries.begin(), displayEntries.end(), [&](const DisplayEntry& lhs, const DisplayEntry& rhs)
        {
            const auto priority = [](ContentEntryType type)
            {
                switch (type)
                {
                case ContentEntryType::Directory:
                    return 0;
                case ContentEntryType::Script:
                    return 1;
                case ContentEntryType::File:
                default:
                    return 2;
                }
            };

            const int lhsPriority = priority(lhs.type);
            const int rhsPriority = priority(rhs.type);
            if (lhsPriority != rhsPriority)
                return lhsPriority < rhsPriority;

            const String lhsName = lhs.type == ContentEntryType::Script
                ? lhs.script.name
                : String(lhs.path.filename().generic_string());

            const String rhsName = rhs.type == ContentEntryType::Script
                ? rhs.script.name
                : String(rhs.path.filename().generic_string());


            return CaseInsensitiveLess(lhsName, rhsName);
        });

        const float cellSize = kContentThumbnailSize + kContentThumbnailPadding;
        const float panelWidth = ImGui::GetContentRegionAvail().x;
        int columns = static_cast<int>(panelWidth / cellSize);
        columns = std::max(columns, 1);

        if (ImGui::BeginTable("ContentBrowserEntries", columns))
        {
            ImGui::TableSetupScrollFreeze(0, 1);

            for (const auto& entry : displayEntries)
            {
                const bool isDirectory = entry.type == ContentEntryType::Directory;
                const bool isScript = entry.type == ContentEntryType::Script;
                const String entryName = isScript ? entry.script.name : String(entry.path.filename().generic_string());
                
                if (!MatchesSearch(entryName, searchQuery))
                    continue;

                const String selectionId = isScript ? BuildScriptSelectionId(entry.script) : String(entry.path.generic_string());
                const char* icon = isDirectory ? "\xef\x81\xbb" : (isScript ? "<>" : "\xef\x81\x96");

                ImGui::TableNextColumn();
                ImGui::PushID(entryName.c_str());
                ImGui::BeginGroup();

                const bool isSelected = selectedEntry == selectionId;
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
                    Utils::DrawEmptyStateMessage("Actions");
                    ImGui::Separator();

                    if (isDirectory)
                    {
                        if (ImGui::MenuItem("Open"))
                        {
                            state.current = entry.path;
                            selectedEntry.Clear();
                        }
                    }
                    else if (isScript)
                    {
                        const bool hasHeader = entry.script.HasHeader();
                        const bool hasSource = entry.script.HasSource();

                        if (ImGui::MenuItem("Open Header", nullptr, false, hasHeader))
                        {
                            selectedEntry = selectionId;
                            RequestOpenScriptFiles(state, entry.script, true, false);
                        }

                        if (ImGui::MenuItem("Open Source", nullptr, false, hasSource))
                        {
                            selectedEntry = selectionId;
                            RequestOpenScriptFiles(state, entry.script, false, true);
                        }

                        if (ImGui::MenuItem("Open Both", nullptr, false, hasHeader || hasSource))
                        {
                            selectedEntry = selectionId;
                            RequestOpenScriptFiles(state, entry.script, true, true);
                        }
                    }
                    else
                    {
                        if (ImGui::MenuItem("Open"))
                            selectedEntry = selectionId;
                    }

                    if (ImGui::MenuItem("Rename..."))
                    {
                        if (isScript)
                        {
                            std::snprintf(requestPopups.renameBuffer, IM_ARRAYSIZE(requestPopups.renameBuffer), "%s", entry.script.name.c_str());
                            requestPopups.renameError.Clear();
                            requestPopups.renameTarget = entry.script.headerPath;
                            requestPopups.renameSecondaryTarget = entry.script.sourcePath;
                            requestPopups.renameTargetIsScriptGroup = true;
                            requestPopups.renameEntry = true;
                        }
                        else
                        {
                            std::snprintf(requestPopups.renameBuffer, IM_ARRAYSIZE(requestPopups.renameBuffer), "%s", entryName.c_str());
                            requestPopups.renameError.Clear();
                            requestPopups.renameTarget = entry.path;
                            requestPopups.renameSecondaryTarget.clear();
                            requestPopups.renameTargetIsScriptGroup = false;
                            requestPopups.renameEntry = true;
                        }
                    }

                    if (ImGui::MenuItem("Delete"))
                    {
                        bool success = true;
                        if (isDirectory)
                        {
                            std::error_code removeError;
                            fs::remove_all(entry.path, removeError);
                            if (removeError)
                            {
                                String message = "Unable to delete entry: ";
                                message += removeError.message();
                                LOG_ERROR(message);
                                success = false;
                            }
                        }
                        else if (isScript)
                        {
                            const auto removeScriptFile = [&](const fs::path& path)
                            {
                                if (path.empty())
                                    return true;

                                std::error_code removeError;
                                fs::remove(path, removeError);
                                if (removeError)
                                {
                                    String message = "Unable to delete entry: ";
                                    message += removeError.message();
                                    LOG_ERROR(message);
                                    return false;
                                }

                                return true;
                            };

                            const bool headerResult = removeScriptFile(entry.script.headerPath);
                            const bool sourceResult = removeScriptFile(entry.script.sourcePath);
                            success = headerResult && sourceResult;
                        }
                        else
                        {
                            std::error_code removeError;
                            fs::remove(entry.path, removeError);
                            if (removeError)
                            {
                                String message = "Unable to delete entry: ";
                                message += removeError.message();
                                LOG_ERROR(message);
                                success = false;
                            }
                        }

                        if (success && selectedEntry == selectionId)
                            selectedEntry.Clear();
                    }

                    ImGui::Separator();
                    Utils::DrawEmptyStateMessage("Utilities");
                    ImGui::Separator();

                    const fs::path explorerPath = isScript
                        ? (entry.script.HasHeader() ? entry.script.headerPath : entry.script.sourcePath)
                        : entry.path;

                    if (ImGui::MenuItem("Reveal in Explorer"))
                        ShowPathInExplorer(explorerPath, isDirectory);

                    if (ImGui::MenuItem("New folder here..."))
                    {
                        std::snprintf(requestPopups.folderName, IM_ARRAYSIZE(requestPopups.folderName), "%s", "NewFolder");
                        requestPopups.folderError.Clear();
                        if (isDirectory)
                        {
                            requestPopups.folderTarget = entry.path;
                        }
                        else
                        {
                            fs::path parent = isScript ? entry.script.directory : entry.path.parent_path();
                            if (parent.empty())
                                parent = state.current;
                            requestPopups.folderTarget = std::move(parent);
                        }
                        requestPopups.createFolder = true;
                    }

                    ImGui::EndPopup();
                }

                if (clicked)
                {
                    if (isDirectory)
                    {
                        state.current = entry.path;
                        selectedEntry.Clear();
                    }
                    else
                    {
                        selectedEntry = selectionId;
                    }
                }

                if (isScript && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    selectedEntry = selectionId;
                    RequestOpenScriptFiles(state, entry.script, true, true);
                }

                if (ImGui::IsItemHovered(kEntryTooltipHoverFlags))
                {
                    if (isDirectory)
                    {
                        ShowActionTooltip(entryName, {
                            {"Open", [&, entryPath = entry.path]()
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
                                ClearSelectedParent(requestPopups);
                            }},
                            {"Create folder...", [&, entryPath = entry.path]()
                            {
                                std::snprintf(requestPopups.folderName, IM_ARRAYSIZE(requestPopups.folderName), "%s", "NewFolder");
                                requestPopups.folderError.Clear();
                                requestPopups.folderTarget = entryPath;
                                requestPopups.createFolder = true;
                            }}
                        });
                    }
                    else if (isScript)
                    {
                        const bool hasSource = entry.script.HasSource();
                        ShowActionTooltip(entryName, {
                            {"Open header", [&, script = entry.script, selection = selectionId]()
                            {
                                selectedEntry = selection;
                                RequestOpenScriptFiles(state, script, true, false);
                            }},
                            {"Open source", [&, script = entry.script, selection = selectionId, hasSource]()
                            {
                                if (!hasSource)
                                    return;
                                selectedEntry = selection;
                                RequestOpenScriptFiles(state, script, false, true);
                            }},
                            {"Open both", [&, script = entry.script, selection = selectionId]()
                            {
                                selectedEntry = selection;
                                RequestOpenScriptFiles(state, script, true, true);
                            }}
                        });
                    }
                    else
                    {
                        ShowActionTooltip(entryName, {
                            {"Open", [&, selection = selectionId]()
                            {
                                selectedEntry = selection;
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
            Utils::DrawEmptyStateMessage("Nothing to show.");
        }

        ImGui::EndChild();
    }
}
