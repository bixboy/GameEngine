#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserUI_Grid.h"

#include "Core/Logger.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserActions.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentEntry.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Engine/Gui/Widgets/ActionTooltip.h"

#include "imgui.h"

#include <cstdio>
#include <ranges>
#include <unordered_map>
#include <vector>

namespace BixEngine::Gui
{
    namespace
    {
        constexpr ImVec4 kContentBackground{0.09f, 0.09f, 0.09f, 0.95f};
        constexpr float kContentThumbnailSize = 72.0f;
        constexpr float kContentThumbnailPadding = 28.0f;

#ifdef ImGuiHoveredFlags_ForTooltip
        constexpr ImGuiHoveredFlags kEntryTooltipHoverFlags = ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_ForTooltip;
#else
        constexpr ImGuiHoveredFlags kEntryTooltipHoverFlags = ImGuiHoveredFlags_DelayNormal;
#endif

        constexpr ImGuiHoveredFlags kEntryDoubleClickHoverFlags =
            ImGuiHoveredFlags_AllowWhenBlockedByActiveItem | ImGuiHoveredFlags_AllowWhenOverlapped;

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

        bool IsActorAsset(const std::filesystem::path& path)
        {
            if (path.empty())
                return false;

            const auto extension = ToLowerCopy(path.extension().generic_string());
            if (extension == ".actor" || extension == ".component")
                return true;

            const String fileName = ToLowerCopy(path.filename().generic_string());
            const auto view = fileName.View();
            return view.ends_with(".actor.json") || view.ends_with(".actor") ||
                   view.ends_with(".component.json") || view.ends_with(".component");
        }

        void RequestOpenScriptFiles(const ContentBrowserState& state, const ContentEntry& entry, bool openHeader, bool openSource)
        {
            namespace fs = std::filesystem;

            if (!openHeader && !openSource)
                return;

            std::vector<fs::path> paths{};
            paths.reserve(2);

            if (openHeader && entry.HasHeader())
                paths.push_back(entry.headerPath);

            if (openSource && entry.HasSource())
                paths.push_back(entry.sourcePath);

            if (paths.empty())
                return;

            if (state.openScriptFilesCallback)
            {
                state.openScriptFilesCallback(paths);
            }
            else
            {
                String message = "Code editor integration is not available to open script: ";
                message += entry.name;
                LOG_WARNING(message);
            }
        }

        bool RefreshDirectoryCache(ContentBrowserState& state)
        {
            namespace fs = std::filesystem;

            const bool needsRefresh = state.cache.dirty || state.cache.directory != state.current;
            if (!needsRefresh)
                return true;

            state.cache.directory = state.current;
            state.cache.entries.clear();
            state.cache.dirty = false;

            std::vector<fs::directory_entry> entries{};
            std::error_code iterationError;
            for (const auto& entry : fs::directory_iterator(state.current, iterationError))
                entries.emplace_back(entry);

            if (iterationError)
            {
                String message = String("Failed to enumerate content: ") + iterationError.message();
                LogAndStoreError(state.error, std::move(message));
                return false;
            }

            state.error.Clear();
            std::unordered_map<String, ContentEntry> scriptGroups{};

            for (const auto& entry : entries)
            {
                const fs::path entryPath = entry.path();

                if (entry.is_directory())
                {
                    ContentEntry directoryEntry{};
                    directoryEntry.type = ContentType::Directory;
                    directoryEntry.path = entryPath;
                    directoryEntry.name = entryPath.filename().generic_string();
                    state.cache.entries.push_back(std::move(directoryEntry));
                    continue;
                }

                const fs::path extension = entryPath.extension();
                const bool isHeader = extension == ".h";
                const bool isSource = extension == ".cpp";
                if (isHeader || isSource)
                {
                    String groupKey = ToLowerCopy(entryPath.stem().generic_string());
                    auto& group = scriptGroups[groupKey];
                    group.type = ContentType::Script;
                    group.path = entryPath.parent_path();
                    if (group.name.IsEmpty())
                        group.name = entryPath.stem().generic_string();
                    if (isHeader)
                        group.headerPath = entryPath;
                    else
                        group.sourcePath = entryPath;
                    continue;
                }

                if (IsActorAsset(entryPath))
                {
                    ContentEntry actorEntry{};
                    actorEntry.type = ContentType::Actor;
                    actorEntry.path = entryPath;
                    actorEntry.name = entryPath.filename().generic_string();
                    state.cache.entries.push_back(std::move(actorEntry));
                    continue;
                }

                ContentEntry fileEntry{};
                fileEntry.type = ContentType::File;
                fileEntry.path = entryPath;
                fileEntry.name = entryPath.filename().generic_string();
                state.cache.entries.push_back(std::move(fileEntry));
            }

            for (auto& [_, script] : scriptGroups)
                state.cache.entries.push_back(std::move(script));

            std::ranges::sort(state.cache.entries, [](const ContentEntry& lhs, const ContentEntry& rhs)
            {
                const int lhsPriority = GetSortPriority(lhs.type);
                const int rhsPriority = GetSortPriority(rhs.type);
                if (lhsPriority != rhsPriority)
                    return lhsPriority < rhsPriority;

                return CaseInsensitiveLess(lhs.name, rhs.name);
            });

            return true;
        }

        void RequestCreateScript(PopupRequestState& requests)
        {
            std::snprintf(requests.scriptName, IM_ARRAYSIZE(requests.scriptName), "%s", "NewScript");
            requests.scriptType = ScriptTemplateType::Actor;
            requests.scriptError.Clear();
            requests.createScript = true;
            ClearSelectedParent(requests);
        }

        void RequestCreateFolder(PopupRequestState& requests, const std::filesystem::path& target)
        {
            std::snprintf(requests.folderName, IM_ARRAYSIZE(requests.folderName), "%s", "NewFolder");
            requests.folderError.Clear();
            requests.folderTarget = target;
            requests.createFolder = true;
        }
    }

    // ─────────────────────────────────────────────
    // 🧱  Grille des entrées et interactions
    // ─────────────────────────────────────────────

    void RenderEntries(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requestPopups, const String& searchQuery)
    {
        namespace Utils = Gui::Utils;

        if (!RefreshDirectoryCache(state))
        {
            Utils::DrawEmptyStateMessage(state.error.IsEmpty() ? "Unable to open directory." : state.error.c_str());
            return;
        }

        ScopedColor background(ImGuiCol_ChildBg, kContentBackground);
        if (!ImGui::BeginChild("ContentBrowserGrid", ImVec2(0.0f, 0.0f), true))
            return;

        if (ImGui::BeginPopupContextWindow("ContentBrowserBackgroundContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Create script..."))
                RequestCreateScript(requestPopups);

            if (ImGui::MenuItem("Create folder..."))
            {
                requestPopups.folderTarget = state.current;
                RequestCreateFolder(requestPopups, state.current);
            }

            ImGui::EndPopup();
        }

        const float thumbnailWithPadding = kContentThumbnailSize + kContentThumbnailPadding;
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        const int columns = std::max(1, static_cast<int>(availableWidth / thumbnailWithPadding));

        if (ImGui::BeginTable("ContentBrowserEntries", columns, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_PadOuterX | ImGuiTableFlags_NoSavedSettings))
        {
            for (const ContentEntry& entry : state.cache.entries)
            {
                if (!MatchesSearch(entry.name, searchQuery))
                    continue;

                ImGui::TableNextColumn();
                ScopedID entryId(entry.SelectionKey().c_str());
                ImGui::BeginGroup();

                const String selectionId = entry.SelectionKey();
                const bool isSelected = selectedEntry == selectionId;

                const ImVec4 baseColor = isSelected ? ImVec4(0.20f, 0.35f, 0.60f, 0.95f) : ImGui::GetStyleColorVec4(ImGuiCol_Button);
                const ImVec4 hoverColor = AdjustColor(baseColor, 0.10f);
                const ImVec4 activeColor = AdjustColor(baseColor, 0.20f);

                ScopedColor buttonColor(ImGuiCol_Button, baseColor);
                ScopedColor buttonHover(ImGuiCol_ButtonHovered, hoverColor);
                ScopedColor buttonActive(ImGuiCol_ButtonActive, activeColor);
                ScopedStyle buttonPadding(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 12.0f));

                const bool clicked = ImGui::Button(GetIcon(entry.type), ImVec2(kContentThumbnailSize, kContentThumbnailSize));
                const bool hoveredForDoubleClick = ImGui::IsItemHovered(kEntryDoubleClickHoverFlags);

                if (ImGui::BeginPopupContextItem("ContentBrowserEntryContext"))
                {
                    DrawEntryContextMenu(state, entry, requestPopups, selectedEntry);
                    ImGui::EndPopup();
                }

                if (clicked)
                {
                    if (entry.IsDirectory())
                    {
                        state.current = entry.path;
                        state.cache.dirty = true;
                        selectedEntry.Clear();
                    }
                    else
                    {
                        selectedEntry = selectionId;
                    }
                }

                if (entry.IsScript() && hoveredForDoubleClick && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    selectedEntry = selectionId;
                    if (state.openActorEditorCallback)
                        state.openActorEditorCallback(entry.path);
                }

                if (ImGui::IsItemHovered(kEntryTooltipHoverFlags))
                {
                    if (entry.IsDirectory())
                    {
                        ShowActionTooltip(entry.name, {
                            {"Open", [&, path = entry.path]()
                            {
                                state.current = path;
                                state.cache.dirty = true;
                                selectedEntry.Clear();
                            }},
                            {"Create script...", [&]()
                            {
                                RequestCreateScript(requestPopups);
                            }},
                            {"Create folder...", [&, path = entry.path]()
                            {
                                RequestCreateFolder(requestPopups, path);
                            }}
                        });
                    }
                    else if (entry.IsScript())
                    {
                        const bool hasSource = entry.HasSource();
                        ShowActionTooltip(entry.name, {
                            {"Open header", [&, selection = selectionId, &entry]()
                            {
                                selectedEntry = selection;
                                RequestOpenScriptFiles(state, entry, true, false);
                            }},
                            {"Open source", [&, selection = selectionId, &entry, hasSource]()
                            {
                                if (!hasSource)
                                    return;
                                selectedEntry = selection;
                                RequestOpenScriptFiles(state, entry, false, true);
                            }},
                            {"Open both", [&, selection = selectionId, &entry]()
                            {
                                selectedEntry = selection;
                                RequestOpenScriptFiles(state, entry, true, true);
                            }}
                        });
                    }
                    else if (entry.IsActor())
                    {
                        ShowActionTooltip(entry.name, {
                            {"Open actor editor", [&, selection = selectionId, &entry]()
                            {
                                selectedEntry = selection;
                                if (state.openActorEditorCallback)
                                    state.openActorEditorCallback(entry.path);
                            }}
                        });
                    }
                    else
                    {
                        ShowActionTooltip(entry.name, {
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

                ImGui::TextWrapped("%s", entry.name.c_str());

                if (pushedTextColor)
                    ImGui::PopStyleColor();

                ImGui::EndGroup();
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

