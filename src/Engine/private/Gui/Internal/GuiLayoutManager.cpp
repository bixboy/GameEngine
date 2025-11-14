#include "Gui/Internal/GuiLayoutManager.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <format>
#include <optional>
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
        constexpr std::string_view kFileVersionLineV1 = "Version:1";
        constexpr std::string_view kFileVersionLineV2 = "Version:2";
        constexpr std::string_view kActiveLayoutPrefix = "Active=";
        constexpr std::string_view kRegionIdsPrefix = "RegionIds=";

        using DockRegionArray = std::array<ImGuiID, static_cast<std::size_t>(DockSpaceRegion::Count)>;

        std::string SerializeRegionIds(const DockRegionArray& ids)
        {
            std::string result{"RegionIds="};
            for (std::size_t i = 0; i < ids.size(); ++i)
            {
                result += std::format("0x{:08X}", ids[i]);
                if (i + 1 < ids.size())
                    result += ',';
            }
            return result;
        }

        std::string_view TrimWhitespace(std::string_view value)
        {
            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
                value.remove_prefix(1);

            while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
                value.remove_suffix(1);

            return value;
        }

        DockRegionArray ParseRegionIds(const std::string& line)
        {
            DockRegionArray ids{};
            if (line.rfind(kRegionIdsPrefix.data(), 0) != 0)
                return ids;

            std::string_view values(line);
            values.remove_prefix(kRegionIdsPrefix.size());

            std::size_t index = 0;
            while (!values.empty() && index < ids.size())
            {
                const std::size_t commaPos = values.find(',');
                std::string_view token = (commaPos == std::string_view::npos)
                                             ? values
                                             : values.substr(0, commaPos);

                token = TrimWhitespace(token);

                if (!token.empty())
                {
                    try
                    {
                        const unsigned long parsed = std::stoul(std::string(token), nullptr, 0);
                        ids[index] = static_cast<ImGuiID>(parsed);
                    }
                    catch (...)
                    {
                        ids[index] = 0;
                    }
                }

                ++index;
                if (commaPos == std::string_view::npos)
                    break;

                values.remove_prefix(commaPos + 1);
            }

            return ids;
        }
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

        StoredLayout record{};
        record.serialized = guiSystem_->SaveLayoutToMemory();
        record.dockRegionIds = guiSystem_->GetDockRegionIds();
        layoutData_[currentLayout_] = std::move(record);
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

        DockRegionArray regionIds{};

        auto it = layoutData_.find(layout);
        if (it != layoutData_.end())
        {
            const StoredLayout& record = it->second;
            if (!record.serialized.empty())
                guiSystem_->LoadLayoutFromMemory(record.serialized);
            else
                guiSystem_->RequestDefaultDockLayout();

            regionIds = record.dockRegionIds;
        }
        else
        {
            guiSystem_->RequestDefaultDockLayout();
        }

        guiSystem_->SetDockRegionIds(regionIds);

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

        std::optional<std::string> pendingLine;
        bool versionParsed = false;
        bool supportsRegionIds = false;

        std::string line;
        while (true)
        {
            if (pendingLine)
            {
                line = std::move(*pendingLine);
                pendingLine.reset();
            }
            else if (!std::getline(file, line))
            {
                break;
            }

            TrimTrailingCarriageReturn_(line);
            if (line.empty())
                continue;

            if (!versionParsed && (line == kFileVersionLineV1 || line == kFileVersionLineV2))
            {
                supportsRegionIds = (line == kFileVersionLineV2);
                versionParsed = true;
                continue;
            }

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

            StoredLayout record{};
            if (dataSize > 0)
            {
                record.serialized.resize(dataSize);
                file.read(record.serialized.data(), dataSize);
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

            if (supportsRegionIds)
            {
                std::string regionLine;
                if (std::getline(file, regionLine))
                {
                    TrimTrailingCarriageReturn_(regionLine);
                    if (!regionLine.empty())
                    {
                        if (regionLine.rfind(kRegionIdsPrefix.data(), 0) == 0)
                        {
                            record.dockRegionIds = ParseRegionIds(regionLine);
                        }
                        else
                        {
                            pendingLine = std::move(regionLine);
                        }
                    }
                }
            }

            layoutData_[*layoutType] = std::move(record);
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

        file << kFileVersionLineV2 << '\n';
        file << kActiveLayoutPrefix << LayoutTypeToString(currentLayout_) << '\n';

        constexpr std::array<EditorLayoutType, 2> layoutOrder = {
            EditorLayoutType::Scene,
            EditorLayoutType::ActorEditor
        };

        for (EditorLayoutType type : layoutOrder)
        {
            const auto it = layoutData_.find(type);
            const StoredLayout* record = (it != layoutData_.end()) ? &it->second : nullptr;
            const std::string* dataPtr = record ? &record->serialized : nullptr;

            file << LayoutTypeToString(type) << '\n';
            const size_t size = dataPtr ? dataPtr->size() : 0;
            file << size << '\n';
            if (dataPtr && size > 0)
                file.write(dataPtr->data(), size);
            file << '\n';

            DockRegionArray regionIds{};
            if (record)
                regionIds = record->dockRegionIds;
            file << SerializeRegionIds(regionIds) << '\n';
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
