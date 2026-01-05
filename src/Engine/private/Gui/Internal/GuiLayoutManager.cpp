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
#include "Gui/Core/GuiManager.h"
#include "Gui/Panels/GuiPanel.h"
#include "Gui/Internal/GuiSystem.h"
#include "Utils/FileIO/FilesUtils.h"
#include "Utils/String/StringUtils.h"

namespace BixEngine::Gui
{
    namespace
    {
        constexpr const char* kRootDockspaceWindow = "EditorRootDockspace";
        constexpr const char* kLayoutStorageFileName = "imgui_layouts.dat";

        constexpr std::string_view kFileVersion = "Version:2";

        constexpr std::string_view kPrefixActiveLayout = "Active=";
        constexpr std::string_view kPrefixRegionIds = "RegionIds=";
        constexpr std::string_view kPrefixLayoutName = "Layout=";

        using DockRegionArray = std::array<ImGuiID, static_cast<size_t>(DockSpaceRegion::Count)>;

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
            
            if (!line.starts_with(kPrefixRegionIds))
                return ids;

            std::string_view v(line);
            v.remove_prefix(kPrefixRegionIds.size());

            size_t index = 0;
            while (!v.empty() && index < ids.size())
            {
                const size_t comma = v.find(',');
                std::string_view token = (comma == std::string_view::npos ? v : v.substr(0, comma));
                
                std::string tokenStr(token);
                if (!tokenStr.empty())
                {
                    try
                    {
                        unsigned long parsed = std::stoul(tokenStr, nullptr, 0);
                        ids[index] = static_cast<ImGuiID>(parsed);
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

    GuiLayoutManager::GuiLayoutManager(GuiSystem& guiSystem, GuiManager& guiManager) : guiSystem_(&guiSystem), guiManager_(&guiManager)
    {
        guiManager.RegisterLayoutManager(*this);

        // Pre-create default layouts
        CreateLayout(DefaultLayouts::Scene);
        CreateLayout(DefaultLayouts::ActorEditor);

        LoadPersistedLayouts_();

        currentLayout_ = DefaultLayouts::Scene;
        pendingLayout_.reset();
        switchRequested_ = false;
        dockspaceDirty_ = true;

        EnsureDockSpaceForCurrentLayout_();
        LoadLayout(currentLayout_);
    }

    void GuiLayoutManager::CreateLayout(const LayoutID& layoutName)
    {
        if (layoutName.empty()) return;
        layoutPanels_.try_emplace(layoutName);
    }

    bool GuiLayoutManager::HasLayout(const LayoutID& layout) const
    {
        return layoutPanels_.contains(layout);
    }
    
    void GuiLayoutManager::Switch(const LayoutID& newLayout)
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

    void GuiLayoutManager::Update()
    {
        ProcessPendingSwitch_();
        EnsureDockSpaceForCurrentLayout_();
    }

    void GuiLayoutManager::Render()
    {
    }

    void GuiLayoutManager::SaveCurrentLayout()
    {
        if (!guiSystem_ || !guiSystem_->IsInitialized() || currentLayout_.empty())
            return;

        StoredLayout record{};
        record.serialized = guiSystem_->SaveLayoutToMemory();
        record.dockRegionIds = guiSystem_->GetDockRegionIds();

        layoutData_[currentLayout_] = std::move(record);
    }

    void GuiLayoutManager::SaveAllLayoutsToDisk()
    {
        PersistLayoutsToDisk_();
    }

    void GuiLayoutManager::RegisterPanels(const LayoutID& layout, std::span<GuiPanel*> panels, LayoutRegistrationMode mode)
    {
        if (!HasLayout(layout))
            CreateLayout(layout);

        SetPanelsForLayout(layout, panels);

        if (mode == LayoutRegistrationMode::RegisterOnly)
            return;

        const bool ready = initializedLayouts_.contains(layout);

        if (mode == LayoutRegistrationMode::ForceLoad || (mode == LayoutRegistrationMode::LoadIfUninitialized && !ready))
        {
            if (currentLayout_ == layout)
            {
                LoadLayout(layout);
            }
            else
            {
                pendingInitialization_.insert(layout);
            }
        }
    }

    void GuiLayoutManager::DetachPanels(std::span<GuiPanel*> panels)
    {
        for (GuiPanel* p : panels)
        {
            if (p)
            {
                RemovePanel(*p);
            }
        }
    }

    void GuiLayoutManager::ResetLayout(const LayoutID& layout)
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

    void GuiLayoutManager::LoadLayout(const LayoutID& layout)
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
            {
                guiSystem_->LoadLayoutFromMemory(rec.serialized);
            }
            else
            {
                guiSystem_->RequestDefaultDockLayout();
            }

            regionIds = rec.dockRegionIds;
        }
        else
        {
            guiSystem_->RequestDefaultDockLayout();
        }

        guiSystem_->SetDockRegionIds(regionIds);
        initializedLayouts_.insert(layout);
    }

    void GuiLayoutManager::SetPanelsForLayout(const LayoutID& layout, std::span<GuiPanel*> panels)
    {
        auto& vect = layoutPanels_[layout];

        // Clean up old lookups for panels in this layout
        for (GuiPanel* p : vect)
        {
            panelLayoutLookup_.erase(p);
        }

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

    void GuiLayoutManager::AddPanel(const LayoutID& layout, GuiPanel& panel)
    {
        RemovePanel(panel);

        auto& vect = layoutPanels_[layout];
        vect.push_back(&panel);
        panelLayoutLookup_[&panel] = layout;

        panel.SetVisible(currentLayout_ == layout);
    }

    void GuiLayoutManager::RemovePanel(GuiPanel& panel)
    {
        auto it = panelLayoutLookup_.find(&panel);
        if (it == panelLayoutLookup_.end())
            return;

        RemovePanelFromLayout_(panel, it->second);
        panelLayoutLookup_.erase(it);

        panel.SetVisible(false);
    }

    void GuiLayoutManager::RemovePanelFromLayout_(GuiPanel& panel, const LayoutID& layout)
    {
        auto& vect = layoutPanels_[layout];
        std::erase(vect, &panel);
    }

    void GuiLayoutManager::EnsureDockSpaceForCurrentLayout_()
    {
        if (!guiSystem_ || !dockspaceDirty_)
            return;

        const std::string label = String::Format("{}::DockSpace", currentLayout_).Std();
        guiSystem_->SetDockspaceIdentifiers(kRootDockspaceWindow, label);

        dockspaceDirty_ = false;
    }

    void GuiLayoutManager::ProcessPendingSwitch_()
    {
        if (!switchRequested_ || !pendingLayout_.has_value() || !guiSystem_)
            return;

        const LayoutID newLayout = *pendingLayout_;

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

        EnsureDockSpaceForCurrentLayout_();
        LoadLayout(newLayout);
        ApplyPanelVisibility_();

        PersistLayoutsToDisk_();

        switchRequested_ = false;
        pendingLayout_.reset();
    }

    void GuiLayoutManager::ApplyPanelVisibility_()
    {
        if (!guiManager_)
            return;

        const auto& vect = layoutPanels_[currentLayout_];
        std::unordered_set<GuiPanel*> visible(vect.begin(), vect.end());

        for (GuiPanel* p : guiManager_->GetPanels())
        {
            if (!p)
                continue;
            
            p->SetVisible(visible.contains(p));
        }
    }

    void GuiLayoutManager::LoadPersistedLayouts_()
    {
        layoutStorageFile_ = BixEngine::Utils::FileUtils::ResolveUserConfigPath(kLayoutStorageFileName);
        if (layoutStorageFile_.empty())
            return;

        std::ifstream file(layoutStorageFile_, std::ios::binary);
        if (!file.is_open())
            return;

        std::string line;
        std::optional<std::string> next;
        bool headerParsed = false;
        LayoutID loadingLayoutName; // Temp to hold current parsing layout name

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

            if (!headerParsed)
            {
                if (line == kFileVersion)
                {
                    headerParsed = true;
                }
                continue;
            }

            if (line.starts_with(kPrefixActiveLayout))
            {
                std::string name = line.substr(kPrefixActiveLayout.size());
                if (!name.empty())
                {
                    currentLayout_ = name.c_str(); // Create LayoutID
                    dockspaceDirty_ = true;
                }
                continue;
            }

            // Check for new layout block header "Layout=Name"
            // or backwards compatibility or just raw name? 
            // In previous version it was just "Scene" or "ActorEditor"
            // But now we can have any string. Let's assume it's a name if it's not a property
            
            if (line.starts_with(kPrefixLayoutName))
            {
                loadingLayoutName = line.substr(kPrefixLayoutName.size()).c_str();
            }
            else
            {
               // Fallback / legacy support could go here for "Scene" / "ActorEditor" lines if needed
               // For now let's assume valid file matches our new format or is simple name
               if (loadingLayoutName.empty())
                   loadingLayoutName = line.c_str();
            }
            
            if (loadingLayoutName.empty()) continue;

            // Ensure layout exists
            if (!layoutPanels_.contains(loadingLayoutName))
                CreateLayout(loadingLayoutName);

            // Read size
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

            // Consume newlines after binary block
            if (file.peek() == '\r') file.get();
            if (file.peek() == '\n') file.get();

            std::string regionLine;
            if (std::getline(file, regionLine))
            {
                StringUtils::Utilities::TrimCarriageReturn(regionLine);
                if (regionLine.starts_with(kPrefixRegionIds))
                {
                    data.dockRegionIds = ParseRegionIds(regionLine);
                }
                else
                {
                    next = std::move(regionLine);   
                }
            }
            
            layoutData_[loadingLayoutName] = std::move(data);
            
            // Reset for next iteration (optional, but cleaner)
            loadingLayoutName.clear(); 
        }
    }

    void GuiLayoutManager::PersistLayoutsToDisk_()
    {
        if (layoutStorageFile_.empty())
            layoutStorageFile_ = BixEngine::Utils::FileUtils::ResolveUserConfigPath(kLayoutStorageFileName);
        
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

        f << kFileVersion << '\n';
        f << kPrefixActiveLayout << currentLayout_.View() << '\n';

        for (const auto& [name, rec] : layoutData_)
        {
            // Only save if we have data
            if (rec.serialized.empty() && name != currentLayout_) // Save current layout even if empty so we track it
                continue;

            f << kPrefixLayoutName << name.View() << '\n';

            const size_t size = rec.serialized.size();
            f << size << '\n';

            if (size > 0)
                f.write(rec.serialized.data(), size);

            f << '\n';
            f << SerializeRegionIds(rec.dockRegionIds) << '\n';
        }
    }

    bool GuiLayoutManager::IsPanelVisibleInCurrentLayout(GuiPanel *panel) const
    {
        if (!panel)
            return false;
            
        if (customMenuFilter_)
        {
            return customMenuFilter_(panel);
        }
        
        auto it = panelLayoutLookup_.find(panel);
        if (it != panelLayoutLookup_.end())
        {
            return it->second == currentLayout_;
        }
        
        // Panels not associated with any layout are considered visible/global
        return true;
    }

    void GuiLayoutManager::SetMenuPanelFilter(std::function<bool(GuiPanel*)> filter)
    {
        customMenuFilter_ = std::move(filter);
    }
}