#include "Gui/DefaultEngineGui.h"

#include "Gui/Panels/ActorInspectorPanel.h"
#include "Gui/Panels/SceneOutlinerPanel.h"
#include "Gui/Panels/SceneViewportPanel.h"
#include "Gui/Panels/StatsPanel.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserPanel.h"

#include "Logger.h"

namespace BixEngine::Gui
{
    // ==========================================================================
    // Construction
    // ==========================================================================

    DefaultEngineGuiPanelManager::DefaultEngineGuiPanelManager(GuiManager& guiManager, const DefaultEngineGuiContext& context)
        : guiManager_(guiManager), context_(context)
    {}

    // ==========================================================================
    // TryCreatePanel — gestion sécurisée
    // ==========================================================================

    template <typename CreateFunc>
    GuiPanel* DefaultEngineGuiPanelManager::TryCreatePanel(const char* name, CreateFunc&& func)
    {
        try
        {
            using ReturnType = decltype(func(guiManager_, context_));
            GuiPanel* panel;

            if constexpr (std::is_pointer_v<ReturnType>)
                panel = func(guiManager_, context_);
            else
                panel = &func(guiManager_, context_);

            if (!panel)
                LOG_WARNING("[DefaultEngineGui] Panel '" + std::string(name) + "' returned NULL.");

            return panel;
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

        panels.sceneViewportPanel   = TryCreatePanel("SceneViewport",   CreateSceneViewportPanel);
        panels.statsPanel           = TryCreatePanel("Stats",           CreateStatsPanel);
        panels.sceneOutlinerPanel   = TryCreatePanel("SceneOutliner",   CreateSceneOutlinerPanel);
        panels.actorInspectorPanel  = TryCreatePanel("ActorInspector",  CreateActorInspectorPanel);
        panels.contentBrowserPanel  = TryCreatePanel("ContentBrowser",  CreateContentBrowserPanel);

        panels.allPanels =
        {
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
