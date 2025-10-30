#include "Engine/Gui/Core/NavBar/GuiAssetEditorManager.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <ranges>
#include <string_view>

#include "Core/Logger.h"
#include "Engine/Gui/Core/GuiManager.h"
#include "Engine/Gui/Core/GuiPanel.h"
#include "Engine/Utils/EditorUtils.h"

namespace BixEngine::Core
{
    namespace
    {
        constexpr std::string_view kSceneNavigationId{"scene"};

        std::string ToLowerCopy(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch)
            {
                return static_cast<char>(std::tolower(ch));
            });
            return value;
        }

        std::string ExtractJsonString(const std::string& source, std::string_view key)
        {
            const std::string pattern = std::string{"\""} + std::string(key) + "\"";
            size_t pos = source.find(pattern);
            if (pos == std::string::npos)
                return {};

            pos = source.find(':', pos);
            if (pos == std::string::npos)
                return {};

            pos = source.find('"', pos);
            if (pos == std::string::npos)
                return {};

            const size_t end = source.find('"', pos + 1);
            if (end == std::string::npos)
                return {};

            return source.substr(pos + 1, end - (pos + 1));
        }
    }

    std::span<Gui::GuiPanel*> GuiAssetEditorManager::PanelSet::CopyTo(std::span<Gui::GuiPanel*> buffer) const noexcept
    {
        std::size_t index = 0;
        auto push = [&](Gui::GuiPanel* panel)
        {
            if (!panel || index >= buffer.size())
                return;
            buffer[index++] = panel;
        };

        push(toolbar);
        push(viewport);
        push(outline);
        push(inspector);

        return buffer.first(index);
    }

    GuiAssetEditorManager::GuiAssetEditorManager(Gui::GuiManager& guiManager, Gui::GuiLayoutManager* layoutManager, FocusRequestCallback focusRequestCallback, FocusSceneCallback focusSceneCallback)
        : guiManager_(&guiManager), layoutManager_(layoutManager), focusRequestCallback_(std::move(focusRequestCallback)), focusSceneCallback_(std::move(focusSceneCallback))
    {
    }

    void GuiAssetEditorManager::SetLayoutManager(Gui::GuiLayoutManager* layoutManager) noexcept
    {
        layoutManager_ = layoutManager;
    }

    void GuiAssetEditorManager::SetFocusCallbacks(FocusRequestCallback focusRequestCallback, FocusSceneCallback focusSceneCallback)
    {
        focusRequestCallback_ = std::move(focusRequestCallback);
        focusSceneCallback_ = std::move(focusSceneCallback);
    }

    void GuiAssetEditorManager::SwitchToLayout(Gui::EditorLayoutType layout, std::string_view navId, Gui::GuiPanel* panelToFocus)
    {
        activeNavigationId_ = std::string(navId);
        activeLayout_ = layout;

        if (layoutManager_)
            layoutManager_->Switch(layout);

        RefreshAssetPanelsVisibility();

        if (layout == Gui::EditorLayoutType::ActorEditor)
            FocusPanel(panelToFocus);
        else if (layout == Gui::EditorLayoutType::Scene)
            RequestSceneFocus();
    }

    void GuiAssetEditorManager::OpenAssetEditor(const std::filesystem::path& path)
    {
        if (!guiManager_ || path.empty())
            return;

        std::filesystem::path normalized = path.lexically_normal();
        if (normalized.empty())
            normalized = path;

        if (auto it = editorsByPath_.find(normalized); it != editorsByPath_.end())
        {
            ActivateEditor(it->second, true);
            return;
        }

        AssetEditorEntry entry;
        if (!CreateAssetEditorEntry(normalized, entry))
            return;

        editorsByPath_[entry.assetPath] = entry.navigationId;
        assetEditorOrder_.push_back(entry.navigationId);
        assetEditors_[entry.navigationId] = std::move(entry);

        auto& storedEntry = assetEditors_.at(assetEditorOrder_.back());
        ApplyPanels(storedEntry);

        Gui::GuiPanel* focusPanel = storedEntry.panels.viewport ? storedEntry.panels.viewport : storedEntry.panels.inspector;
        if (!focusPanel)
            focusPanel = storedEntry.panels.toolbar;

        SwitchToLayout(Gui::EditorLayoutType::ActorEditor, storedEntry.navigationId, focusPanel);
    }

    void GuiAssetEditorManager::CloseAssetEditor(const std::string& navigationId)
    {
        auto it = assetEditors_.find(navigationId);
        if (it == assetEditors_.end())
            return;

        AssetEditorEntry entry = std::move(it->second);
        assetEditors_.erase(it);
        editorsByPath_.erase(entry.assetPath);

        DetachAndRemovePanels(entry.panels);
        entry.sharedState.reset();

        auto orderIt = std::find(assetEditorOrder_.begin(), assetEditorOrder_.end(), navigationId);
        if (orderIt != assetEditorOrder_.end())
            assetEditorOrder_.erase(orderIt);

        if (assetEditors_.empty())
        {
            if (layoutManager_)
                layoutManager_->ResetLayout(Gui::EditorLayoutType::ActorEditor);

            SwitchToLayout(Gui::EditorLayoutType::Scene, kSceneNavigationId);
        }
        else
        {
            const std::string& nextNav = assetEditorOrder_.back();
            ActivateEditor(nextNav, true);
        }
    }

    void GuiAssetEditorManager::ActivateEditor(std::string_view navigationId, bool requestFocus)
    {
        auto* entry = FindEditor(navigationId);
        if (!entry)
            return;

        ApplyPanels(*entry);

        Gui::GuiPanel* focusPanel = entry->panels.viewport ? entry->panels.viewport : entry->panels.inspector;
        if (!focusPanel)
            focusPanel = entry->panels.toolbar;

        SwitchToLayout(Gui::EditorLayoutType::ActorEditor, navigationId, requestFocus ? focusPanel : nullptr);
    }

    void GuiAssetEditorManager::ActivateScene(bool requestFocus)
    {
        SwitchToLayout(Gui::EditorLayoutType::Scene, kSceneNavigationId, requestFocus ? nullptr : nullptr);
    }

    void GuiAssetEditorManager::RefreshAssetPanelsVisibility()
    {
        const bool assetLayoutActive = (layoutManager_ && layoutManager_->GetCurrentLayout() == Gui::EditorLayoutType::ActorEditor) || activeLayout_ == Gui::EditorLayoutType::ActorEditor;

        for (auto& [navId, entry] : assetEditors_)
        {
            const bool visible = assetLayoutActive && activeNavigationId_ == navId;
            entry.panels.ForEachPanel([visible](Gui::GuiPanel* panel)
            {
                if (panel)
                    panel->SetVisible(visible);
            });
        }
    }

    void GuiAssetEditorManager::RemoveAllEditors()
    {
        for (auto& entry : assetEditors_ | std::views::values)
        {
            DetachAndRemovePanels(entry.panels);
            entry.sharedState.reset();
        }

        assetEditors_.clear();
        editorsByPath_.clear();
        assetEditorOrder_.clear();

        if (layoutManager_)
            layoutManager_->ResetLayout(Gui::EditorLayoutType::ActorEditor);

        SwitchToLayout(Gui::EditorLayoutType::Scene, kSceneNavigationId);
    }

    void GuiAssetEditorManager::OnLayoutChanged(Gui::EditorLayoutType layout) noexcept
    {
        activeLayout_ = layout;
    }

    GuiAssetEditorManager::AssetEditorEntry* GuiAssetEditorManager::FindEditor(std::string_view navigationId) noexcept
    {
        if (auto it = assetEditors_.find(std::string(navigationId)); it != assetEditors_.end())
            return &it->second;

        return nullptr;
    }

    void GuiAssetEditorManager::ApplyPanels(AssetEditorEntry& entry)
    {
        if (!layoutManager_)
            return;

        PanelBuffer buffer{};
        const auto span = CollectPanels(entry.panels, buffer);
        layoutManager_->RegisterPanels(Gui::EditorLayoutType::ActorEditor, span, Gui::GuiLayoutManager::LayoutRegistrationMode::LoadIfUninitialized);
    }

    std::span<Gui::GuiPanel*> GuiAssetEditorManager::CollectPanels(const PanelSet& panels, PanelBuffer& buffer) const noexcept
    {
        std::span spanBuffer{buffer};
        return panels.CopyTo(spanBuffer);
    }

    void GuiAssetEditorManager::DetachAndRemovePanels(const PanelSet& panels)
    {
        PanelBuffer buffer{};
        const auto span = CollectPanels(panels, buffer);
        if (layoutManager_)
            layoutManager_->DetachPanels(span);

        if (guiManager_)
            guiManager_->RemovePanels(span);
    }

    void GuiAssetEditorManager::FocusPanel(Gui::GuiPanel* panel) const
    {
        if (panel && focusRequestCallback_)
            focusRequestCallback_(panel->GetTitle().Std());
    }

    void GuiAssetEditorManager::RequestSceneFocus() const
    {
        if (focusSceneCallback_)
            focusSceneCallback_();
    }

    bool GuiAssetEditorManager::CreateAssetEditorEntry(const std::filesystem::path& path, AssetEditorEntry& outEntry)
    {
        const std::string extensionLower = ToLowerCopy(path.extension().generic_string());
        std::string typeTag;
        std::string assetType;

        if (extensionLower == ".bixactor")
        {
            typeTag = "actor";
            assetType = "Actor Prefab";
        }
        else if (extensionLower == ".bixcomponent")
        {
            typeTag = "component";
            assetType = "Component Prefab";
        }
        else
        {
            LOG_WARNING("[GuiAssetEditorManager] Unsupported asset extension: " + path.generic_string());
            return false;
        }

        const std::string navigationId = MakeNavigationId(path, typeTag);
        auto onClose = [this, navigationId]() { CloseAssetEditor(navigationId); };

        std::shared_ptr<Gui::BaseAssetEditorController::SharedState> sharedState;
        if (assetType == "Actor Prefab")
            sharedState = Gui::ActorEditorController::CreateSharedState(path, String(navigationId.c_str()), onClose);
        else
            sharedState = Gui::ComponentEditorController::CreateSharedState(path, String(navigationId.c_str()), onClose);

        if (!sharedState)
        {
            LOG_ERROR("[GuiAssetEditorManager] Failed to create shared state for asset editor.");
            return false;
        }

        PopulatePrefabMetadata(path, *sharedState);

        AssetEditorEntry entry{};
        entry.assetPath = path;
        entry.navigationId = navigationId;
        entry.buttonLabel = sharedState->assetDisplayName.Std();
        entry.metadata.extension = extensionLower;
        entry.metadata.assetType = assetType;
        entry.sharedState = sharedState;

        bool created = false;
        if (assetType == "Actor Prefab")
            created = CreateActorPrefabEditor(path, entry, navigationId);
        else
            created = CreateComponentPrefabEditor(path, entry, navigationId);

        if (!created)
            return false;

        outEntry = std::move(entry);
        return true;
    }

    bool GuiAssetEditorManager::CreateActorPrefabEditor(const std::filesystem::path& path, AssetEditorEntry& entry, const std::string& navigationId)
    {
        static_cast<void>(path);
        if (!guiManager_)
            return false;

        auto makePanel = [&](std::string_view suffix, Gui::ActorEditorController::Section section) -> Gui::GuiPanel*
        {
            const std::string panelId = std::format("{}_{}", navigationId, suffix);
            Gui::GuiPanel& panel = guiManager_->CreatePanel(String(panelId.c_str()), String{"Actor Prefab"});
            panel.SetVisible(false);
            guiManager_->AttachController(panel, std::make_unique<Gui::ActorEditorController>(entry.sharedState, section));
            return &panel;
        };

        entry.panels.toolbar = makePanel("toolbar", Gui::ActorEditorController::Section::Toolbar);
        entry.panels.viewport = makePanel("viewport", Gui::ActorEditorController::Section::Viewport);
        entry.panels.outline = makePanel("outline", Gui::ActorEditorController::Section::Outline);
        entry.panels.inspector = makePanel("inspector", Gui::ActorEditorController::Section::Inspector);

        return true;
    }

    bool GuiAssetEditorManager::CreateComponentPrefabEditor(const std::filesystem::path& path, AssetEditorEntry& entry, const std::string& navigationId)
    {
        static_cast<void>(path);
        if (!guiManager_)
            return false;

        auto makePanel = [&](std::string_view suffix, Gui::ComponentEditorController::Section section) -> Gui::GuiPanel*
        {
            const std::string panelId = std::format("{}_{}", navigationId, suffix);
            Gui::GuiPanel& panel = guiManager_->CreatePanel(String(panelId.c_str()), String{"Component Prefab"});
            panel.SetVisible(false);
            guiManager_->AttachController(panel, std::make_unique<Gui::ComponentEditorController>(entry.sharedState, section));
            return &panel;
        };

        entry.panels.toolbar = makePanel("toolbar", Gui::ComponentEditorController::Section::Toolbar);
        entry.panels.inspector = makePanel("inspector", Gui::ComponentEditorController::Section::Inspector);
        entry.panels.viewport = nullptr;
        entry.panels.outline = nullptr;

        return true;
    }

    void GuiAssetEditorManager::PopulatePrefabMetadata(const std::filesystem::path& path, Gui::BaseAssetEditorController::SharedState& state)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            LOG_WARNING("[GuiAssetEditorManager] Unable to open asset file: " + path.generic_string());
            return;
        }

        std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        std::string className = ExtractJsonString(contents, "class");
        if (!className.empty())
            state.primaryClassName = className;

        std::string includePath = ExtractJsonString(contents, "include");
        if (!includePath.empty())
            state.includePath = includePath;
    }

    std::string GuiAssetEditorManager::MakeNavigationId(const std::filesystem::path& path, std::string_view typeTag) const
    {
        const std::string hash = std::format("{:x}", EditorUtils::HashFNV1a(path.generic_string()));
        return std::format("asset_{}_{}", typeTag, hash);
    }
}
