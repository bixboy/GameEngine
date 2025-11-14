#include "Gui/Internal/GuiLayoutManager.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <imgui.h>

#include "Gui/GuiManager.h"
#include "Gui/Internal/GuiPanel.h"
#include "Gui/Internal/GuiSystem.h"


namespace BixEngine::Gui
{
    namespace
    {
        constexpr auto kRootDockspaceWindow = "EditorRootDockspace";
        constexpr std::string_view kLayoutStorageFileName = "imgui_layouts.dat";
        constexpr std::string_view kFileVersionLine = "Version:1";
        constexpr std::string_view kActiveLayoutPrefix = "Active=";
    }

    GuiLayoutManager::GuiLayoutManager(GuiSystem& guiSystem, GuiManager& guiManager) : guiSystem_(&guiSystem), guiManager_(&guiManager)
    {
        guiManager.RegisterLayoutManager(*this);

        dockspaceNames_[EditorLayoutType::Scene] = "SceneDockspace";
        dockspaceNames_[EditorLayoutType::ActorEditor] = "ActorEditorDockspace";

        layoutPanels_.emplace(EditorLayoutType::Scene, std::vector<GuiPanel*>{});
        layoutPanels_.emplace(EditorLayoutType::ActorEditor, std::vector<GuiPanel*>{});

        LoadPersistedLayouts_();

        currentLayout_ = EditorLayoutType::Scene;
        pendingLayout_.reset();
        switchRequested_ = false;
        dockspaceDirty_ = true;

        EnsureDockspaceForCurrentLayout_();
        LoadLayout(currentLayout_);
    }

    void GuiLayoutManager::Switch(EditorLayoutType newLayout)
    {
        if (!guiSystem_)
            return;

        if (currentLayout_ == newLayout)
        {
            if (switchRequested_)
            {
                switchRequested_ = false;
                pendingLayout_.reset();
            }
            return;
        }

        pendingLayout_ = newLayout;
        switchRequested_ = true;
    }

    void GuiLayoutManager::Render()
    {
        ProcessPendingSwitch_();
        EnsureDockspaceForCurrentLayout_();
    }

    void GuiLayoutManager::SaveCurrentLayout()
    {
        if (!guiSystem_ || !guiSystem_->IsInitialized())
            return;

        const std::string serialized = guiSystem_->SaveLayoutToMemory();
        layoutData_[currentLayout_] = serialized;
    }

    void GuiLayoutManager::SaveAllLayoutsToDisk()
    {
        PersistLayoutsToDisk_();
    }

    void GuiLayoutManager::RegisterPanels(EditorLayoutType layout, std::span<GuiPanel*> panels, LayoutRegistrationMode mode)
    {
        SetPanelsForLayout(layout, panels);

        switch (mode)
        {
        case LayoutRegistrationMode::RegisterOnly:
            break;
        case LayoutRegistrationMode::LoadIfUninitialized:
            if (!initializedLayouts_.contains(layout))
                LoadLayout(layout);
            break;
        case LayoutRegistrationMode::ForceLoad:
            LoadLayout(layout);
            break;
        }
    }

    void GuiLayoutManager::DetachPanels(std::span<GuiPanel*> panels)
    {
        for (GuiPanel* panel : panels)
        {
            if (!panel)
                continue;
            RemovePanel(*panel);
        }
    }

    void GuiLayoutManager::ResetLayout(EditorLayoutType layout)
    {
        auto layoutIt = layoutPanels_.find(layout);
        if (layoutIt == layoutPanels_.end())
            return;

        for (GuiPanel* panel : layoutIt->second)
        {
            if (!panel)
                continue;
            panelLayoutLookup_.erase(panel);
            panel->SetVisible(false);
        }

        layoutIt->second.clear();
        initializedLayouts_.erase(layout);
    }

    void GuiLayoutManager::LoadLayout(EditorLayoutType layout)
    {
        if (!guiSystem_ || !guiSystem_->IsInitialized())
            return;

        auto it = layoutData_.find(layout);
        if (it != layoutData_.end() && !it->second.empty())
        {
            guiSystem_->LoadLayoutFromMemory(it->second);
        }
        else
        {
            guiSystem_->RequestDefaultDockLayout();
        }

        initializedLayouts_.insert(layout);
    }

    void GuiLayoutManager::SetPanelsForLayout(EditorLayoutType layout, std::span<GuiPanel*> panels)
    {
        auto& layoutVector = layoutPanels_[layout];
        for (GuiPanel* panel : layoutVector)
        {
            if (!panel)
                continue;

            auto lookup = panelLayoutLookup_.find(panel);
            if (lookup != panelLayoutLookup_.end() && lookup->second == layout)
                panelLayoutLookup_.erase(lookup);
        }

        layoutVector.clear();

        layoutVector.reserve(panels.size());
        for (GuiPanel* panel : panels)
        {
            if (!panel)
                continue;

            if (std::find(layoutVector.begin(), layoutVector.end(), panel) != layoutVector.end())
                continue;

            layoutVector.push_back(panel);
            panelLayoutLookup_[panel] = layout;
        }

        ApplyPanelVisibility_();
    }

    void GuiLayoutManager::AddPanel(EditorLayoutType layout, GuiPanel& panel)
    {
        RemovePanel(panel);

        auto& layoutVector = layoutPanels_[layout];
        layoutVector.push_back(&panel);
        panelLayoutLookup_[&panel] = layout;

        if (currentLayout_ == layout)
            panel.SetVisible(true);
        else
            panel.SetVisible(false);
    }

    void GuiLayoutManager::RemovePanel(GuiPanel& panel)
    {
        auto lookup = panelLayoutLookup_.find(&panel);
        if (lookup == panelLayoutLookup_.end())
            return;

        RemovePanelFromLayout_(panel, lookup->second);
        panelLayoutLookup_.erase(lookup);
        panel.SetVisible(false);
    }

    void GuiLayoutManager::EnsureDockspaceForCurrentLayout_()
    {
        if (!guiSystem_)
            return;

        const std::string dockspaceName = dockspaceNames_[currentLayout_];
        const std::string dockspaceLabel = dockspaceName + "::DockSpace";

        if (!dockspaceDirty_)
            return;

        guiSystem_->SetDockspaceIdentifiers(kRootDockspaceWindow, dockspaceLabel);
        dockspaceDirty_ = false;
    }

    void GuiLayoutManager::ProcessPendingSwitch_()
    {
        if (!switchRequested_ || !pendingLayout_.has_value() || !guiSystem_)
            return;

        const EditorLayoutType newLayout = pendingLayout_.value();
        if (currentLayout_ == newLayout)
        {
            switchRequested_ = false;
            pendingLayout_.reset();
            return;
        }

        SaveCurrentLayout();

        currentLayout_ = newLayout;
        dockspaceDirty_ = true;
        EnsureDockspaceForCurrentLayout_();
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

        std::unordered_set<GuiPanel*> visiblePanels;
        const auto& panels = layoutPanels_[currentLayout_];
        visiblePanels.insert(panels.begin(), panels.end());

        for (GuiPanel* panel : guiManager_->GetPanels())
        {
            if (!panel)
                continue;

            const bool shouldBeVisible = visiblePanels.contains(panel);
            panel->SetVisible(shouldBeVisible);
        }
    }

    void GuiLayoutManager::RemovePanelFromLayout_(GuiPanel& panel, EditorLayoutType layout)
    {
        auto& layoutVector = layoutPanels_[layout];
        std::erase(layoutVector, &panel);
    }

    void GuiLayoutManager::LoadPersistedLayouts_()
    {
        layoutStorageFile_ = ResolveLayoutStoragePath_();
        if (layoutStorageFile_.empty())
            return;

        std::ifstream file(layoutStorageFile_, std::ios::binary);
        if (!file.is_open())
            return;

        std::string line;
        while (std::getline(file, line))
        {
            TrimTrailingCarriageReturn_(line);
            if (line.empty())
                continue;

            if (line.rfind(kFileVersionLine.data(), 0) == 0)
                continue;

            if (line.rfind(kActiveLayoutPrefix.data(), 0) == 0)
            {
                const std::string layoutName = line.substr(kActiveLayoutPrefix.size());
                if (auto parsed = LayoutTypeFromString(layoutName))
                {
                    currentLayout_ = *parsed;
                    dockspaceDirty_ = true;
                }
                continue;
            }

            auto layoutType = LayoutTypeFromString(line);
            if (!layoutType.has_value())
                continue;

            std::string sizeLine;
            if (!std::getline(file, sizeLine))
                break;

            TrimTrailingCarriageReturn_(sizeLine);

            size_t dataSize = 0;
            if (!sizeLine.empty())
            {
                try
                {
                    dataSize = std::stoull(sizeLine);
                }
                catch (...)
                {
                    dataSize = 0;
                }
            }

            std::string data(dataSize, '\0');
            if (dataSize > 0)
            {
                file.read(data.data(), dataSize);
                if (!file)
                    break;
            }

            if (file.peek() == '\r')
            {
                file.get();
                if (file.peek() == '\n')
                    file.get();
            }
            else if (file.peek() == '\n')
            {
                file.get();
            }

            layoutData_[*layoutType] = std::move(data);
        }
    }

    void GuiLayoutManager::PersistLayoutsToDisk_()
    {
        if (layoutStorageFile_.empty())
            layoutStorageFile_ = ResolveLayoutStoragePath_();

        if (layoutStorageFile_.empty())
            return;

        std::filesystem::path directory = layoutStorageFile_.parent_path();
        if (!directory.empty())
        {
            std::error_code ec;
            std::filesystem::create_directories(directory, ec);
        }

        std::ofstream file(layoutStorageFile_, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
            return;

        file << kFileVersionLine << '\n';
        file << kActiveLayoutPrefix << LayoutTypeToString(currentLayout_) << '\n';

        constexpr std::array<EditorLayoutType, 2> layoutOrder = {
            EditorLayoutType::Scene,
            EditorLayoutType::ActorEditor
        };

        for (EditorLayoutType type : layoutOrder)
        {
            const auto it = layoutData_.find(type);
            const std::string* dataPtr = (it != layoutData_.end()) ? &it->second : nullptr;

            file << LayoutTypeToString(type) << '\n';
            const size_t size = dataPtr ? dataPtr->size() : 0;
            file << size << '\n';
            if (dataPtr && !dataPtr->empty())
                file.write(dataPtr->data(), dataPtr->size());
            file << '\n';
        }
    }

    std::filesystem::path GuiLayoutManager::ResolveLayoutStoragePath_() const
    {
        if (!ImGui::GetCurrentContext())
            return {};

        const ImGuiIO& io = ImGui::GetIO();
        std::filesystem::path basePath;
        if (io.IniFilename && io.IniFilename[0] != '\0')
            basePath = std::filesystem::path(io.IniFilename).parent_path();

        if (basePath.empty())
            basePath = std::filesystem::current_path();

        return basePath / kLayoutStorageFileName;
    }

    void GuiLayoutManager::TrimTrailingCarriageReturn_(std::string& value)
    {
        if (!value.empty() && value.back() == '\r')
            value.pop_back();
    }

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

    std::optional<EditorLayoutType> GuiLayoutManager::LayoutTypeFromString(const std::string& value)
    {
        if (value == "Scene")
            return EditorLayoutType::Scene;
        if (value == "ActorEditor")
            return EditorLayoutType::ActorEditor;

        return std::nullopt;
    }
}
