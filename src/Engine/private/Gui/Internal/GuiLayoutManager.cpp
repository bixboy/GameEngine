#include "Gui/Internal/GuiLayoutManager.h"
#include "Gui/Core/EditorPreferences.h"
#include "Levels/EmptyScene.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <format>
#include <optional>
#include <string_view>
#include <system_error>

#include "imgui.h"
#include "Gui/Core/GuiManager.h"
#include "Gui/Panels/GuiPanel.h"
#include "Gui/Internal/GuiSystem.h"
#include "Utils/FileIO/FilesUtils.h"
#include "Utils/String/StringUtils.h"
#include "Framework/SceneManager.h"
#include "Serializer/SceneSerializer.h"
#include "Framework/SceneRegistry.h"

namespace BixEngine::Gui
{
    namespace
    {
        // ---------------------- CONSTANTES ----------------------
        constexpr const char* kRootDockspaceWindow = "EditorRootDockspace";
        constexpr const char* kLayoutStorageFileName = "imgui_layouts.dat";

        constexpr std::string_view kFileVersionV1 = "Version:1";
        constexpr std::string_view kFileVersionV2 = "Version:2";

        constexpr std::string_view kPrefixActiveLayout = "Active=";
        constexpr std::string_view kPrefixRegionIds = "RegionIds=";

        using DockRegionArray = std::array<ImGuiID, static_cast<size_t>(DockSpaceRegion::Count)>;

        // ---------------------- HELPERS ----------------------
        std::string SerializeRegionIds(const DockRegionArray& ids)
        {
            std::string out = "RegionIds=";
            for (size_t i = 0; i < ids.size(); ++i)
            {
                out += std::format("0x{:08X}", ids[i]);
                if (i + 1 < ids.size())
                    out += ',';
            }
            
            return out;
        }

        DockRegionArray ParseRegionIds(const std::string& line)
        {
            DockRegionArray ids{};
            if (line.rfind(kPrefixRegionIds.data(), 0) != 0)
                return ids;

            std::string_view v(line);
            v.remove_prefix(kPrefixRegionIds.size());

            size_t index = 0;
            while (!v.empty() && index < ids.size())
            {
                const size_t comma = v.find(',');
                std::string_view token = (comma == std::string_view::npos ? v : v.substr(0, comma));
                token = String(token).Trim().View();

                if (!token.empty())
                {
                    try
                    {
                        unsigned long parsed = std::stoul(std::string(token), nullptr, 0);
                        ids[index] = (ImGuiID)parsed;
                    }
                    catch (...)
                    {
                        ids[index] = 0;
                    }
                }

                ++index;
                if (comma == std::string_view::npos)
                    break;
                
                v.remove_prefix(comma + 1);
            }
            return ids;
        }
    }

    // --------------------------------------------------------------
    // CONSTRUCTEUR
    // --------------------------------------------------------------
    GuiLayoutManager::GuiLayoutManager(GuiSystem& guiSystem, GuiManager& guiManager)
        : guiSystem_(&guiSystem), guiManager_(&guiManager)
    {
        guiManager.RegisterLayoutManager(*this);

        dockspaceNames_[EditorLayoutType::Scene] = "SceneDockspace";
        dockspaceNames_[EditorLayoutType::ActorEditor] = "ActorEditorDockspace";

        layoutPanels_.try_emplace(EditorLayoutType::Scene);
        layoutPanels_.try_emplace(EditorLayoutType::ActorEditor);

        LoadPersistedLayouts_();
        LoadRecentScenes_();

        currentLayout_ = EditorLayoutType::Scene;
        pendingLayout_.reset();
        switchRequested_ = false;
        dockspaceDirty_ = true;

        EnsureDockspaceForCurrentLayout_();
        LoadLayout(currentLayout_);
    }

    // --------------------------------------------------------------
    // SWITCH
    // --------------------------------------------------------------
    void GuiLayoutManager::Switch(EditorLayoutType newLayout)
    {
        if (!guiSystem_)
            return;

        if (currentLayout_ == newLayout)
        {
            switchRequested_ = false;
            pendingLayout_.reset();
            return;
        }

        pendingLayout_ = newLayout;
        switchRequested_ = true;
    }

    // --------------------------------------------------------------
    // UPDATE LOOP (Before NewFrame)
    // --------------------------------------------------------------
    void GuiLayoutManager::Update()
    {
        ProcessPendingSwitch_();
        EnsureDockspaceForCurrentLayout_();
    }

    // --------------------------------------------------------------
    // RENDER LOOP (After NewFrame)
    // --------------------------------------------------------------
    void GuiLayoutManager::Render()
    {
        DrawMainMenuBar_();
        EditorPreferencesWindow::Draw(&showEditorPreferences_);
    }

    // --------------------------------------------------------------
    // SAVE CURRENT
    // --------------------------------------------------------------
    void GuiLayoutManager::SaveCurrentLayout()
    {
        if (!guiSystem_ || !guiSystem_->IsInitialized())
            return;

        StoredLayout record{};
        record.serialized = guiSystem_->SaveLayoutToMemory();
        record.dockRegionIds = guiSystem_->GetDockRegionIds();

        layoutData_[currentLayout_] = std::move(record);
    }

    // --------------------------------------------------------------
    // SAVE ALL (DISK)
    // --------------------------------------------------------------
    void GuiLayoutManager::SaveAllLayoutsToDisk()
    {
        PersistLayoutsToDisk_();
    }

    // --------------------------------------------------------------
    // PANEL REGISTRATION
    // --------------------------------------------------------------
    void GuiLayoutManager::RegisterPanels(EditorLayoutType layout, std::span<GuiPanel*> panels, LayoutRegistrationMode mode)
    {
        SetPanelsForLayout(layout, panels);

        if (mode == LayoutRegistrationMode::RegisterOnly)
            return;

        const bool ready = initializedLayouts_.contains(layout);

        if (mode == LayoutRegistrationMode::LoadIfUninitialized && !ready)
        {
            if (currentLayout_ == layout)
                LoadLayout(layout);
            else
                pendingInitialization_.insert(layout);

            return;
        }

        if (mode == LayoutRegistrationMode::ForceLoad)
        {
            if (currentLayout_ == layout) LoadLayout(layout);
            else pendingInitialization_.insert(layout);
        }
    }

    // --------------------------------------------------------------
    // DETACH PANELS
    // --------------------------------------------------------------
    void GuiLayoutManager::DetachPanels(std::span<GuiPanel*> panels)
    {
        for (GuiPanel* p : panels)
            if (p) RemovePanel(*p);
    }

    // --------------------------------------------------------------
    // RESET
    // --------------------------------------------------------------
    void GuiLayoutManager::ResetLayout(EditorLayoutType layout)
    {
        auto it = layoutPanels_.find(layout);
        if (it == layoutPanels_.end())
            return;

        for (GuiPanel* p : it->second)
        {
            panelLayoutLookup_.erase(p);
            if (p)
                p->SetVisible(false);
        }

        it->second.clear();
        initializedLayouts_.erase(layout);
    }

    // --------------------------------------------------------------
    // LOAD LAYOUT
    // --------------------------------------------------------------
    void GuiLayoutManager::LoadLayout(EditorLayoutType layout)
    {
        if (!guiSystem_ || !guiSystem_->IsInitialized())
            return;

        if (layout != currentLayout_)
        {
            pendingInitialization_.insert(layout);
            return;
        }

        pendingInitialization_.erase(layout);

        DockRegionArray regionIds{};

        if (auto it = layoutData_.find(layout); it != layoutData_.end())
        {
            auto& rec = it->second;

            if (!rec.serialized.empty())
                guiSystem_->LoadLayoutFromMemory(rec.serialized);
            else
                guiSystem_->RequestDefaultDockLayout();

            regionIds = rec.dockRegionIds;
        }
        else
        {
            guiSystem_->RequestDefaultDockLayout();
        }

        guiSystem_->SetDockRegionIds(regionIds);

        initializedLayouts_.insert(layout);
    }

    // --------------------------------------------------------------
    // SET PANELS FOR LAYOUT
    // --------------------------------------------------------------
    void GuiLayoutManager::SetPanelsForLayout(EditorLayoutType layout, std::span<GuiPanel*> panels)
    {
        auto& vect = layoutPanels_[layout];

        for (GuiPanel* p : vect)
            panelLayoutLookup_.erase(p);

        vect.clear();
        vect.reserve(panels.size());

        for (GuiPanel* p : panels)
        {
            if (!p)
                continue;
            
            vect.push_back(p);
            panelLayoutLookup_[p] = layout;
        }

        ApplyPanelVisibility_();
    }

    // --------------------------------------------------------------
    // ADD PANEL
    // --------------------------------------------------------------
    void GuiLayoutManager::AddPanel(EditorLayoutType layout, GuiPanel& panel)
    {
        RemovePanel(panel);

        auto& vect = layoutPanels_[layout];
        vect.push_back(&panel);
        panelLayoutLookup_[&panel] = layout;

        panel.SetVisible(currentLayout_ == layout);
    }

    // --------------------------------------------------------------
    // REMOVE PANEL
    // --------------------------------------------------------------
    void GuiLayoutManager::RemovePanel(GuiPanel& panel)
    {
        auto it = panelLayoutLookup_.find(&panel);
        if (it == panelLayoutLookup_.end())
            return;

        RemovePanelFromLayout_(panel, it->second);
        panelLayoutLookup_.erase(it);

        panel.SetVisible(false);
    }

    void GuiLayoutManager::RemovePanelFromLayout_(GuiPanel& panel, EditorLayoutType layout)
    {
        auto& vect = layoutPanels_[layout];
        std::erase(vect, &panel);
    }

    // --------------------------------------------------------------
    // DOCKSPACE HANDLING
    // --------------------------------------------------------------
    void GuiLayoutManager::EnsureDockspaceForCurrentLayout_()
    {
        if (!guiSystem_ || !dockspaceDirty_)
            return;

        const std::string label = dockspaceNames_[currentLayout_] + "::DockSpace";
        guiSystem_->SetDockspaceIdentifiers(kRootDockspaceWindow, label);

        dockspaceDirty_ = false;
    }

    // --------------------------------------------------------------
    // PENDING SWITCH
    // --------------------------------------------------------------
    void GuiLayoutManager::ProcessPendingSwitch_()
    {
        if (!switchRequested_ || !pendingLayout_.has_value() || !guiSystem_)
            return;

        const EditorLayoutType newLayout = *pendingLayout_;

        if (currentLayout_ == newLayout)
        {
            switchRequested_ = false;
            pendingLayout_.reset();
            return;
        }

        SaveCurrentLayout();

        currentLayout_ = newLayout;
        dockspaceDirty_ = true;

        pendingInitialization_.erase(newLayout);

        EnsureDockspaceForCurrentLayout_();
        LoadLayout(newLayout);
        ApplyPanelVisibility_();

        PersistLayoutsToDisk_();

        switchRequested_ = false;
        pendingLayout_.reset();
    }

    // --------------------------------------------------------------
    // VISIBILITY
    // --------------------------------------------------------------
    void GuiLayoutManager::ApplyPanelVisibility_()
    {
        if (!guiManager_)
            return;

        const auto& vect = layoutPanels_[currentLayout_];
        std::unordered_set visible(vect.begin(), vect.end());

        for (GuiPanel* p : guiManager_->GetPanels())
        {
            if (!p)
                continue;
            
            p->SetVisible(visible.contains(p));
        }
    }

    // --------------------------------------------------------------
    // MAIN MENU BAR
    // --------------------------------------------------------------
    // --------------------------------------------------------------
    // MAIN MENU BAR
    // --------------------------------------------------------------
    void GuiLayoutManager::DrawMainMenuBar_()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                // New Scene
                if (ImGui::MenuItem("New Scene"))
                {
                     if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                     {
                        sceneManager->SetScene(std::make_unique<Game::EmptyScene>());
                        currentScenePath_.clear();
                        isSceneDirty_ = false;
                     }
                }

                // Open Scene
                if (ImGui::MenuItem("Open Scene..."))
                {
                    showOpenSceneDialog_ = true;
                }

                // Recent Scenes submenu
                if (ImGui::BeginMenu("Recent Scenes", !recentScenes_.empty()))
                {
                    for (size_t i = 0; i < recentScenes_.size(); ++i)
                    {
                        const auto& path = recentScenes_[i];
                        std::string label = path.filename().string();
                        if (label.empty()) label = "Unknown";
                        std::string menuId = label + "##recent_" + std::to_string(i);
                        if (ImGui::MenuItem(menuId.c_str()))
                        {
                            if (std::filesystem::exists(path))
                            {
                                if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                                {
                                    auto newScene = std::make_unique<Game::EmptyScene>();
                                    newScene->SetName(path.stem().string().c_str());
                                    if (BixEngine::Serialization::SceneSerializer::LoadBinary(*newScene, path))
                                    {
                                        sceneManager->SetScene(std::move(newScene));
                                        currentScenePath_ = path;
                                        isSceneDirty_ = false;
                                        AddToRecentScenes_(path);
                                    }
                                }
                            }
                        }
                    }
                    ImGui::EndMenu();
                }

                ImGui::Separator();

                // Close Scene
                bool hasScene = !currentScenePath_.empty();
                if (ImGui::MenuItem("Close Scene", nullptr, false, hasScene))
                {
                    if (isSceneDirty_)
                    {
                        showCloseSceneConfirmation_ = true;
                    }
                    else
                    {
                        if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                        {
                            sceneManager->SetScene(std::make_unique<Game::EmptyScene>());
                            currentScenePath_.clear();
                            isSceneDirty_ = false;
                        }
                    }
                }

                // Save Scene
                if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                {
                    if (currentScenePath_.empty())
                    {
                        showSaveAsDialog_ = true;
                        saveAsFilenameBuffer_[0] = '\0';
                    }
                    else
                    {
                        if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                        {
                            if (auto* scene = sceneManager->GetScene())
                            {
                                BixEngine::Serialization::SceneSerializer::SaveBinary(*scene, currentScenePath_);
                                isSceneDirty_ = false;
                                AddToRecentScenes_(currentScenePath_);
                            }
                        }
                    }
                }

                // Save Scene As
                if (ImGui::MenuItem("Save Scene As..."))
                {
                    showSaveAsDialog_ = true;
                    saveAsFilenameBuffer_[0] = '\0';
                }

                ImGui::Separator();

                // Rename Scene
                if (ImGui::MenuItem("Rename Scene...", nullptr, false, hasScene))
                {
                    showRenameSceneDialog_ = true;
                    // Pre-fill with current name
                    std::string currentName = currentScenePath_.stem().string();
                    strncpy_s(renameFilenameBuffer_, currentName.c_str(), sizeof(renameFilenameBuffer_) - 1);
                }

                // Delete Scene
                if (ImGui::MenuItem("Delete Scene..."))
                {
                    showDeleteSceneDialog_ = true;
                }

                ImGui::Separator();

                // Scene Info
                if (hasScene)
                {
                    ImGui::TextDisabled("Current: %s", currentScenePath_.stem().string().c_str());
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", currentScenePath_.string().c_str());
                    }
                }
                else
                {
                    ImGui::TextDisabled("No scene loaded");
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Editor Preferences..."))
                {
                    showEditorPreferences_ = true;
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Windows"))
            {
                if (guiManager_)
                {
                    for (GuiPanel* panel : guiManager_->GetPanels())
                    {
                        if (!panel) continue;
                        
                        // Filter panels based on context
                        if (menuPanelFilter_)
                        {
                            if (!menuPanelFilter_(panel))
                                continue;
                        }
                        else
                        {
                            // Default behavior: only show panels belonging to current layout
                            auto it = panelLayoutLookup_.find(panel);
                            if (it != panelLayoutLookup_.end() && it->second != currentLayout_)
                                continue;
                        }
                        
                        bool visible = panel->IsVisible();
                        if (ImGui::MenuItem(panel->GetTitle().c_str(), nullptr, &visible))
                        {
                            panel->SetVisible(visible);
                            if (visible)
                            {
                                AddPanel(currentLayout_, *panel);
                            }
                            else
                            {
                                RemovePanelFromLayout_(*panel, currentLayout_);
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }

        DrawSaveAsDialog_();
        DrawOpenSceneDialog_();
        DrawDeleteSceneDialog_();
        DrawRenameSceneDialog_();
        DrawCloseSceneConfirmation_();
    }

    void GuiLayoutManager::DrawSaveAsDialog_()
    {
        if (showSaveAsDialog_)
        {
            ImGui::OpenPopup("Save Scene As");
            showSaveAsDialog_ = false;
        }

        if (ImGui::BeginPopupModal("Save Scene As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::InputText("Filename", saveAsFilenameBuffer_, sizeof(saveAsFilenameBuffer_));
            
            if (ImGui::Button("Save", ImVec2(120, 0)))
            {
                std::string filename = saveAsFilenameBuffer_;
                if (!filename.empty())
                {
                    if (!filename.ends_with(".bix"))
                        filename += ".bix";
                    
                    std::filesystem::create_directories("assets/scenes");
                    currentScenePath_ = std::filesystem::path("assets/scenes") / filename;

                    if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                    {
                        if (auto* scene = sceneManager->GetScene())
                        {
                            scene->SetName(std::filesystem::path(filename).stem().string().c_str());
                            BixEngine::Serialization::SceneSerializer::SaveBinary(*scene, currentScenePath_);
                            isSceneDirty_ = false;
                            AddToRecentScenes_(currentScenePath_);
                        }
                    }
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void GuiLayoutManager::DrawOpenSceneDialog_()
    {
        if (showOpenSceneDialog_)
        {
            ImGui::OpenPopup("Open Scene");
            showOpenSceneDialog_ = false;
        }

        if (ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static std::vector<std::filesystem::path> sceneFiles;
            if (ImGui::IsWindowAppearing())
            {
                sceneFiles.clear();
                if (std::filesystem::exists("assets/scenes"))
                {
                    for (const auto& entry : std::filesystem::directory_iterator("assets/scenes"))
                    {
                        if (entry.path().extension() == ".bix")
                        {
                            sceneFiles.push_back(entry.path());
                        }
                    }
                }
            }

            if (sceneFiles.empty())
            {
                ImGui::Text("No scenes found in assets/scenes/");
            }
            else
            {
                for (size_t i = 0; i < sceneFiles.size(); ++i)
                {
                    const auto& path = sceneFiles[i];
                    std::string label = path.filename().string();
                    if (label.empty()) label = "Unknown";
                    std::string selectId = label + "##open_" + std::to_string(i);
                    if (ImGui::Selectable(selectId.c_str()))
                    {
                        if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                        {
                            auto newScene = std::make_unique<Game::EmptyScene>();
                            newScene->SetName(path.stem().string().c_str());
                            if (BixEngine::Serialization::SceneSerializer::LoadBinary(*newScene, path))
                            {
                                sceneManager->SetScene(std::move(newScene));
                                currentScenePath_ = path;
                                isSceneDirty_ = false;
                                AddToRecentScenes_(path);
                            }
                        }
                        ImGui::CloseCurrentPopup();
                    }
                }
            }

            ImGui::Separator();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    // --------------------------------------------------------------
    // LOAD FROM DISK
    // --------------------------------------------------------------
    void GuiLayoutManager::LoadPersistedLayouts_()
    {
        layoutStorageFile_ = FilesUtils::Utilities::ResolveUserConfigPath(kLayoutStorageFileName);
        if (layoutStorageFile_.empty())
            return;

        std::ifstream file(layoutStorageFile_, std::ios::binary);
        if (!file.is_open())
            return;

        std::string line;
        std::optional<std::string> next;
        bool versionParsed = false;
        bool supportsRegionIds = false;

        while (true)
        {
            if (next)
            {
                line = std::move(*next);
                next.reset();
            }
            else if (!std::getline(file, line))
                break;

            StringUtils::Utilities::TrimCarriageReturn(line);
            if (line.empty())
                continue;

            if (!versionParsed &&
                (line == kFileVersionV1 || line == kFileVersionV2))
            {
                supportsRegionIds = (line == kFileVersionV2);
                versionParsed = true;
                continue;
            }

            if (line.rfind(kPrefixActiveLayout.data(), 0) == 0)
            {
                std::string name = line.substr(kPrefixActiveLayout.size());
                if (auto parsed = LayoutTypeFromString(name))
                {
                    currentLayout_ = *parsed;
                    dockspaceDirty_ = true;
                }
                
                continue;
            }

            auto parsedLayout = LayoutTypeFromString(line);
            if (!parsedLayout)
                continue;

            std::string sizeLine;
            if (!std::getline(file, sizeLine))
                break;

            StringUtils::Utilities::TrimCarriageReturn(sizeLine);

            size_t size = 0;
            try
            {
                size = std::stoull(sizeLine);
            }
            catch (...)
            {
                size = 0;
            }

            StoredLayout data{};

            if (size > 0)
            {
                data.serialized.resize(size);
                file.read(data.serialized.data(), size);
            }

            if (file.peek() == '\r') file.get();
            if (file.peek() == '\n') file.get();

            if (supportsRegionIds)
            {
                std::string regionLine;
                if (std::getline(file, regionLine))
                {
                    StringUtils::Utilities::TrimCarriageReturn(regionLine);
                    if (regionLine.rfind(kPrefixRegionIds.data(), 0) == 0)
                    {
                        data.dockRegionIds = ParseRegionIds(regionLine);
                    }
                    else
                    {
                        next = std::move(regionLine);   
                    }
                }
            }

            layoutData_[*parsedLayout] = std::move(data);
        }
    }

    // --------------------------------------------------------------
    // SAVE TO DISK
    // --------------------------------------------------------------
    void GuiLayoutManager::PersistLayoutsToDisk_()
    {
        if (layoutStorageFile_.empty())
            layoutStorageFile_ = FilesUtils::Utilities::ResolveUserConfigPath(kLayoutStorageFileName);
        
        if (layoutStorageFile_.empty())
            return;

        std::filesystem::path dir = layoutStorageFile_.parent_path();
        if (!dir.empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
        }

        std::ofstream f(layoutStorageFile_, std::ios::binary | std::ios::trunc);
        if (!f.is_open())
            return;

        f << kFileVersionV2 << '\n';
        f << kPrefixActiveLayout << LayoutTypeToString(currentLayout_) << '\n';

        constexpr std::array order = {
            EditorLayoutType::Scene,
            EditorLayoutType::ActorEditor
        };

        for (EditorLayoutType L : order)
        {
            const auto it = layoutData_.find(L);
            const StoredLayout* rec = (it != layoutData_.end() ? &it->second : nullptr);

            f << LayoutTypeToString(L) << '\n';

            const size_t size = (rec ? rec->serialized.size() : 0);
            f << size << '\n';

            if (rec && size > 0)
                f.write(rec->serialized.data(), size);

            f << '\n';

            DockRegionArray region{};
            if (rec)
                region = rec->dockRegionIds;
            
            f << SerializeRegionIds(region) << '\n';
        }
    }

    // --------------------------------------------------------------
    // MISC HELPERS
    // --------------------------------------------------------------

    std::string GuiLayoutManager::LayoutTypeToString(EditorLayoutType type)
    {
        switch (type)
        {
            case EditorLayoutType::Scene:
                return "Scene";
            
            case EditorLayoutType::ActorEditor:
                return "ActorEditor";
        }
        
        return "Scene";
    }

    std::optional<EditorLayoutType> GuiLayoutManager::LayoutTypeFromString(const std::string& v)
    {
        if (v == "Scene")
            return EditorLayoutType::Scene;
        
        if (v == "ActorEditor")
            return EditorLayoutType::ActorEditor;
        
        return std::nullopt;
    }

    // --------------------------------------------------------------
    // DELETE SCENE DIALOG
    // --------------------------------------------------------------
    void GuiLayoutManager::DrawDeleteSceneDialog_()
    {
        if (showDeleteSceneDialog_)
        {
            ImGui::OpenPopup("Delete Scene");
            showDeleteSceneDialog_ = false;
        }

        if (ImGui::BeginPopupModal("Delete Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static std::vector<std::filesystem::path> sceneFiles;
            static std::filesystem::path selectedScene;
            
            if (ImGui::IsWindowAppearing())
            {
                sceneFiles.clear();
                selectedScene.clear();
                if (std::filesystem::exists("assets/scenes"))
                {
                    for (const auto& entry : std::filesystem::directory_iterator("assets/scenes"))
                    {
                        if (entry.path().extension() == ".bix")
                        {
                            sceneFiles.push_back(entry.path());
                        }
                    }
                }
            }

            if (sceneFiles.empty())
            {
                ImGui::Text("No scenes found in assets/scenes/");
            }
            else
            {
                ImGui::Text("Select a scene to delete:");
                ImGui::Separator();
                
                // Use a child window to contain the list
                if (ImGui::BeginChild("SceneList", ImVec2(300, 200), true))
                {
                    for (size_t i = 0; i < sceneFiles.size(); ++i)
                    {
                        const auto& path = sceneFiles[i];
                        std::string label = path.filename().string();
                        if (label.empty()) label = "Unknown";
                        std::string selectId = label + "##delete_" + std::to_string(i);
                        bool isSelected = (selectedScene == path);
                        if (ImGui::Selectable(selectId.c_str(), isSelected))
                        {
                            selectedScene = path;
                        }
                    }
                }
                ImGui::EndChild();
            }

            ImGui::Separator();
            
            bool canDelete = !selectedScene.empty();
            if (ImGui::Button("Delete", ImVec2(120, 0)) && canDelete)
            {
                // Confirmation popup
                sceneToDelete_ = selectedScene;
                ImGui::OpenPopup("Confirm Delete");
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            // Nested confirmation dialog
            if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Are you sure you want to delete:");
                std::string deleteLabel = sceneToDelete_.filename().string();
                if (deleteLabel.empty()) deleteLabel = "Unknown";
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "%s", deleteLabel.c_str());
                ImGui::Text("This action cannot be undone!");
                ImGui::Separator();

                if (ImGui::Button("Yes, Delete", ImVec2(120, 0)))
                {
                    std::error_code ec;
                    std::filesystem::remove(sceneToDelete_, ec);
                    
                    // Remove from recent scenes
                    auto it = std::find(recentScenes_.begin(), recentScenes_.end(), sceneToDelete_);
                    if (it != recentScenes_.end())
                    {
                        recentScenes_.erase(it);
                        SaveRecentScenes_();
                    }
                    
                    // If we deleted the current scene, clear it
                    if (currentScenePath_ == sceneToDelete_)
                    {
                        if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                        {
                            sceneManager->SetScene(std::make_unique<Game::EmptyScene>());
                            currentScenePath_.clear();
                            isSceneDirty_ = false;
                        }
                    }
                    
                    // Remove from the list and clear selection
                    auto fileIt = std::find(sceneFiles.begin(), sceneFiles.end(), sceneToDelete_);
                    if (fileIt != sceneFiles.end())
                    {
                        sceneFiles.erase(fileIt);
                    }
                    selectedScene.clear();
                    sceneToDelete_.clear();
                    
                    ImGui::CloseCurrentPopup(); // Close confirm dialog
                    ImGui::CloseCurrentPopup(); // Close parent dialog
                }
                
                ImGui::SameLine();
                if (ImGui::Button("No, Cancel", ImVec2(120, 0)))
                {
                    sceneToDelete_.clear();
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::EndPopup();
            }

            ImGui::EndPopup();
        }
    }

    // --------------------------------------------------------------
    // RENAME SCENE DIALOG
    // --------------------------------------------------------------
    void GuiLayoutManager::DrawRenameSceneDialog_()
    {
        if (showRenameSceneDialog_)
        {
            ImGui::OpenPopup("Rename Scene");
            showRenameSceneDialog_ = false;
        }

        if (ImGui::BeginPopupModal("Rename Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            std::string currentName = currentScenePath_.filename().string();
            if (currentName.empty()) currentName = "Unknown";
            ImGui::Text("Current: %s", currentName.c_str());
            ImGui::Separator();
            ImGui::InputText("New Name", renameFilenameBuffer_, sizeof(renameFilenameBuffer_));
            
            if (ImGui::Button("Rename", ImVec2(120, 0)))
            {
                std::string newName = renameFilenameBuffer_;
                if (!newName.empty())
                {
                    if (!newName.ends_with(".bix"))
                        newName += ".bix";
                    
                    std::filesystem::path newPath = currentScenePath_.parent_path() / newName;
                    
                    if (newPath != currentScenePath_)
                    {
                        std::error_code ec;
                        std::filesystem::rename(currentScenePath_, newPath, ec);
                        
                        if (!ec)
                        {
                            // Update recent scenes
                            auto it = std::find(recentScenes_.begin(), recentScenes_.end(), currentScenePath_);
                            if (it != recentScenes_.end())
                            {
                                *it = newPath;
                                SaveRecentScenes_();
                            }
                            
                            // Update current path and scene name
                            currentScenePath_ = newPath;
                            if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                            {
                                if (auto* scene = sceneManager->GetScene())
                                {
                                    scene->SetName(std::filesystem::path(newName).stem().string().c_str());
                                    isSceneDirty_ = true;
                                }
                            }
                        }
                    }
                    
                    ImGui::CloseCurrentPopup();
                }
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }

    // --------------------------------------------------------------
    // CLOSE SCENE CONFIRMATION
    // --------------------------------------------------------------
    void GuiLayoutManager::DrawCloseSceneConfirmation_()
    {
        if (showCloseSceneConfirmation_)
        {
            ImGui::OpenPopup("Close Scene");
            showCloseSceneConfirmation_ = false;
        }

        if (ImGui::BeginPopupModal("Close Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Scene has unsaved changes.");
            ImGui::Text("Do you want to save before closing?");
            ImGui::Separator();

            if (ImGui::Button("Save and Close", ImVec2(140, 0)))
            {
                // Save the scene first
                if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                {
                    if (auto* scene = sceneManager->GetScene())
                    {
                        BixEngine::Serialization::SceneSerializer::SaveBinary(*scene, currentScenePath_);
                    }
                    
                    // Then close
                    sceneManager->SetScene(std::make_unique<Game::EmptyScene>());
                    currentScenePath_.clear();
                    isSceneDirty_ = false;
                }
                
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Don't Save", ImVec2(140, 0)))
            {
                // Close without saving
                if (auto* sceneManager = Game::SceneManager::GetActiveSceneManager())
                {
                    sceneManager->SetScene(std::make_unique<Game::EmptyScene>());
                    currentScenePath_.clear();
                    isSceneDirty_ = false;
                }
                
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(140, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
    }

    // --------------------------------------------------------------
    // RECENT SCENES MANAGEMENT
    // --------------------------------------------------------------
    void GuiLayoutManager::AddToRecentScenes_(const std::filesystem::path& path)
    {
        if (path.empty())
            return;
        
        // Remove if already exists
        auto it = std::find(recentScenes_.begin(), recentScenes_.end(), path);
        if (it != recentScenes_.end())
        {
            recentScenes_.erase(it);
        }
        
        // Add to front
        recentScenes_.insert(recentScenes_.begin(), path);
        
        // Keep only 5 most recent
        if (recentScenes_.size() > 5)
        {
            recentScenes_.resize(5);
        }
        
        SaveRecentScenes_();
    }

    void GuiLayoutManager::LoadRecentScenes_()
    {
        recentScenesFile_ = FilesUtils::Utilities::ResolveUserConfigPath("recent_scenes.txt");
        if (recentScenesFile_.empty())
            return;
        
        std::ifstream file(recentScenesFile_);
        if (!file.is_open())
            return;
        
        recentScenes_.clear();
        std::string line;
        while (std::getline(file, line))
        {
            StringUtils::Utilities::TrimCarriageReturn(line);
            if (!line.empty())
            {
                std::filesystem::path path(line);
                if (std::filesystem::exists(path))
                {
                    recentScenes_.push_back(path);
                }
            }
            
            if (recentScenes_.size() >= 5)
                break;
        }
    }

    void GuiLayoutManager::SaveRecentScenes_()
    {
        if (recentScenesFile_.empty())
            recentScenesFile_ = FilesUtils::Utilities::ResolveUserConfigPath("recent_scenes.txt");
        
        if (recentScenesFile_.empty())
            return;
        
        std::filesystem::path dir = recentScenesFile_.parent_path();
        if (!dir.empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
        }
        
        std::ofstream file(recentScenesFile_, std::ios::trunc);
        if (!file.is_open())
            return;
        
        for (const auto& path : recentScenes_)
        {
            file << path.string() << '\n';
        }
    }
}
