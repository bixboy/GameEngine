#include "Gui/DefaultEngineGui.h"

#include <string>
#include <utility>

#include "Gui/Panels/ActorInspectorPanel.h"
#include "Gui/Panels/SceneOutlinerPanel.h"
#include "Gui/Panels/SceneViewportPanel.h"
#include "Gui/Panels/StatsPanel.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserPanel.h"
#include "Gui/GuiCommon.h"

#include "Logger.h"

namespace BixEngine::Gui
{
    DefaultEngineGuiPanelManager::DefaultEngineGuiPanelManager(GuiManager& guiManager, const DefaultEngineGuiContext& context)
        : guiManager_(guiManager), context_(context)
    {}

    // ==========================================================================
    // TryRegisterPanel — gestion sécurisée
    // ==========================================================================

    template <typename PanelT, typename... Args>
    GuiPanel* DefaultEngineGuiPanelManager::TryRegisterPanel(const char* name, GuiManager::PanelDescriptor descriptor, Args&&... args)
    {
        try
        {
            auto registration = guiManager_.RegisterPanel<PanelT>(std::move(descriptor), std::forward<Args>(args)...);
            return &registration.panel;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("[DefaultEngineGui] Exception while creating panel '" + std::string(name) + "': " + e.what());
            return nullptr;
        }
    }

    // ==========================================================================
    // Création de tous les panneaux
    // ==========================================================================

    DefaultEngineGuiPanels DefaultEngineGuiPanelManager::CreatePanels()
    {
        DefaultEngineGuiPanels panels{};

        if (!context_.bEnableGui)
        {
            LOG_INFO("[DefaultEngineGui] Headless mode — no GUI panels created.");
            return panels;
        }

        using Descriptor = GuiManager::PanelDescriptor;


        // Scene viewport
        auto viewportDesc = Descriptor{};
        viewportDesc.identifier   = "scene_viewport";
        viewportDesc.title        = "Scene";
        viewportDesc.dockRegion   = DockSpaceRegion::Center;
        viewportDesc.closable     = false;
        viewportDesc.collapsable  = false;
        viewportDesc.resizable    = true;
        viewportDesc.windowFlags  = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
        viewportDesc.onInitialize = [](GuiPanel& panel)
        {
            panel.SetBackgroundColor(Theme::ViewportBackground);
            panel.SetMovable(true);
        };
        panels.sceneViewportPanel = TryRegisterPanel<SceneViewportPanel>("SceneViewport", viewportDesc, context_);


        // Engine stats
        auto statsDesc = Descriptor{};
        statsDesc.identifier   = "engine_stats";
        statsDesc.title        = "Engine Stats";
        statsDesc.dockRegion   = DockSpaceRegion::Right;
        statsDesc.closable     = true;
        statsDesc.collapsable  = true;
        statsDesc.resizable    = false;
        statsDesc.windowFlags  = ImGuiWindowFlags_NoCollapse;
        statsDesc.onInitialize = [](GuiPanel& panel)
        {
            panel.SetBackgroundColor(Theme::StatsBackground);
        };
        panels.statsPanel = TryRegisterPanel<StatsPanel>("Stats", statsDesc, context_);

        
        // Scene outliner
        auto outlinerDesc = Descriptor{};
        outlinerDesc.identifier   = "scene_outliner";
        outlinerDesc.title        = "Scene Outliner";
        outlinerDesc.dockRegion   = DockSpaceRegion::Left;
        outlinerDesc.closable     = true;
        outlinerDesc.collapsable  = true;
        outlinerDesc.windowFlags  = ImGuiWindowFlags_NoCollapse;
        outlinerDesc.onInitialize = [](GuiPanel& panel)
        {
            panel.SetBackgroundColor(Theme::OutlinerBackground);
        };
        panels.sceneOutlinerPanel = TryRegisterPanel<SceneOutlinerPanel>("SceneOutliner", outlinerDesc, context_);

        
        // Actor inspector
        auto inspectorDesc = Descriptor{};
        inspectorDesc.identifier   = "actor_inspector";
        inspectorDesc.title        = "Actor Details";
        inspectorDesc.dockRegion   = DockSpaceRegion::Right;
        inspectorDesc.closable     = true;
        inspectorDesc.collapsable  = true;
        inspectorDesc.windowFlags  = ImGuiWindowFlags_NoCollapse;
        inspectorDesc.onInitialize = [](GuiPanel& panel)
        {
            panel.SetBackgroundColor(Theme::InspectorBackground);
        };
        panels.actorInspectorPanel = TryRegisterPanel<ActorInspectorPanel>("ActorInspector", inspectorDesc, context_);

        // Content browser
        auto browserDesc = Descriptor{};
        browserDesc.identifier   = "content_browser";
        browserDesc.title        = "Content Browser";
        browserDesc.dockRegion   = DockSpaceRegion::Bottom;
        browserDesc.closable     = true;
        browserDesc.collapsable  = true;
        browserDesc.windowFlags  = ImGuiWindowFlags_NoCollapse;
        browserDesc.onInitialize = [](GuiPanel& panel)
        {
            panel.SetBackgroundColor(Theme::ContentBackground);
        };
        panels.contentBrowserPanel = TryRegisterPanel<ContentBrowserPanel>("ContentBrowser", browserDesc, context_);

        panels.allPanels = {
            
            {"SceneViewport",   panels.sceneViewportPanel},
            {"Stats",           panels.statsPanel},
            {"SceneOutliner",   panels.sceneOutlinerPanel},
            {"ActorInspector",  panels.actorInspectorPanel},
            {"ContentBrowser",  panels.contentBrowserPanel},
        };

        return panels;
    }
    

    DefaultEngineGuiPanels CreateDefaultEngineGui(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        DefaultEngineGuiPanelManager manager(guiManager, context);
        return manager.CreatePanels();
    }
}
