#include "Gui/Panels/ContentBrowser/ContentBrowserState.h"
#include "Gui/Panels/ContentBrowser/ContentEntry.h"
#include "Gui/Dialogs/CreateFolderDialog.h"
#include "Gui/Dialogs/CreatePrefabDialog.h"
#include "Gui/Dialogs/CreateScriptDialog.h"
#include "Gui/Dialogs/CreateSpriteAtlasDialog.h"
#include "Gui/Dialogs/RenameEntryDialog.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Widgets/GuiWidgetLibrary.h"
#include "Utils/EditorUtils.h"
#include "imgui.h"
#include <filesystem>
#include <unordered_map>
#include <ranges>

#include "Utils/FilesUtils.h"
#include "Utils/StringUtils.h"

namespace BixEngine::Gui
{
    using namespace Utils;

    // ─────────────────────────────────────────────
    // 🔧  Fichiers et helpers
    // ─────────────────────────────────────────────

    static bool DeleteScriptFiles(const ContentEntry& entry, String& error)
    {
        const auto tryRemove = [&](const path& path)
        {
            return path.empty() || FileUtils::TryRemove(path, false, error);
        };

        return tryRemove(entry.headerPath) && tryRemove(entry.sourcePath);
    }

    static bool RefreshDirectoryCache(ContentBrowserState& state)
    {
        if (!state.cache.dirty && state.cache.directory == state.current)
            return true;

        state.cache.entries.clear();
        state.cache.dirty = false;
        state.cache.directory = state.current;
        std::unordered_map<String, ContentEntry> scriptGroups;

        std::error_code error;
        for (const auto& entry : fs::directory_iterator(state.current, error))
        {
            const path& path = entry.path();

            if (entry.is_directory())
            {
                state.cache.entries.push_back({
                    .type = ContentType::Directory,
                    .path = path,
                    .name = path.filename().generic_string()
                });
                continue;
            }

            const String ext = StringUtils::ToLowerCopy(path.extension().generic_string());
            if (ext == ".h" || ext == ".cpp")
            {
                auto& group = scriptGroups[StringUtils::ToLowerCopy(path.stem().generic_string())];
                group.type = ContentType::Script;
                group.path = path.parent_path();
                group.name = path.stem().generic_string();

                if (ext == ".h")
                {
                    group.headerPath = path;
                }
                else
                {
                    group.sourcePath = path;
                }

                continue;
            }

            auto type = ContentType::File;
            if (ext == ".bixactor")
            {
                type = ContentType::ActorPrefab;
            }
            else if (ext == ".bixcomponent")
            {
                type = ContentType::ComponentPrefab;
            }
            else if (ext == ".atlas")
            {
                type = ContentType::SpriteAtlas;
            }

            state.cache.entries.push_back({
                .type = type,
                .path = path,
                .name = path.filename().generic_string()
            });
        }

        for (auto& [_, script] : scriptGroups)
        {
            state.cache.entries.push_back(std::move(script));
        }

        std::ranges::sort(state.cache.entries, [](const ContentEntry& a, const ContentEntry& b)
        {
            const int pa = GetSortPriority(a.type);
            const int pb = GetSortPriority(b.type);
            return pa == pb ? FileUtils::CaseInsensitiveLess(a.name, b.name) : pa < pb;
        });

        return !error;
    }

    // ─────────────────────────────────────────────
    // 📁 Popups (création / renommage)
    // ─────────────────────────────────────────────
    void RenderPopups(ContentBrowserState& state, String& selected, PopupRequestState& requests)
    {
        static CreatePrefabDialog prefab(state, selected);
        static CreateScriptDialog script(state, selected);
        static CreateFolderDialog folder(state, selected);
        static CreateSpriteAtlasDialog atlas(state, selected);
        static RenameEntryDialog rename(state, selected);

        if (requests.createPrefab)
        {
            prefab.Open();
            requests.createPrefab = false;
        }

        if (requests.createScript)
        {
            script.Open();
            requests.createScript = false;
        }
        if (requests.createFolder)
        {
            folder.Open(state.current);
            requests.createFolder = false;
        }
        if (requests.createSpriteAtlas)
        {
            atlas.Open(state.current);
            requests.createSpriteAtlas = false;
        }
        if (requests.renameEntry)
        {
            rename.Open(requests.renameTarget, requests.renameSecondaryTarget, requests.renameTargetIsScriptGroup);
            requests.renameEntry = false;
        }

        if (prefab.IsOpen())
            prefab.Render();

        if (script.IsOpen())
            script.Render();

        if (folder.IsOpen())
            folder.Render();

        if (atlas.IsOpen())
            atlas.Render();

        if (rename.IsOpen())
            rename.Render();
    }

    // ─────────────────────────────────────────────
    // 🧭 Header
    // ─────────────────────────────────────────────
    void RenderHeader(ContentBrowserState& state, String& selected, char (&search)[256])
    {
        ScopedColor bg(ImGuiCol_ChildBg, Theme::HeaderBackground);

        if (!ImGui::BeginChild("ContentHeader", ImVec2(0, 72), true))
        {
            ImGui::EndChild();
            return;
        }

        ImGui::PushID("Header");

        Widgets::DrawPanelHeader({.title = "Content Browser"});
        Widgets::PanelToolbar bar{};

        bar.AddLeft([&]
        {
            ScopedStyle s1(ImGuiStyleVar_FrameRounding, 5.f);
            ScopedStyle s2(ImGuiStyleVar_ItemSpacing, ImVec2(5, 0));

            if (ImGui::Button("Content", {76, 24}))
                state.current = state.root;
            selected.Clear();

            ImGui::SameLine();
            ImGui::BeginDisabled(state.current == state.root);

            if (ImGui::Button("Up", {76, 24}))
            {
                auto parent = state.current.parent_path();
                auto rel = parent.lexically_relative(state.root);

                if (rel.empty() || rel.generic_string().starts_with(".."))
                {
                    state.current = state.root;
                }
                else
                {
                    state.current = parent;
                }

                selected.Clear();
            }

            ImGui::EndDisabled();
        });

        bar.AddLeft([&]
        {
            ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(3, 0));
            ScopedStyle pad(ImGuiStyleVar_FramePadding, ImVec2(6, 4));

            path path = state.root;
            std::vector<fs::path> segs;

            for (auto& p : state.current.lexically_relative(state.root))
            {
                segs.push_back(p);
            }

            if (ImGui::Button("Content##Breadcrumb"))
                state.current = state.root;
            selected.Clear();

            for (size_t i = 0; i < segs.size(); ++i)
            {
                ImGui::SameLine();
                ImGui::TextUnformatted("›");
                ImGui::SameLine();
                path /= segs[i];
                ScopedID id(static_cast<int>(i));
                if (ImGui::Button(segs[i].string().c_str()))
                {
                    state.current = path;
                    selected.Clear();
                }
            }
        });

        bar.AddRight([&]
        {
            ScopedStyle s(ImGuiStyleVar_FrameRounding, 4);
            ImGui::SetNextItemWidth(200);
            SearchInput("##Search", search, IM_ARRAYSIZE(search), "Search...");
        });

        bar.Commit();
        ImGui::PopID();
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────
    // 🌳 Directory Tree
    // ─────────────────────────────────────────────
    void RenderDirectoryTree(ContentBrowserState& state, String& selected)
    {
        static std::unordered_map<std::string, std::vector<path>> cache;
        static bool expand = false, collapse = false;

        ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
        if (!ImGui::BeginChild("Tree", {Theme::ContentTreeWidth, 0}, true))
        {
            ImGui::EndChild();
            return;
        }

        ScopedColor bg(ImGuiCol_ChildBg, Theme::ContentTreeBackground);

        if (ImGui::Button("➕"))
        {
            expand = true;
            collapse = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("➖"))
        {
            collapse = true;
            expand = false;
        }

        ImGui::SameLine();
        if (ImGui::Button("🔄"))
            cache.clear();

        ImGui::Separator();

        if (ImGui::BeginChild("TreeInner", {0, 0}, false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            const auto draw = [&](auto&& self, const path& dir)-> void
            {
                std::error_code err;

                const bool isSel = fs::equivalent(dir, state.current, err);
                const String id = dir.generic_string();

                if (expand || collapse)
                    ImGui::SetNextItemOpen(expand, ImGuiCond_Always);

                bool open = ImGui::TreeNodeEx(
                    id.c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth |
                    (isSel ? ImGuiTreeNodeFlags_Selected : 0),
                    "%s  %s",
                    GetIcon(ContentType::Directory),
                    dir.filename().string().c_str());


                if (ImGui::IsItemClicked())
                    state.current = dir;
                selected.Clear();

                if (open)
                {
                    auto& children = cache[id.c_str()];
                    if (children.empty())
                    {
                        for (auto& e : fs::directory_iterator(dir))
                        {
                            if (e.is_directory())
                                children.push_back(e.path());
                        }

                        std::ranges::sort(children, [](auto& a, auto& b)
                        {
                            return FileUtils::CaseInsensitiveLess(a.filename().string(), b.filename().string());
                        });
                    }

                    for (auto& c : children)
                    {
                        self(self, c);
                    }

                    ImGui::TreePop();
                }
            };

            if (fs::exists(state.root))
            {
                draw(draw, state.root);
            }
            else
            {
                DrawEmptyStateMessage("Content directory not found.");
            }
        }

        ImGui::EndChild();
        ImGui::EndChild();
        expand = collapse = false;
    }

    // ─────────────────────────────────────────────
    // 🗂️ Grid View
    // ─────────────────────────────────────────────
    void RenderEntries(ContentBrowserState& state, String& selected, PopupRequestState& req, const String& query)
    {
        if (!RefreshDirectoryCache(state))
        {
            DrawEmptyStateMessage("Unable to read directory.");
            return;
        }

        ScopedColor bg(ImGuiCol_ChildBg, Theme::ContentBackground);
        if (!ImGui::BeginChild("Grid", {0, 0}, true))
        {
            ImGui::EndChild();
            return;
        }

        if (ImGui::BeginPopupContextWindow(
            "GridCtx", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("New Script"))
            {
                req.createScript = true;
            }

            if (ImGui::MenuItem("New Prefab"))
            {
                req.createPrefab = true;
            }

            if (ImGui::MenuItem("New Folder"))
            {
                req.createFolder = true;
            }

            if (ImGui::MenuItem("New Sprite Atlas"))
            {
                req.createSpriteAtlas = true;
            }

            ImGui::EndPopup();
        }

        constexpr float cell = Theme::ThumbnailSize + Theme::ThumbnailPadding;
        const int cols = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cell));

        if (ImGui::BeginTable("Entries", cols, ImGuiTableFlags_SizingFixedFit))
        {
            for (const auto& entry : state.cache.entries)
            {
                if (!StringUtils::MatchesSearch(entry.name, query))
                    continue;

                ImGui::TableNextColumn();
                ScopedID id(entry.SelectionKey().c_str());

                const bool sel = (selected == entry.SelectionKey());
                const ImVec4 base = sel ? ImVec4(0.2f, 0.35f, 0.6f, 0.9f) : ImGui::GetStyleColorVec4(ImGuiCol_Button);

                ScopedColor b(ImGuiCol_Button, base);

                if (ImGui::Button(GetIcon(entry.type), {Theme::ThumbnailSize, Theme::ThumbnailSize}))
                {
                    if (entry.IsDirectory())
                    {
                        state.current = entry.path;
                        state.cache.dirty = true;
                        selected.Clear();
                    }
                    else
                    {
                        selected = entry.SelectionKey();
                    }
                }

                if (IsItemDoubleClicked(ImGuiMouseButton_Left))
                {
                    selected = entry.SelectionKey();
                    if (entry.IsDirectory())
                    {
                        state.current = entry.path;
                        state.cache.dirty = true;
                        selected.Clear();
                    }
                    else if (entry.IsScript())
                    {
                        std::vector<path> files;
                        if (entry.HasHeader())
                            files.push_back(entry.headerPath);

                        if (entry.HasSource())
                            files.push_back(entry.sourcePath);

                        for (auto& f : files)
                        {
                            EditorUtils::OpenFileInCodeEditor(f);
                        }
                    }
                    else if (entry.IsPrefab() || entry.IsSpriteAtlas())
                        if (state.openAssetEditorCallback) state.openAssetEditorCallback(entry.path);
                }

                if (ImGui::BeginPopupContextItem("EntryCtx"))
                {
                    if (ImGui::MenuItem("Rename"))
                    {
                        req.renameTarget = entry.path;
                        req.renameEntry = true;
                    }

                    if (ImGui::MenuItem("Delete"))
                    {
                        String err;
                        FileUtils::TryRemove(entry.path, entry.IsDirectory(), err);
                        state.cache.dirty = true;
                    }

                    if (ImGui::MenuItem("Show in Explorer"))
                    {
                        EditorUtils::ShowPathInExplorer(entry.path, entry.IsDirectory());
                    }

                    ImGui::EndPopup();
                }

                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", entry.name.c_str());
                if (sel)
                {
                    ScopedColor t(ImGuiCol_Text, ImVec4(1, 0.85f, 0.3f, 1));
                    ImGui::TextWrapped("%s", entry.name.c_str());
                }
                else ImGui::TextWrapped("%s", entry.name.c_str());
            }
            ImGui::EndTable();
        }

        ImGui::EndChild();
    }
}
