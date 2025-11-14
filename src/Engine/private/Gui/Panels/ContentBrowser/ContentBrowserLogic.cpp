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
#include "Utils/FilesUtils.h"
#include "Utils/StringUtils.h"

#include <filesystem>
#include <unordered_map>
#include <ranges>
#include <imgui.h>

namespace BixEngine::Gui
{
    using namespace Utils;
    namespace fs = std::filesystem;

    // ─────────────────────────────────────────────
    // 🔧 Helpers
    // ─────────────────────────────────────────────

    static bool DeleteScriptFiles(const ContentEntry& entry, String& error)
    {
        auto tryRemove = [&](const fs::path& p)
        {
            return p.empty() || FileUtils::TryRemove(p, false, error);
        };

        return tryRemove(entry.headerPath) && tryRemove(entry.sourcePath);
    }

    // ─────────────────────────────────────────────
    // 📂 Cache du répertoire
    // ─────────────────────────────────────────────

    static bool RefreshDirectoryCache(ContentBrowserState& state)
    {
        if (!state.cache.dirty && state.cache.directory == state.current)
            return true;

        state.cache.entries.clear();
        state.cache.dirty = false;
        state.cache.directory = state.current;

        std::unordered_map<String, ContentEntry> scriptGroups;
        std::error_code error;

        for (auto& entry : fs::directory_iterator(state.current, error))
        {
            const fs::path p = entry.path();

            // ──────────────── Dossier ────────────────
            if (entry.is_directory())
            {
                state.cache.entries.push_back(ContentEntry{
                    .name = p.filename().generic_string(),
                    .path = p,
                    .type = ContentType::Directory,
                    .extension = "",
                    .headerPath = {},
                    .sourcePath = {}
                });
                continue;
            }

            // ──────────────── Extension ────────────────
            const String ext = StringUtils::ToLowerCopy(p.extension().generic_string());

            // ──────────────── Scripts (.h/.cpp) ────────────────
            if (ext == ".h" || ext == ".cpp")
            {
                auto key = StringUtils::ToLowerCopy(p.stem().generic_string());
                auto& group = scriptGroups[key];

                group.name = p.stem().generic_string();
                group.path = p.parent_path();
                group.type = ContentType::Script;

                if (ext == ".h") group.headerPath = p;
                else group.sourcePath = p;

                continue;
            }

            // ──────────────── Type de fichier ────────────────
            ContentType type = ContentType::File;

            if (ext == ".bixactor")      type = ContentType::ActorPrefab;
            else if (ext == ".bixcomponent") type = ContentType::ComponentPrefab;
            else if (ext == ".atlas")    type = ContentType::SpriteAtlas;

            // ❗ FIX : avant c’était `.type = Directory` → FAUX
            state.cache.entries.push_back(ContentEntry{
                .name = p.filename().generic_string(),
                .path = p,
                .type = type,
                .extension = ext,
                .headerPath = {},
                .sourcePath = {}
            });
        }

        // Ajouter les scripts regroupés
        for (auto& [_, script] : scriptGroups)
            state.cache.entries.push_back(std::move(script));

        // Tri
        std::ranges::sort(state.cache.entries, [](const ContentEntry& a, const ContentEntry& b)
        {
            int pa = GetSortPriority(a.type);
            int pb = GetSortPriority(b.type);

            if (pa != pb) return pa < pb;
            return FileUtils::CaseInsensitiveLess(a.name, b.name);
        });

        return !error;
    }

    // ─────────────────────────────────────────────
    // 📦 Popups
    // ─────────────────────────────────────────────

    void RenderPopups(ContentBrowserState& state, String& selected, PopupRequestState& req)
    {
        static CreatePrefabDialog prefab(state, selected);
        static CreateScriptDialog script(state, selected);
        static CreateFolderDialog folder(state, selected);
        static CreateSpriteAtlasDialog atlas(state, selected);
        static RenameEntryDialog rename(state, selected);

        if (req.createPrefab) { prefab.Open(); req.createPrefab = false; }
        if (req.createScript) { script.Open(); req.createScript = false; }
        if (req.createFolder) { folder.Open(state.current); req.createFolder = false; }
        if (req.createSpriteAtlas) { atlas.Open(state.current); req.createSpriteAtlas = false; }
        if (req.renameEntry)
        {
            rename.Open(req.renameTarget, req.renameSecondaryTarget, req.renameTargetIsScriptGroup);
            req.renameEntry = false;
        }

        if (prefab.IsOpen()) prefab.Render();
        if (script.IsOpen()) script.Render();
        if (folder.IsOpen()) folder.Render();
        if (atlas.IsOpen()) atlas.Render();
        if (rename.IsOpen()) rename.Render();
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
        Widgets::DrawPanelHeader({ .title = "Content Browser" });

        Widgets::PanelToolbar bar{};

        // Boutons Content + Up
        bar.AddLeft([&]
        {
            if (ImGui::Button("Content"))
                state.current = state.root;

            selected.Clear();

            ImGui::SameLine();
            ImGui::BeginDisabled(state.current == state.root);

            if (ImGui::Button("Up"))
            {
                auto parent = state.current.parent_path();
                auto rel = parent.lexically_relative(state.root);

                state.current = (rel.empty() || rel.string().starts_with(".."))
                    ? state.root
                    : parent;

                selected.Clear();
            }

            ImGui::EndDisabled();
        });

        // Fil d'Ariane
        bar.AddLeft([&]
        {
            fs::path cur = state.root;
            std::vector<fs::path> segs;

            for (auto& seg : state.current.lexically_relative(state.root))
                segs.push_back(seg);

            if (ImGui::Button("Content##Breadcrumb"))
            {
                state.current = state.root;
                selected.Clear();
            }

            for (size_t i = 0; i < segs.size(); i++)
            {
                ImGui::SameLine();
                ImGui::Text(">");
                ImGui::SameLine();

                cur /= segs[i];
                ScopedID id((int)i);

                if (ImGui::Button(segs[i].string().c_str()))
                {
                    state.current = cur;
                    selected.Clear();
                }
            }
        });

        // Recherche
        bar.AddRight([&]
        {
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
        static std::unordered_map<std::string, std::vector<fs::path>> cache;
        static bool expand = false, collapse = false;

        if (!ImGui::BeginChild("Tree", { Theme::ContentTreeWidth, 0 }, true))
        {
            ImGui::EndChild();
            return;
        }

        if (ImGui::Button("+")) { expand = true; collapse = false; }
        ImGui::SameLine();
        if (ImGui::Button("-")) { collapse = true; expand = false; }
        ImGui::SameLine();
        if (ImGui::Button("Reload")) cache.clear();

        ImGui::Separator();

        if (ImGui::BeginChild("TreeInner"))
        {
            std::function<void(const fs::path&)> draw;
            draw = [&](const fs::path& dir)
            {
                std::error_code ec;
                bool isSel = fs::equivalent(dir, state.current, ec);
                String id = dir.generic_string();

                if (expand || collapse)
                    ImGui::SetNextItemOpen(expand, ImGuiCond_Always);

                bool open = ImGui::TreeNodeEx(
                    id.c_str(),
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_SpanFullWidth |
                    (isSel ? ImGuiTreeNodeFlags_Selected : 0),
                    "%s  %s",
                    GetIcon(ContentType::Directory),
                    dir.filename().string().c_str());

                if (ImGui::IsItemClicked())
                {
                    state.current = dir;
                    selected.Clear();
                }

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
                        draw(c);

                    ImGui::TreePop();
                }
            };

            if (fs::exists(state.root))
                draw(state.root);
            else
                DrawEmptyStateMessage("Content directory not found.");
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

        if (!ImGui::BeginChild("Grid", { 0,0 }, true))
        {
            ImGui::EndChild();
            return;
        }

        // Menu clic droit
        if (ImGui::BeginPopupContextWindow("GridCtx"))
        {
            if (ImGui::MenuItem("New Script")) req.createScript = true;
            if (ImGui::MenuItem("New Prefab")) req.createPrefab = true;
            if (ImGui::MenuItem("New Folder")) req.createFolder = true;
            if (ImGui::MenuItem("New Sprite Atlas")) req.createSpriteAtlas = true;
            ImGui::EndPopup();
        }

        const float cell = Theme::ThumbnailSize + Theme::ThumbnailPadding;
        const int cols = std::max(1, int(ImGui::GetContentRegionAvail().x / cell));

        if (ImGui::BeginTable("Entries", cols, ImGuiTableFlags_SizingFixedFit))
        {
            for (auto& entry : state.cache.entries)
            {
                if (!StringUtils::MatchesSearch(entry.name, query))
                    continue;

                ImGui::TableNextColumn();
                ScopedID id(entry.SelectionKey().c_str());

                bool sel = (selected == entry.SelectionKey());
                ImVec4 base = sel ? ImVec4(0.2f,0.35f,0.6f,0.9f) : ImGui::GetStyleColorVec4(ImGuiCol_Button);

                ScopedColor b(ImGuiCol_Button, base);

                if (ImGui::Button(GetIcon(entry.type), { Theme::ThumbnailSize, Theme::ThumbnailSize }))
                {
                    if (entry.IsDirectory())
                    {
                        state.current = entry.path;
                        state.cache.dirty = true;
                        selected.Clear();
                    }
                    else selected = entry.SelectionKey();
                }

                // Double click
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
                        std::vector<fs::path> files;
                        if (entry.HasHeader()) files.push_back(entry.headerPath);
                        if (entry.HasSource()) files.push_back(entry.sourcePath);

                        for (auto& f : files)
                            EditorUtils::OpenFileInCodeEditor(f);
                    }
                    else if (entry.IsPrefab() || entry.IsSpriteAtlas())
                    {
                        if (state.openAssetEditorCallback)
                            state.openAssetEditorCallback(entry.path);
                    }
                }

                // Menu contextuel par item
                if (ImGui::BeginPopupContextItem("EntryCtx"))
                {
                    const bool isScript = entry.IsScript();

                    if (ImGui::MenuItem("Rename"))
                    {
                        if (isScript)
                        {
                            req.renameTarget = entry.headerPath;
                            req.renameSecondaryTarget = entry.sourcePath;
                            req.renameTargetIsScriptGroup = true;
                        }
                        else
                        {
                            req.renameTarget = entry.path;
                            req.renameSecondaryTarget.clear();
                            req.renameTargetIsScriptGroup = false;
                        }

                        req.renameEntry = true;
                    }

                    if (ImGui::MenuItem("Delete"))
                    {
                        String err;
                        const bool removed = isScript
                            ? DeleteScriptFiles(entry, err)
                            : FileUtils::TryRemove(entry.path, entry.IsDirectory(), err);

                        if (!removed)
                        {
                            state.error = err;
                        }
                        else
                        {
                            state.error.Clear();
                            selected.Clear();
                            state.cache.dirty = true;
                        }
                    }

                    if (ImGui::MenuItem("Show in Explorer"))
                    {
                        fs::path targetPath = entry.path;
                        bool targetIsDirectory = entry.IsDirectory();

                        if (isScript)
                        {
                            targetPath = !entry.headerPath.empty() ? entry.headerPath : entry.sourcePath;
                            targetIsDirectory = false;

                            if (targetPath.empty())
                                targetPath = entry.path;
                        }

                        EditorUtils::ShowPathInExplorer(targetPath, targetIsDirectory);
                    }

                    ImGui::EndPopup();
                }

                // Label
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("%s", entry.name.c_str());

                if (sel)
                {
                    ScopedColor t(ImGuiCol_Text, ImVec4(1,0.85f,0.3f,1));
                    ImGui::TextWrapped("%s", entry.name.c_str());
                }
                else ImGui::TextWrapped("%s", entry.name.c_str());
            }

            ImGui::EndTable();
        }

        ImGui::EndChild();
    }
}
