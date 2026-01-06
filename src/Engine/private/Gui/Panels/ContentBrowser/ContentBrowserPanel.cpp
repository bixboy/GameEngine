#include "Gui/Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Gui/Utils/GuiHelpers.h"
#include "Gui/Utils/ContentBrowserUtils.h"
#include "Utils/FileIO/FilesUtils.h" 
#include "Utils/String/StringUtils.h"
#include "Utils/Editor/EditorUtils.h"
#include "Debug/Logger.h"
#include "Gui/Widgets/Builders/PanelBuilder.h"
#include "Gui/Dialogs/CreateFolderDialog.h"
#include "Gui/Dialogs/CreatePrefabDialog.h"
#include "Gui/Dialogs/CreateScriptDialog.h"
#include "Gui/Dialogs/CreateSpriteAtlasDialog.h"
#include "Gui/Dialogs/CreateAudioContainerDialog.h"
#include "Gui/Dialogs/RenameEntryDialog.h"
#include "Gui/Core/EditorPreferences.h"
#include <imgui.h>
#include <filesystem>
#include <ranges>
#include <algorithm>
#include <unordered_map>
#include <functional>


namespace BixEngine::Gui
{
    // ==================================================================================
    //                           CONSTRUCTOR / DESTRUCTOR
    // ==================================================================================

    ContentBrowserPanel* ContentBrowserPanel::activeInstance_ = nullptr;
    
    ContentBrowserPanel::ContentBrowserPanel(const DefaultEngineGuiContext& context) : GuiPanelBase("Content Browser")
    {
        state_.openScriptFilesCallback = context.openScriptFilesInEditor;
        state_.openAssetEditorCallback = context.openAssetInEditor;

        if (state_.root.empty()) 
            state_.root = "Assets"; 
        
        state_.current = state_.root;

        m_LastImportTime = std::chrono::steady_clock::now();
        activeInstance_ = this;
    }

    ContentBrowserPanel::~ContentBrowserPanel()
    {
        if (activeInstance_ == this)
            activeInstance_ = nullptr;
    }

    ContentBrowserPanel* ContentBrowserPanel::GetActiveInstance() noexcept
    {
        return activeInstance_;
    }

    // ==================================================================================
    //                                  MAIN LOOP
    // ==================================================================================

    void ContentBrowserPanel::Draw()
    {
        if (m_PendingRefresh)
        {
            m_RefreshTimer -= ImGui::GetIO().DeltaTime;

            if (m_RefreshTimer <= 0.0f)
            {
                state_.cache.dirty = true;
                selectedEntry_.clear();
                m_PendingRefresh = false;
                m_RefreshTimer = 0.0f;
            }
        }

        if (!ContentBrowserUtils::EnsureContentBrowserInitialized(state_))
        {
            GuiUtils::DrawEmptyStateMessage("Content folder invalid.");
            return;
        }

        EnsureValidDirectory();

        DrawHeader();
        
        ImGui::Spacing();
        
        DrawBody();

        DrawPopups();
    }

    void ContentBrowserPanel::DrawBody()
    {
        GuiUtils::ScopedStyle spacing(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 6.f));
        
        DrawDirectoryTree();
        
        ImGui::SameLine();

        const String searchQuery(searchBuffer_);
        DrawEntries(searchQuery);
    }

    // ==================================================================================
    //                                  SUB-PANELS (UI)
    // ==================================================================================

    void ContentBrowserPanel::DrawHeader()
    {
        GuiUtils::ScopedColor bg(ImGuiCol_ChildBg, Theme::HeaderBackground);

        if (!ImGui::BeginChild("ContentHeader", ImVec2(0, 72), true))
        {
            ImGui::EndChild();
            return;
        }

        ImGui::PushID("Header");
        Widgets::DrawPanelHeader({ .title = "Content Browser" });

        Widgets::Layout::PanelToolbar bar{};

        bar.AddLeft([&]
        {
            if (ImGui::Button("Content"))
            {
                state_.current = state_.root;
                selectedEntry_.clear();
            }

            ImGui::SameLine();
            
            // Bouton Up (Parent)
            ImGui::BeginDisabled(state_.current == state_.root);
            if (ImGui::Button("Up"))
            {
                auto parent = state_.current.parent_path();
                auto rel = parent.lexically_relative(state_.root);
                
                state_.current = (rel.empty() || rel.string().starts_with("..")) ? state_.root : parent;
                
                selectedEntry_.clear();
            }
            
            ImGui::EndDisabled();
        });

        bar.AddLeft([&]
        {
            path cur = state_.root;
            
            std::vector<path> segs;
            for (auto& seg : state_.current.lexically_relative(state_.root))
                segs.push_back(seg);

            if (ImGui::Button("Content##Breadcrumb"))
            {
                state_.current = state_.root;
                selectedEntry_.clear();
            }

            for (size_t i = 0; i < segs.size(); i++)
            {
                ImGui::SameLine(); ImGui::Text(">"); ImGui::SameLine();
                cur /= segs[i];
                
                GuiUtils::ScopedID id(static_cast<int>(i));

                if (ImGui::Button(segs[i].string().c_str()))
                {
                    state_.current = cur;
                    selectedEntry_.clear();
                }
            }
        });

        bar.AddRight([&]
        {
            ImGui::SetNextItemWidth(200);
            GuiUtils::SearchInput("##Search", searchBuffer_, IM_ARRAYSIZE(searchBuffer_), "Search...");
        });

        bar.Commit();
        ImGui::PopID();
        ImGui::EndChild();
    }

    void ContentBrowserPanel::DrawDirectoryTree()
    {
        static std::unordered_map<std::string, std::vector<path>> treeCache;
        static bool expand = false, collapse = false;

        const auto& settings = EditorSettings::Get();
        if (!ImGui::BeginChild("Tree", { settings.ContentTreeWidth, 0 }, true))
        {
            ImGui::EndChild();
            return;
        }

        if (ImGui::Button("+"))
        {
            expand = true;
            collapse = false;
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("-"))
        {
            collapse = true;
            expand = false;
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Reload"))
            treeCache.clear();

        ImGui::Separator();

        if (ImGui::BeginChild("TreeInner"))
        {
            std::function<void(const path&)> drawNode;

            drawNode = [&](const path& dir)
            {
                std::error_code ec;
                bool isSel = fs::equivalent(dir, state_.current, ec);
                String id = dir.generic_string();

                if (expand || collapse)
                    ImGui::SetNextItemOpen(expand, ImGuiCond_Always);

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
                if (isSel)
                    flags |= ImGuiTreeNodeFlags_Selected;

                bool open = ImGui::TreeNodeEx(id.c_str(), flags, "%s  %s", 
                    GetIcon(ContentType::Directory), dir.filename().string().c_str());

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                    {
                        path sourcePath(static_cast<const char*>(payload->Data));
                        path destPath = dir / sourcePath.filename();
                        
                        String err;
                        if (!Utils::FileUtils::TryRename(sourcePath, destPath, false, err))
                        {
                            state_.error = err;
                        }
                        else
                        {
                             state_.cache.dirty = true;
                             selectedEntry_.clear();
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                if (ImGui::IsItemClicked())
                {
                    state_.current = dir;
                    selectedEntry_.clear();
                }

                if (open)
                {
                    auto& children = treeCache[id.c_str()];
                    if (children.empty())
                    {
                        for (auto& e : fs::directory_iterator(dir))
                        {
                            if (e.is_directory())
                                children.push_back(e.path());
                        }
                        std::ranges::sort(children, [](auto& a, auto& b)
                        {
                            return Utils::FileUtils::CaseInsensitiveLess(a.filename().string(), b.filename().string());
                        });
                    }

                    for (auto& c : children) 
                        drawNode(c);
                    
                    ImGui::TreePop();
                }
            };

            if (fs::exists(state_.root))
            {
                drawNode(state_.root);
            }
            else
            {
                GuiUtils::DrawEmptyStateMessage("Content directory not found.");
            }
        }

        ImGui::EndChild();
        ImGui::EndChild();

        expand = collapse = false;
    }

    void ContentBrowserPanel::DrawEntries(const String& query)
    {
        if (!RefreshDirectoryCache())
        {
            GuiUtils::DrawEmptyStateMessage("Unable to read directory.");
            return;
        }

        if (!ImGui::BeginChild("Grid", { 0,0 }, true)) {
            ImGui::EndChild();
            return;
        }

        // Menu Contextuel Global (Clic droit dans le vide)
        if (ImGui::BeginPopupContextWindow("GridCtx"))
        {
            if (ImGui::MenuItem("New Script"))
                popupRequests_.createScript = true;
            
            if (ImGui::MenuItem("New Prefab"))
                popupRequests_.createPrefab = true;
            
            if (ImGui::MenuItem("New Folder"))
                popupRequests_.createFolder = true;
            
            if (ImGui::MenuItem("New Sprite Atlas"))
                popupRequests_.createSpriteAtlas = true;
            
            if (ImGui::MenuItem("New Audio Container"))
                popupRequests_.createAudioContainer = true;
            
            ImGui::EndPopup();
        }

        const auto& settings = EditorSettings::Get();
        const float cell = settings.ContentThumbnailSize + settings.ContentThumbnailPadding;
        const int cols = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cell));

        if (ImGui::BeginTable("Entries", cols, ImGuiTableFlags_SizingFixedFit))
        {
            for (auto& entry : state_.cache.entries)
            {
                if (!StringUtils::Utilities::MatchesSearch(entry.name, query))
                    continue;

                ImGui::TableNextColumn();
                GuiUtils::ScopedID id(entry.SelectionKey().c_str());

                bool isSelected = (selectedEntry_ == entry.SelectionKey());
                ImVec4 baseColor = isSelected ? ImVec4(0.2f,0.35f,0.6f,0.9f) : ImGui::GetStyleColorVec4(ImGuiCol_Button);
                GuiUtils::ScopedColor b(ImGuiCol_Button, baseColor);

                if (ImGui::Button(GetIcon(entry.type), { settings.ContentThumbnailSize, settings.ContentThumbnailSize }))
                {
                    if (entry.IsDirectory())
                    {
                        state_.current = entry.path;
                        state_.cache.dirty = true;
                        selectedEntry_.clear();
                    }
                    else
                    {
                        selectedEntry_ = entry.SelectionKey();
                    }
                }

                if (ImGui::BeginDragDropSource())
                {
                    const std::string pathStr = entry.path.string();
                    ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);
                    ImGui::Text("%s", entry.name.c_str());
                    ImGui::EndDragDropSource();
                }

                if (entry.IsDirectory())
                {
                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
                        {
                            path sourcePath(static_cast<const char*>(payload->Data));
                            path destPath = entry.path / sourcePath.filename();
                            String err;
                            if (!Utils::FileUtils::TryRename(sourcePath, destPath, false, err))
                            {
                                state_.error = err;
                            }
                            else
                            {
                                 state_.cache.dirty = true;
                                 selectedEntry_.clear();
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                }

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    selectedEntry_ = entry.SelectionKey();
                    
                    if (entry.IsDirectory())
                    {
                        state_.current = entry.path;
                        state_.cache.dirty = true;
                        selectedEntry_.clear();
                    }
                    else if (entry.IsScript())
                    {
                        std::vector<path> files;
                        if (entry.HasHeader())
                            files.push_back(entry.headerPath);
                        
                        if (entry.HasSource())
                            files.push_back(entry.sourcePath);
                        
                        for (auto& f : files)
                            EditorUtils::Utilities::OpenFileInCodeEditor(f);
                    }
                    else if (entry.IsPrefab() || entry.IsSpriteAtlas() || entry.IsAudioContainer())
                    {
                        if (state_.openAssetEditorCallback)
                            state_.openAssetEditorCallback(entry.path);
                    }
                }

                if (ImGui::BeginPopupContextItem("EntryCtx"))
                {
                    const bool isScript = entry.IsScript();

                    if (ImGui::MenuItem("Rename"))
                    {
                        if (isScript)
                        {
                            popupRequests_.renameTarget = entry.headerPath;
                            popupRequests_.renameSecondaryTarget = entry.sourcePath;
                            popupRequests_.renameTargetIsScriptGroup = true;
                        }
                        else
                        {
                            popupRequests_.renameTarget = entry.path;
                            popupRequests_.renameSecondaryTarget.clear();
                            popupRequests_.renameTargetIsScriptGroup = false;
                        }
                        
                        popupRequests_.renameEntry = true;
                    }

                    if (ImGui::MenuItem("Delete"))
                    {
                        String err;
                        const bool removed = isScript ? DeleteScriptFiles(entry, err) : Utils::FileUtils::TryRemove(entry.path, entry.IsDirectory(), err);

                        if (!removed)
                        {
                            state_.error = err;
                        }
                        else
                        {
                            state_.error.clear();
                            selectedEntry_.clear();
                            state_.cache.dirty = true;
                        }
                    }

                    if (ImGui::MenuItem("Show in Explorer"))
                    {
                        path targetPath = (isScript && !entry.headerPath.empty()) ? entry.headerPath : entry.path;
                        EditorUtils::Utilities::ShowPathInExplorer(targetPath, entry.IsDirectory() && !isScript);
                    }

                    ImGui::EndPopup();
                }

                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", entry.name.c_str());

                if (isSelected)
                {
                    GuiUtils::ScopedColor t(ImGuiCol_Text, ImVec4(1,0.85f,0.3f,1));
                    ImGui::TextWrapped("%s", entry.name.c_str());
                }
                else
                {
                    ImGui::TextWrapped("%s", entry.name.c_str());
                }
            }
            ImGui::EndTable();
        }

        ImGui::EndChild();
    }

    void ContentBrowserPanel::DrawPopups()
    {
        static CreatePrefabDialog prefab(state_, selectedEntry_);
        static CreateScriptDialog script(state_, selectedEntry_);
        static CreateFolderDialog folder(state_, selectedEntry_);
        static CreateSpriteAtlasDialog atlas(state_, selectedEntry_);
        static CreateAudioContainerDialog audioContainer(state_, selectedEntry_);
        static RenameEntryDialog rename(state_, selectedEntry_);

        if (popupRequests_.createPrefab)
        {
            prefab.Open();
            popupRequests_.createPrefab = false;
        }
        
        if (popupRequests_.createScript)
        {
            script.Open();
            popupRequests_.createScript = false;
        }
        
        if (popupRequests_.createFolder)
        {
            folder.Open(state_.current);
            popupRequests_.createFolder = false;
        }
        
        if (popupRequests_.createSpriteAtlas)
        {
            atlas.Open(state_.current);
            popupRequests_.createSpriteAtlas = false;
        }
        
        if (popupRequests_.createAudioContainer)
        {
            audioContainer.Open(state_.current);
            popupRequests_.createAudioContainer = false;
        }
        
        if (popupRequests_.renameEntry)
        {
            rename.Open(popupRequests_.renameTarget, popupRequests_.renameSecondaryTarget, popupRequests_.renameTargetIsScriptGroup);
            popupRequests_.renameEntry = false;
        }

        if (prefab.IsOpen())
            prefab.Render();
        
        if (script.IsOpen())
            script.Render();
        
        if (folder.IsOpen())
            folder.Render();
        
        if (atlas.IsOpen())
            atlas.Render();
        
        if (audioContainer.IsOpen())
            audioContainer.Render();
        
        if (rename.IsOpen())
            rename.Render();
    }

    // ==================================================================================
    //                                  LOGIC & HELPERS
    // ==================================================================================

    void ContentBrowserPanel::ImportExternalFiles(const std::vector<path>& paths)
    {
        auto now = std::chrono::steady_clock::now();
        
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_LastImportTime).count() < 500)
            return;
        
        m_LastImportTime = now;

        if (paths.empty() || !ContentBrowserUtils::EnsureContentBrowserInitialized(state_))
            return;
        
        EnsureValidDirectory();

        const path targetDirectory = (!state_.current.empty() && fs::exists(state_.current)) ? state_.current : state_.root;
        
        bool copiedAny = false;

        for (const auto& source : paths)
        {
            if (source.empty())
                continue;

            path destination = targetDirectory / source.filename();
            path finalDestination = destination;
            
            int suffix = 1;
            while (fs::exists(finalDestination))
            {
                finalDestination = destination.parent_path() / (destination.stem().string() + "_" + std::to_string(suffix++) + destination.extension().string());
            }

            String copyError;
            if (Utils::FileUtils::TryCopyFile(source, finalDestination, true, copyError))
            {
                copiedAny = true;
                LOG_INFO("Import success: " + finalDestination.generic_string());
            }
            else
            {
                LOG_ERROR(String("Import failed for " + source.generic_string() + ": ") + String(copyError));
            }
        }

        if (copiedAny)
        {
            m_PendingRefresh = true;
            m_RefreshTimer = 0.2f;
            selectedEntry_.clear(); 
        }
    }

    void ContentBrowserPanel::EnsureValidDirectory()
    {
        std::error_code ec;
        if (state_.current.empty() || !fs::exists(state_.current, ec))
            state_.current = state_.root;
    }

    bool ContentBrowserPanel::RefreshDirectoryCache()
    {
        if (!state_.cache.dirty && state_.cache.directory == state_.current)
            return true;
        
        std::vector<ContentEntry> newEntries;
        std::unordered_map<String, ContentEntry> scriptGroups;
        std::error_code error;
        
        try 
        {
            if (!fs::exists(state_.current))
                return false;

            for (auto& entry : fs::directory_iterator(state_.current, error))
            {
                if (error)
                    break;

                const path p = entry.path();

                if (entry.is_directory())
                {
                    newEntries.push_back({
                        .name = p.filename().generic_string(),
                        .path = p,
                        .type = ContentType::Directory
                    });
                    
                    continue;
                }

                const String ext = StringUtils::Utilities::ToLowerCopy(p.extension().generic_string());

                if (ext == ".h" || ext == ".cpp")
                {
                    auto key = StringUtils::Utilities::ToLowerCopy(p.stem().generic_string());
                    auto& group = scriptGroups[key];

                    if (group.name.empty())
                    {
                        group.name = p.stem().generic_string();
                        group.path = p.parent_path();
                        group.type = ContentType::Script;
                    }
                    
                    if (ext == ".h")
                    {
                        group.headerPath = p;
                    }
                    else
                    {
                        group.sourcePath = p;
                    }
                    
                    continue;
                }

                ContentType type = ContentType::File;
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
                else if (ext == ".mp3" || ext == ".wav" || ext == ".ogg")
                {
                    type = ContentType::Audio;
                }

                newEntries.push_back({
                    .name = p.filename().generic_string(),
                    .path = p,
                    .type = type,
                    .extension = ext
                });
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Exception in Content Browser Refresh: " + std::string(e.what()));
            return false; 
        }

        for (auto& script : scriptGroups | std::views::values)
        {
            newEntries.push_back(std::move(script));   
        }

        std::ranges::sort(newEntries, [](const ContentEntry& a, const ContentEntry& b)
        {
            int pa = GetSortPriority(a.type);
            int pb = GetSortPriority(b.type);
            if (pa != pb)
                return pa < pb;
            
            return Utils::FileUtils::CaseInsensitiveLess(a.name.Std(), b.name.Std());
        });

        state_.cache.entries = std::move(newEntries);
        state_.cache.dirty = false;
        state_.cache.directory = state_.current;

        return !error;
    }

    bool ContentBrowserPanel::DeleteScriptFiles(const ContentEntry& entry, String& error)
    {
        auto tryRemove = [&](const path& p)
        {
            return p.empty() || Utils::FileUtils::TryRemove(p, false, error);
        };
        
        return tryRemove(entry.headerPath) && tryRemove(entry.sourcePath);
    }

    void ContentBrowserPanel::HandleShortcuts()
    {
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        {
            if (ImGui::IsKeyPressed(ImGuiKey_F5))
            {
                state_.cache.dirty = true;
                state_.cache.ClearMeta();
            }
        }
    }

    void ContentBrowserPanel::OnOpen()
    {
        activeInstance_ = this;
    }
    void ContentBrowserPanel::OnClose()
    {
        if (activeInstance_ == this)
            activeInstance_ = nullptr;
    }

}