#include "Gui/Internal/NavBar/GuiAssetEditorManager.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <ranges>
#include <string_view>
#include <Gui/Internal/GuiLayoutManager.h>
#include <nlohmann/json.hpp>

#include "Logger.h"
#include "Gui/GuiManager.h"
#include "Gui/Controllers/ActorEditorController.h"
#include "Gui/Controllers/ComponentEditorController.h"
#include "Gui/Controllers/SpriteAtlasEditorController.h"
#include "Gui/Panels/GuiPanel.h"
#include "Utils/EditorUtils.h"
#include "Utils/StringUtils.h"


namespace BixEngine::Core
{
    namespace
    {
        constexpr std::string_view kSceneNavigationId{"scene"};

        String JsonValueToString(const nlohmann::json& value)
        {
            if (value.is_string())
                return String(value.get<std::string>());

            if (value.is_boolean())
                return value.get<bool>() ? String{"true"} : String{"false"};

            if (value.is_number_integer() || value.is_number_unsigned())
                return String(std::to_string(value.get<long long>()));

            if (value.is_number_float())
                return String(std::format("{}", value.get<double>()));

            if (value.is_null())
                return String{"null"};

            return String(value.dump());
        }

        void PopulateVariablesMetadata(const nlohmann::json& document,
                                       Gui::BaseAssetEditorController::SharedState& state)
        {
            state.exposedVariables.clear();

            const auto it = document.find("variables");
            if (it == document.end() || !it->is_array())
                return;

            for (const auto& entry : *it)
            {
                if (!entry.is_object())
                    continue;

                Gui::BaseAssetEditorController::SharedState::VariableMetadata metadata;

                if (const auto nameIt = entry.find("name"); nameIt != entry.end() && nameIt->is_string())
                    metadata.name = nameIt->get<std::string>();

                if (const auto typeIt = entry.find("type"); typeIt != entry.end() && typeIt->is_string())
                    metadata.type = typeIt->get<std::string>();
                else if (const auto valueTypeIt = entry.find("valueType");
                         valueTypeIt != entry.end() && valueTypeIt->is_string())
                    metadata.type = valueTypeIt->get<std::string>();

                const nlohmann::json* defaultCandidate = nullptr;
                if (const auto defaultIt = entry.find("default"); defaultIt != entry.end())
                    defaultCandidate = &(*defaultIt);
                else if (const auto valueIt = entry.find("value"); valueIt != entry.end())
                    defaultCandidate = &(*valueIt);
                else if (const auto initialIt = entry.find("initial"); initialIt != entry.end())
                    defaultCandidate = &(*initialIt);

                if (defaultCandidate)
                    metadata.defaultValue = JsonValueToString(*defaultCandidate);

                state.exposedVariables.push_back(std::move(metadata));
            }
        }
    }

    std::span<GuiPanel*> GuiAssetEditorManager::PanelSet::CopyTo(std::span<GuiPanel*> buffer) const noexcept
    {
        std::size_t index = 0;
        auto push = [&](GuiPanel* panel)
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

    GuiAssetEditorManager::GuiAssetEditorManager(Gui::GuiManager& guiManager, Gui::GuiLayoutManager* layoutManager,
                                                 FocusRequestCallback focusRequestCallback,
                                                 FocusSceneCallback focusSceneCallback)
        : guiManager_(&guiManager), layoutManager_(layoutManager),
          focusRequestCallback_(std::move(focusRequestCallback)), focusSceneCallback_(std::move(focusSceneCallback))
    {
    }

    void GuiAssetEditorManager::SetLayoutManager(Gui::GuiLayoutManager* layoutManager) noexcept
    {
        layoutManager_ = layoutManager;
    }

    void GuiAssetEditorManager::SetFocusCallbacks(FocusRequestCallback focusRequestCallback,
                                                  FocusSceneCallback focusSceneCallback)
    {
        focusRequestCallback_ = std::move(focusRequestCallback);
        focusSceneCallback_ = std::move(focusSceneCallback);
    }

    void GuiAssetEditorManager::SwitchToLayout(EditorLayoutType layout, std::string_view navId, GuiPanel* panelToFocus)
    {
        activeNavigationId_ = std::string(navId);
        activeLayout_ = layout;

        if (layoutManager_)
            layoutManager_->Switch(layout);

        RefreshAssetPanelsVisibility();

        if (layout == EditorLayoutType::ActorEditor)
            FocusPanel(panelToFocus);

        else if (layout == EditorLayoutType::Scene)
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

        GuiPanel* focusPanel = storedEntry.panels.viewport ? storedEntry.panels.viewport : storedEntry.panels.inspector;
        if (!focusPanel)
            focusPanel = storedEntry.panels.toolbar;

        SwitchToLayout(EditorLayoutType::ActorEditor, storedEntry.navigationId, focusPanel);
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

        GuiPanel* focusPanel = entry->panels.viewport ? entry->panels.viewport : entry->panels.inspector;
        if (!focusPanel)
            focusPanel = entry->panels.toolbar;

        SwitchToLayout(EditorLayoutType::ActorEditor, navigationId, requestFocus ? focusPanel : nullptr);
    }

    void GuiAssetEditorManager::ActivateScene(bool requestFocus)
    {
        SwitchToLayout(EditorLayoutType::Scene, kSceneNavigationId, requestFocus ? nullptr : nullptr);
    }

    void GuiAssetEditorManager::RefreshAssetPanelsVisibility()
    {
        const bool assetLayoutActive = (layoutManager_ && layoutManager_->GetCurrentLayout() ==
            EditorLayoutType::ActorEditor) || activeLayout_ == EditorLayoutType::ActorEditor;

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
        layoutManager_->RegisterPanels(Gui::EditorLayoutType::ActorEditor, span,
                                       Gui::GuiLayoutManager::LayoutRegistrationMode::LoadIfUninitialized);
    }

    std::span<Gui::GuiPanel*> GuiAssetEditorManager::CollectPanels(const PanelSet& panels,
                                                                   PanelBuffer& buffer) const noexcept
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
        const std::string extensionLower = StringUtils::ToLowerCopy(path.extension().generic_string());
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
        else if (extensionLower == ".atlas")
        {
            typeTag = "atlas";
            assetType = "Sprite Atlas";
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
        else if (assetType == "Component Prefab")
            sharedState =
                Gui::ComponentEditorController::CreateSharedState(path, String(navigationId.c_str()), onClose);
        else
            sharedState = Gui::SpriteAtlasEditorController::CreateSharedState(
                path, String(navigationId.c_str()), onClose);

        if (!sharedState)
        {
            LOG_ERROR("[GuiAssetEditorManager] Failed to create shared state for asset editor.");
            return false;
        }

        if (assetType == "Actor Prefab" || assetType == "Component Prefab")
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
        else if (assetType == "Component Prefab")
            created = CreateComponentPrefabEditor(path, entry, navigationId);
        else
            created = CreateSpriteAtlasEditor(path, entry, navigationId);

        if (!created)
            return false;

        outEntry = std::move(entry);
        return true;
    }

    bool GuiAssetEditorManager::CreateActorPrefabEditor(const std::filesystem::path& path, AssetEditorEntry& entry,
                                                        const std::string& navigationId)
    {
        static_cast<void>(path);
        if (!guiManager_)
            return false;

        auto makePanel = [&](std::string_view suffix, Gui::ActorEditorController::Section section) -> Gui::GuiPanel*
        {
            const std::string panelId = std::format("{}_{}", navigationId, suffix);
            Gui::GuiPanel& panel = guiManager_->CreatePanel(String(panelId.c_str()), String{"Actor Prefab"});
            panel.SetVisible(false);
            guiManager_->AttachController(
                panel, std::make_unique<Gui::ActorEditorController>(entry.sharedState, section));
            return &panel;
        };

        entry.panels.toolbar = makePanel("toolbar", Gui::ActorEditorController::Section::Toolbar);
        entry.panels.viewport = makePanel("viewport", Gui::ActorEditorController::Section::Viewport);
        entry.panels.outline = makePanel("outline", Gui::ActorEditorController::Section::Outline);
        entry.panels.inspector = makePanel("inspector", Gui::ActorEditorController::Section::Inspector);

        return true;
    }

    bool GuiAssetEditorManager::CreateComponentPrefabEditor(const std::filesystem::path& path, AssetEditorEntry& entry,
                                                            const std::string& navigationId)
    {
        static_cast<void>(path);
        if (!guiManager_)
            return false;

        auto makePanel = [&](std::string_view suffix, ComponentEditorController::Section section) -> GuiPanel*
        {
            const std::string panelId = std::format("{}_{}", navigationId, suffix);
            GuiPanel& panel = guiManager_->CreatePanel(String(panelId.c_str()), String{"Component Prefab"});
            panel.SetVisible(false);
            
            guiManager_->AttachController(panel, std::make_unique<ComponentEditorController>(entry.sharedState, section));
            return &panel;
        };

        entry.panels.toolbar = makePanel("toolbar", ComponentEditorController::Section::Toolbar);
        entry.panels.inspector = makePanel("inspector", ComponentEditorController::Section::Inspector);
        entry.panels.viewport = nullptr;
        entry.panels.outline = nullptr;

        return true;
    }

    bool GuiAssetEditorManager::CreateSpriteAtlasEditor(const std::filesystem::path& path, AssetEditorEntry& entry, const String& navigationId)
    {
        static_cast<void>(path);
        if (!guiManager_)
            return false;

        auto atlasState = std::static_pointer_cast<SpriteAtlasEditorController::SharedState>(entry.sharedState);
        if (!atlasState)
            return false;

        const std::string panelId = std::format("{}_atlas", navigationId);
        GuiPanel& panel = guiManager_->CreatePanel(String(panelId.c_str()), String{"Sprite Atlas"});
        panel.SetVisible(false);
        guiManager_->AttachController(panel, std::make_unique<SpriteAtlasEditorController>(atlasState));
        entry.panels.viewport = &panel;
        entry.panels.toolbar = nullptr;
        entry.panels.outline = nullptr;
        entry.panels.inspector = nullptr;
        return true;
    }

    void GuiAssetEditorManager::PopulatePrefabMetadata(const std::filesystem::path& path, BaseAssetEditorController::SharedState& state)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            LOG_WARNING("[GuiAssetEditorManager] Unable to open asset file: " + path.generic_string());
            return;
        }

        std::string contents((std::istreambuf_iterator(file)), std::istreambuf_iterator<char>());

        nlohmann::json document = nlohmann::json::parse(contents, nullptr, false);
        if (document.is_discarded() || !document.is_object())
        {
            LOG_WARNING("[GuiAssetEditorManager] Invalid prefab metadata for: " + path.generic_string());
            return;
        }

        if (const auto classIt = document.find("class"); classIt != document.end() && classIt->is_string())
            state.primaryClassName = classIt->get<std::string>();
        else
            state.primaryClassName.clear();

        if (const auto includeIt = document.find("include"); includeIt != document.end() && includeIt->is_string())
            state.includePath = includeIt->get<std::string>();
        else
            state.includePath.clear();

        PopulateVariablesMetadata(document, state);
    }

    std::string GuiAssetEditorManager::MakeNavigationId(const std::filesystem::path& path, std::string_view typeTag) const
    {
        const std::string hash = std::format("{:x}", EditorUtils::HashFNV1a(path.generic_string()));
        return std::format("asset_{}_{}", typeTag, hash);
    }
}
