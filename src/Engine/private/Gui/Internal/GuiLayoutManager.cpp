#include "Gui/Internal/GuiLayoutManager.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <format>
#include <optional>
#include <string_view>
#include <system_error>

#include "imgui.h"
#include "Gui/GuiManager.h"
#include "Gui/Panels/GuiPanel.h"
#include "Gui/Internal/GuiSystem.h"
#include "Utils/FilesUtils.h"
#include "Utils/StringUtils.h"

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
    // RENDER LOOP
    // --------------------------------------------------------------
    void GuiLayoutManager::Render()
    {
        ProcessPendingSwitch_();
        EnsureDockspaceForCurrentLayout_();
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
    // LOAD FROM DISK
    // --------------------------------------------------------------
    void GuiLayoutManager::LoadPersistedLayouts_()
    {
        layoutStorageFile_ = FileUtils::ResolveUserConfigPath(kLayoutStorageFileName);
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

            StringUtils::TrimCarriageReturn(line);
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

            StringUtils::TrimCarriageReturn(sizeLine);

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
                    StringUtils::TrimCarriageReturn(regionLine);
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
            layoutStorageFile_ = FileUtils::ResolveUserConfigPath(kLayoutStorageFileName);
        
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
}
