#include "Gui/Internal/NavBar/GuiAssetEditorManager.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <fstream>
#include <nlohmann/json.hpp>
#include "Gui/Core/GuiManager.h"
#include "Gui/Panels/GuiPanel.h"
#include "Utils/String/StringUtils.h"
#include "Debug/Logger.h"
#include "Gui/Controllers/Windows/ActorEditorWindow.h"
#include "Gui/Controllers/Windows/AudioContainerEditorWindow.h"
#include "Gui/Controllers/Windows/ComponentEditorWindow.h"
#include "Gui/Controllers/Windows/SpriteAtlasEditorWindow.h"
#include "Utils/Editor/EditorUtils.h"
#include "Utils/FileIO/FilesUtils.h"


namespace BixEngine::Gui
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

        void PopulateVariablesMetadata(const nlohmann::json& document, BaseAssetEditorWindow::SharedState& state)
        {
            state.exposedVariables.clear();

            const auto it = document.find("variables");
            if (it == document.end() || !it->is_array())
                return;

            for (const auto& entry : *it)
            {
                if (!entry.is_object())
                    continue;

                BaseAssetEditorWindow::SharedState::VariableMetadata metadata;

                if (const auto nameIt = entry.find("name"); nameIt != entry.end() && nameIt->is_string())
                    metadata.name = nameIt->get<std::string>();

                if (const auto typeIt = entry.find("type"); typeIt != entry.end() && typeIt->is_string())
                {
                    metadata.type = typeIt->get<std::string>();
                }
                else if (const auto valueTypeIt = entry.find("valueType"); valueTypeIt != entry.end() && valueTypeIt->is_string())
                {
                    metadata.type = valueTypeIt->get<std::string>();
                }

                const nlohmann::json* defaultCandidate = nullptr;
                if (const auto defaultIt = entry.find("default"); defaultIt != entry.end())
                {
                    defaultCandidate = &(*defaultIt);
                }
                else if (const auto valueIt = entry.find("value"); valueIt != entry.end())
                {
                    defaultCandidate = &(*valueIt);
                }
                else if (const auto initialIt = entry.find("initial"); initialIt != entry.end())
                {
                    defaultCandidate = &(*initialIt);
                }

                if (defaultCandidate)
                    metadata.value = JsonValueToString(*defaultCandidate);

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

    GuiAssetEditorManager::GuiAssetEditorManager(GuiManager& guiManager, GuiLayoutManager* layoutManager,
        FocusRequestCallback focusRequestCallback, FocusSceneCallback focusSceneCallback) : guiManager_(&guiManager),
        layoutManager_(layoutManager), focusRequestCallback_(std::move(focusRequestCallback)), focusSceneCallback_(std::move(focusSceneCallback))
    {
    }

    void GuiAssetEditorManager::SetLayoutManager(GuiLayoutManager* layoutManager) noexcept
    {
        layoutManager_ = layoutManager;
    }

    void GuiAssetEditorManager::SetFocusCallbacks(FocusRequestCallback focusRequestCallback, FocusSceneCallback focusSceneCallback)
    {
        focusRequestCallback_ = std::move(focusRequestCallback);
        focusSceneCallback_ = std::move(focusSceneCallback);
    }

    void GuiAssetEditorManager::SwitchToLayout(const LayoutID& layout, std::string_view navId, GuiPanel* panelToFocus)
    {
        activeNavigationId_ = std::string(navId);
        activeLayout_ = layout;

        if (layoutManager_)
        {
            layoutManager_->Switch(layout);

            if (layout == DefaultLayouts::Scene)
            {
                layoutManager_->SetMenuPanelFilter(nullptr);
            }
            else
            {
                layoutManager_->SetMenuPanelFilter([this, currentNavId = std::string(navId)](GuiPanel* panel) -> bool
                {
                    auto it = assetEditors_.find(currentNavId);
                    if (it == assetEditors_.end()) return false;
                    
                    const auto& entry = it->second;
                    bool found = false;
                    entry.panels.ForEachPanel([panel, &found](Gui::GuiPanel* p)
                    {
                        if (p == panel) found = true;
                    });
                    return found;
                });
            }
        }

        RefreshAssetPanelsVisibility();

        if (layout != DefaultLayouts::Scene)
            FocusPanel(panelToFocus);

        else if (layout == DefaultLayouts::Scene)
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

        SwitchToLayout(storedEntry.layoutId, storedEntry.navigationId, focusPanel);
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
            {
                // Reset standard layouts if needed, or just the one we just closed?
                // For safety, we can leave this or remove it as ResetLayout essentially just hides panels
                // layoutManager_->ResetLayout(entry.layoutId); 
            }

            SwitchToLayout(DefaultLayouts::Scene, kSceneNavigationId);
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

        SwitchToLayout(entry->layoutId, navigationId, requestFocus ? focusPanel : nullptr);
    }

    void GuiAssetEditorManager::ActivateScene(bool requestFocus)
    {
        SwitchToLayout(DefaultLayouts::Scene, kSceneNavigationId, requestFocus ? nullptr : nullptr);
    }

    void GuiAssetEditorManager::RefreshAssetPanelsVisibility()
    {
        const bool isScene = activeLayout_ == DefaultLayouts::Scene;
        const bool assetLayoutActive = !isScene;

        for (auto& [navId, entry] : assetEditors_)
        {
            // Only visible if global asset mode active AND this specific editor is the active one
            // However, with per-layout system, layout manager handles visibility of registered panels automatically when switching layouts!
            // But we still need to ensure panels are set to Visible=true so the layout manager shows them.
            // The LayoutManager::Switch calls SetVisible(true) for panels in that layout.
            // But if we have multiple tabs sharing a layout (e.g. 2 Audio Containers), we need to manually hide the inactive one.
            
            const bool isSameLayout = entry.layoutId == activeLayout_;
            const bool visible = assetLayoutActive && activeNavigationId_ == navId && isSameLayout;
            
            entry.panels.ForEachPanel([visible](GuiPanel* panel)
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
            layoutManager_->ResetLayout(DefaultLayouts::ActorEditor);

        SwitchToLayout(DefaultLayouts::Scene, kSceneNavigationId);
    }

    void GuiAssetEditorManager::OnLayoutChanged(const LayoutID& layout) noexcept
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
        layoutManager_->RegisterPanels(entry.layoutId, span, GuiLayoutManager::LayoutRegistrationMode::LoadIfUninitialized);
    }

    std::span<GuiPanel*> GuiAssetEditorManager::CollectPanels(const PanelSet& panels, PanelBuffer& buffer) const noexcept
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
        const std::string extensionLower = StringUtils::Utilities::ToLowerCopy(path.extension().generic_string());
        std::string typeTag;
        std::string assetType;

        if (extensionLower == ".bixactor" || extensionLower == ".prefab" || extensionLower == ".bixprefab")
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
        else if (extensionLower == ".bixaudio")
        {
            typeTag = "audio_container";
            assetType = "Audio Container";
        }
        else
        {
            LOG_WARNING("[GuiAssetEditorManager] Unsupported asset extension: " + path.generic_string());
            return false;
        }

        LOG_INFO("[GuiAssetEditorManager] Detected type tag '" + typeTag + "' for path: " + path.generic_string());

        const std::string navigationId = MakeNavigationId(path, typeTag);
        auto onClose = [this, navigationId]()
        {
            CloseAssetEditor(navigationId);
        };

        std::shared_ptr<BaseAssetEditorWindow::SharedState> sharedState;
        if (assetType == "Actor Prefab")
        {
            sharedState = ActorEditorWindow::CreateSharedState(path, String(navigationId.c_str()), onClose);
        }
        else if (assetType == "Component Prefab")
        {
            sharedState = ComponentEditorWindow::CreateSharedState(path, String(navigationId.c_str()), onClose);
        }
        else if (assetType == "Sprite Atlas")
        {
            sharedState =SpriteAtlasEditorWindow::CreateSharedState(path, String(navigationId.c_str()), onClose);
        }
        else if (assetType == "Audio Container")
        {
            sharedState = AudioContainerEditorWindow::CreateSharedState(path, String(navigationId.c_str()), onClose);
        }

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

        // Assign Layout ID based on type
        if (assetType == "Actor Prefab") entry.layoutId = "Layout_ActorPrefab";
        else if (assetType == "Component Prefab") entry.layoutId = "Layout_ComponentPrefab";
        else if (assetType == "Sprite Atlas") entry.layoutId = "Layout_SpriteAtlas";
        else if (assetType == "Audio Container") entry.layoutId = "Layout_AudioContainer";
        else entry.layoutId = "Layout_GenericAsset";

        bool created = false;
        if (assetType == "Actor Prefab")
        {
            created = CreateActorPrefabEditor(path, entry, navigationId);
        }
        else if (assetType == "Component Prefab")
        {
            created = CreateComponentPrefabEditor(path, entry, navigationId);
        }
        else if (assetType == "Sprite Atlas")
        {
            created = CreateSpriteAtlasEditor(path, entry, navigationId);
        }
        else if (assetType == "Audio Container")
        {
            created = CreateAudioContainerEditor(path, entry, navigationId);
        }

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

        auto makePanel = [&](std::string_view suffix, ActorEditorWindow::Section section, DockSpaceRegion region) -> GuiPanel*
        {
            const std::string panelId = std::format("{}_{}", navigationId, suffix);
            
            GuiPanel& panel = guiManager_->CreatePanel(String(panelId.c_str()), String{"Actor Prefab"});
            panel.SetVisible(false);
            panel.SetDockingPreference(region);
            
            guiManager_->AttachController(panel, std::make_unique<ActorEditorWindow>(entry.sharedState, section));
            return &panel;
        };

        entry.panels.toolbar = makePanel("toolbar", ActorEditorWindow::Section::Toolbar, DockSpaceRegion::Top);
        entry.panels.viewport = makePanel("viewport", ActorEditorWindow::Section::Viewport, DockSpaceRegion::Center);
        entry.panels.outline = makePanel("outline", ActorEditorWindow::Section::Outline, DockSpaceRegion::Left);
        entry.panels.inspector = makePanel("inspector", ActorEditorWindow::Section::Inspector, DockSpaceRegion::Right);

        return true;
    }

    bool GuiAssetEditorManager::CreateComponentPrefabEditor(const std::filesystem::path& path, AssetEditorEntry& entry,
                                                            const std::string& navigationId)
    {
        static_cast<void>(path);
        if (!guiManager_)
            return false;

        auto makePanel = [&](std::string_view suffix, ComponentEditorWindow::Section section, DockSpaceRegion region) -> GuiPanel*
        {
            const std::string panelId = std::format("{}_{}", navigationId, suffix);
            GuiPanel& panel = guiManager_->CreatePanel(String(panelId.c_str()), String{"Component Prefab"});
            panel.SetVisible(false);
            panel.SetDockingPreference(region);
            
            guiManager_->AttachController(panel, std::make_unique<ComponentEditorWindow>(entry.sharedState, section));
            return &panel;
        };

        entry.panels.toolbar = makePanel("toolbar", ComponentEditorWindow::Section::Toolbar, DockSpaceRegion::Top);
        entry.panels.inspector = makePanel("inspector", ComponentEditorWindow::Section::Inspector, DockSpaceRegion::Right);
        entry.panels.viewport = nullptr;
        entry.panels.outline = nullptr;

        return true;
    }

    bool GuiAssetEditorManager::CreateSpriteAtlasEditor(const std::filesystem::path& path, AssetEditorEntry& entry, const String& navigationId)
    {
        static_cast<void>(path);
        if (!guiManager_)
            return false;

        auto atlasState = std::static_pointer_cast<SpriteAtlasEditorWindow::SharedState>(entry.sharedState);
        if (!atlasState)
            return false;

        const std::string panelId = std::format("{}_atlas", navigationId);
        GuiPanel& panel = guiManager_->CreatePanel(String(panelId.c_str()), String{"Sprite Atlas"});
        panel.SetVisible(false);
        panel.SetDockingPreference(DockSpaceRegion::Center);
        guiManager_->AttachController(panel, std::make_unique<SpriteAtlasEditorWindow>(atlasState));
        entry.panels.viewport = &panel;
        entry.panels.toolbar = nullptr;
        entry.panels.outline = nullptr;
        entry.panels.inspector = nullptr;
        return true;
    }

    bool GuiAssetEditorManager::CreateAudioContainerEditor(const std::filesystem::path& path, AssetEditorEntry& entry, const String& navigationId)
    {
        static_cast<void>(path);
        if (!guiManager_)
            return false;

        const std::string panelId = std::format("{}_audiocontainer", navigationId);
        GuiPanel& panel = guiManager_->CreatePanel(String(panelId.c_str()), String{"Audio Container"});
        panel.SetVisible(false);
        panel.SetDockingPreference(DockSpaceRegion::Center);
        guiManager_->AttachController(panel, std::make_unique<AudioContainerEditorWindow>(entry.sharedState));
        entry.panels.viewport = &panel; 
        entry.panels.toolbar = nullptr;
        entry.panels.outline = nullptr;
        entry.panels.inspector = nullptr;
        return true;
    }

    void GuiAssetEditorManager::PopulatePrefabMetadata(const std::filesystem::path& path, BaseAssetEditorWindow::SharedState& state)
    {
        if (path.extension() == ".bixactor" || path.extension() == ".prefab" || path.extension() == ".bixprefab")
        {
             
            return;
        }

        String content;
        if (!Utils::FileUtils::ReadFile(path, content))
        {
            LOG_WARNING("[GuiAssetEditorManager] Unable to open asset file: " + path.generic_string());
            return;
        }

        std::string contents = content.Std();

        nlohmann::json document = nlohmann::json::parse(contents, nullptr, false);
        if (document.is_discarded() || !document.is_object())
        {
            LOG_WARNING("[GuiAssetEditorManager] Invalid prefab metadata for: " + path.generic_string());
            return;
        }

        if (const auto classIt = document.find("class"); classIt != document.end() && classIt->is_string())
        {
            state.primaryClassName = classIt->get<std::string>();
        }
        else
        {
            state.primaryClassName.clear();
        }

        if (const auto includeIt = document.find("include"); includeIt != document.end() && includeIt->is_string())
        {
            state.includePath = includeIt->get<std::string>();
        }
        else
        {
            state.includePath.clear();
        }

        PopulateVariablesMetadata(document, state);
    }

    std::string GuiAssetEditorManager::MakeNavigationId(const std::filesystem::path& path, std::string_view typeTag) const
    {
        const std::string hash = std::format("{:x}", EditorUtils::Utilities::HashFNV1a(path.generic_string()));
        return std::format("asset_{}_{}", typeTag, hash);
    }
}
