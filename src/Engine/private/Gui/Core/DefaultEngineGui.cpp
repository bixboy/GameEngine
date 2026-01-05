#include "Gui/Core/DefaultEngineGui.h"

#include <string>
#include <utility>
#include <algorithm>

#include "Gui/Panels/ActorInspectorPanel.h"
#include "Gui/Panels/SceneOutlinerPanel.h"
#include "Gui/Panels/SceneViewportPanel.h"
#include "Gui/Panels/StatsPanel.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserPanel.h"

#include "Gui/Core/GuiManager.h"
#include "Debug/Logger.h"
#include "Gui/Controllers/Windows/ActorEditorWindow.h"
#include "Gui/Controllers/Windows/AudioContainerEditorWindow.h"
#include "Gui/Controllers/Windows/ComponentEditorWindow.h"
#include "Gui/Controllers/Windows/SpriteAtlasEditorWindow.h"
#include "Gui/Core/GuiTheme.h"


namespace BixEngine::Gui
{
    DefaultEngineGuiPanelManager::DefaultEngineGuiPanelManager(GuiManager& guiManager, const DefaultEngineGuiContext& context)
        : guiManager_(&guiManager), context_(&context)
    {}

    template <typename PanelT, typename... Args>
    GuiPanel* DefaultEngineGuiPanelManager::TryRegisterPanel(const char* name, GuiManager::PanelDescriptor descriptor, Args&&... args)
    {
        try
        {
            auto registration = guiManager_->RegisterPanel<PanelT>(std::move(descriptor), std::forward<Args>(args)...);
            return &registration.panel;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("[DefaultEngineGui] Exception while creating panel '" + std::string(name) + "': " + e.what());
            return nullptr;
        }
    }

    DefaultEngineGuiPanels DefaultEngineGuiPanelManager::CreatePanels()
    {
        DefaultEngineGuiPanels panels{};

        if (!context_->bEnableGui)
        {
            LOG_INFO("[DefaultEngineGui] Headless mode — no GUI panels created.");
            return panels;
        }

        using Descriptor = GuiManager::PanelDescriptor;

        // 1. Scene Viewport
        auto viewportDesc = Descriptor{};
        viewportDesc.identifier   = "scene_viewport";
        viewportDesc.title        = "Scene";
        viewportDesc.dockRegion   = DockSpaceRegion::Center;
        viewportDesc.closable     = false;
        viewportDesc.collapsable  = false;
        viewportDesc.resizable    = true;
        viewportDesc.windowFlags  = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        viewportDesc.onInitialize = [](GuiPanel& panel) { 
            panel.SetBackgroundColor(Theme::ViewportBackground); 
            panel.SetMovable(true); 
        };
        
        panels.sceneViewportPanel = TryRegisterPanel<SceneViewportPanel>("SceneViewport", viewportDesc, *context_);

        // 2. Stats
        auto statsDesc = Descriptor{};
        statsDesc.identifier   = "engine_stats";
        statsDesc.title        = "Engine Stats";
        statsDesc.dockRegion   = DockSpaceRegion::Right;
        statsDesc.closable     = true;
        statsDesc.onInitialize = [](GuiPanel& panel) { panel.SetBackgroundColor(Theme::StatsBackground); };
        panels.statsPanel = TryRegisterPanel<StatsPanel>("Stats", statsDesc, *context_);

        // 3. Outliner
        auto outlinerDesc = Descriptor{};
        outlinerDesc.identifier   = "scene_outliner";
        outlinerDesc.title        = "Scene Outliner";
        outlinerDesc.dockRegion   = DockSpaceRegion::Left;
        outlinerDesc.closable     = true;
        outlinerDesc.onInitialize = [](GuiPanel& panel) { panel.SetBackgroundColor(Theme::OutlinerBackground); };
        panels.sceneOutlinerPanel = TryRegisterPanel<SceneOutlinerPanel>("SceneOutliner", outlinerDesc, *context_);

        // 4. Actor Inspector
        auto inspectorDesc = Descriptor{};
        inspectorDesc.identifier   = "actor_inspector";
        inspectorDesc.title        = "Actor Details";
        inspectorDesc.dockRegion   = DockSpaceRegion::Right;
        inspectorDesc.closable     = true;
        inspectorDesc.onInitialize = [](GuiPanel& panel) { panel.SetBackgroundColor(Theme::InspectorBackground); };
        panels.actorInspectorPanel = TryRegisterPanel<ActorInspectorPanel>("ActorInspector", inspectorDesc, *context_);

        // 5. Content Browser
        auto browserDesc = Descriptor{};
        browserDesc.identifier   = "content_browser";
        browserDesc.title        = "Content Browser";
        browserDesc.dockRegion   = DockSpaceRegion::Bottom;
        browserDesc.closable     = true;
        browserDesc.onInitialize = [](GuiPanel& panel) { panel.SetBackgroundColor(Theme::ContentBackground); };
        panels.contentBrowserPanel = TryRegisterPanel<ContentBrowserPanel>("ContentBrowser", browserDesc, *context_);

        panels.allPanels = {
            {"SceneViewport",   panels.sceneViewportPanel},
            {"Stats",           panels.statsPanel},
            {"SceneOutliner",   panels.sceneOutlinerPanel},
            {"ActorInspector",  panels.actorInspectorPanel},
            {"ContentBrowser",  panels.contentBrowserPanel},
        };

        return panels;
    }

    // --- Public API ---

    void SetupDefaultEditorBehaviors(GuiManager& guiManager, DefaultEngineGuiContext& context)
    {
        context.openAssetInEditor = [&guiManager](const std::filesystem::path& path)
        {
            std::string ext = path.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

            if (ext == ".bixatlas")
            {
                guiManager.OpenAssetEditor<SpriteAtlasEditorWindow>(path);
            }
            else if (ext == ".bixaudio")
            {
                guiManager.OpenAssetEditor<AudioContainerEditorWindow>(path);
            }
            else if (ext == ".bixprefab" || ext == ".prefab")
            {
                guiManager.OpenAssetEditor<ActorEditorWindow>(path, ActorEditorWindow::Section::Viewport);
            }
            else if (ext == ".bixcomp")
            {
                 guiManager.OpenAssetEditor<ComponentEditorWindow>(path, ComponentEditorWindow::Section::Inspector);
            }
            else
            {
                LOG_WARNING("[Editor] No editor registered for extension: " + ext);
            }
        };
    }

    DefaultEngineGuiPanels CreateDefaultEngineGui(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        DefaultEngineGuiPanelManager manager(guiManager, context);
        return manager.CreatePanels();
    }
}
