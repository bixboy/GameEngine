#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserUI_Grid.h"

#include "Core/Logger.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserFileUtils.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Engine/Gui/Panels/ContentBrowser/ContentEntry.h"
#include "Engine/Gui/Utils/GuiHelpers.h"
#include "Engine/Gui/Widgets/ActionTooltip.h"

#include "imgui.h"

#include <cstdio>
#include <functional>
#include <ranges>
#include <unordered_map>
#include <vector>

namespace BixEngine::Gui
{
    using namespace Theme;
    using namespace Utils;

    namespace
    {
        bool IsActorAsset(const std::filesystem::path& path)
        {
            if (path.empty())
                return false;

            const auto extension = ToLowerCopy(path.extension().generic_string());
            if (extension == ".actor" || extension == ".component")
                return true;

            const String fileName = ToLowerCopy(path.filename().generic_string());
            const auto view = fileName.View();
            
            return view.ends_with(".actor.json") || view.ends_with(".actor") || view.ends_with(".component.json") || view.ends_with(".component");
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

    void RenderEntries(ContentBrowserState& state, String& selectedEntry, PopupRequestState& requests, const String& searchQuery)
    {
        if (!RefreshDirectoryCache(state))
        {
            DrawEmptyStateMessage(state.error.IsEmpty() ? "Unable to open directory." : state.error.c_str());
            return;
        }

        ScopedColor bg(ImGuiCol_ChildBg, ContentBackground);
        if (!ImGui::BeginChild("ContentBrowserGrid", ImVec2(0, 0), true))
            return;

        // Clic droit sur le fond
        if (ImGui::BeginPopupContextWindow("ContentBrowserBackgroundContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Create script..."))
                RequestCreateScript(requests);
            if (ImGui::MenuItem("Create folder..."))
                RequestCreateFolder(requests, state.current);
            
            ImGui::EndPopup();
        }

        const float cellSize = ThumbnailSize + ThumbnailPadding;
        const float available = ImGui::GetContentRegionAvail().x;
        const int columns = std::max(1, static_cast<int>(available / cellSize));

        if (ImGui::BeginTable("ContentBrowserEntries", columns, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings))
        {
            for (const auto& entry : state.cache.entries)
            {
                if (!MatchesSearch(entry.name, searchQuery))
                    continue;

                ImGui::TableNextColumn();
                ScopedID id(entry.SelectionKey().c_str());
                ImGui::BeginGroup();

                const bool isSelected = (selectedEntry == entry.SelectionKey());
                const ImVec2 btnSize(ThumbnailSize, ThumbnailSize);

                // Style optimisé (push/pop en RAII)
                const ImVec4 base = isSelected ? ImVec4(0.20f, 0.35f, 0.60f, 0.95f) : ImGui::GetStyleColorVec4(ImGuiCol_Button);

                ScopedColor button(ImGuiCol_Button, base);
                ScopedColor hover(ImGuiCol_ButtonHovered, AdjustColor(base, 0.08f));
                ScopedColor active(ImGuiCol_ButtonActive, AdjustColor(base, 0.14f));
                ScopedStyle padding(ImGuiStyleVar_FramePadding, ImVec2(12, 12));

                // ────── Bouton principal ──────
                if (ImGui::Button(GetIcon(entry.type), btnSize))
                {
                    if (entry.IsDirectory())
                    {
                        state.current = entry.path;
                        state.cache.dirty = true;
                        selectedEntry.Clear();
                    }
                    else
                    {
                        selectedEntry = entry.SelectionKey();
                    }
                }

                // Double-clic : ouvre TOUJOURS Actor Editor
                if (IsItemDoubleClicked(ImGuiMouseButton_Left))
                {
                    selectedEntry = entry.SelectionKey();
                    if (state.openActorEditorCallback)
                        state.openActorEditorCallback(entry.path);
                }

                // Popup clic droit spécifique à l’item
                if (ImGui::BeginPopupContextItem("ContentBrowserEntryContext"))
                {
                    const bool hasSource = entry.HasSource();

                    ShowActionTooltip(entry.name, {
                        {"Open header", [&]() { RequestOpenScriptFiles(state, entry, true, false); }},
                        {"Open source", [&]() { if (hasSource) RequestOpenScriptFiles(state, entry, false, true); }},
                        {"Open both", [&]() { RequestOpenScriptFiles(state, entry, true, true); }}
                    });

                    ImGui::EndPopup();
                }

                // Tooltip léger
                if (ImGui::IsItemHovered(TooltipHoverFlags) && ImGui::BeginTooltip())
                {
                    ImGui::TextUnformatted(entry.name.c_str());
                    ImGui::Separator();
                    ImGui::Text("Path: %s", entry.path.generic_string().c_str());
                    ImGui::EndTooltip();
                }

                // Légende
                if (isSelected)
                {
                    ScopedColor textColor(ImGuiCol_Text, ImVec4(0.95f, 0.85f, 0.40f, 1));
                    ImGui::TextWrapped("%s", entry.name.c_str());
                }
                else
                    ImGui::TextWrapped("%s", entry.name.c_str());

                ImGui::EndGroup();
            }

            ImGui::EndTable();
        }
        else
        {
            DrawEmptyStateMessage("Nothing to show.");
        }

        ImGui::EndChild();
    }
}

