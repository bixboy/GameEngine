#include "Gui/DefaultEngineGui.h"
#include "Gui/Panels/ActorInspectorPanel.h"
#include "Gui/Panels/SceneOutlinerPanel.h"
#include "Gui/Panels/SceneViewportPanel.h"
#include "Gui/Panels/StatsPanel.h"
#include "Gui/Panels/ContentBrowser/ContentBrowserPanel.h"

#include "Logger.h"

namespace BixEngine::Gui
{
    namespace
    {
        template <typename CreateFunc>
        GuiPanel* TryCreatePanel(const char* name, CreateFunc&& createFunc, GuiManager& gui,
                                 const DefaultEngineGuiContext& ctx)
        {
            try
            {
                using ReturnType = decltype(createFunc(gui, ctx));
                GuiPanel* panel = nullptr;

                if constexpr (std::is_pointer_v<ReturnType>)
                {
                    panel = createFunc(gui, ctx);
                }
                else if constexpr (std::is_lvalue_reference_v<ReturnType>)
                {
                    panel = &createFunc(gui, ctx);
                }

                if (!panel)
                    LOG_WARNING("[DefaultEngineGui] ⚠️ Panel '" + std::string(name) + "' returned null.");

                return panel;
            }
            catch (const std::exception& e)
            {
                LOG_ERROR(
                    "[DefaultEngineGui] ❌ Exception while creating panel '" + std::string(name) + "': " + e.what());
                return nullptr;
            }
        }

        class DefaultEngineGuiPanelManager
        {
        public:
            DefaultEngineGuiPanelManager(GuiManager& guiManager,
                                         const DefaultEngineGuiContext& context) : guiManager_(guiManager),
                context_(context)
            {
            }

            DefaultEngineGuiPanels CreatePanels()
            {
                DefaultEngineGuiPanels panels{};

                if (!context_.bEnableGui)
                {
                    LOG_INFO("[DefaultEngineGui] Mode headless actif — aucun panneau cree.");
                    return panels;
                }

                panels.sceneViewportPanel = TryCreatePanel("SceneViewport", CreateSceneViewportPanel, guiManager_,
                                                           context_);
                panels.statsPanel = TryCreatePanel("Stats", CreateStatsPanel, guiManager_, context_);
                panels.sceneOutlinerPanel = TryCreatePanel("SceneOutliner", CreateSceneOutlinerPanel, guiManager_,
                                                           context_);
                panels.actorInspectorPanel = TryCreatePanel("ActorInspector", CreateActorInspectorPanel, guiManager_,
                                                            context_);
                panels.contentBrowserPanel = TryCreatePanel("ContentBrowser", CreateContentBrowserPanel, guiManager_,
                                                            context_);

                // Enregistre les panneaux dans la map
                panels.allPanels =
                {
                    {"SceneViewport", panels.sceneViewportPanel},
                    {"Stats", panels.statsPanel},
                    {"SceneOutliner", panels.sceneOutlinerPanel},
                    {"ActorInspector", panels.actorInspectorPanel},
                    {"ContentBrowser", panels.contentBrowserPanel}
                };

                return panels;
            }

        private:
            GuiManager& guiManager_;
            const DefaultEngineGuiContext& context_;
        };
    }

    DefaultEngineGuiPanels CreateDefaultEngineGui(GuiManager& guiManager, const DefaultEngineGuiContext& context)
    {
        DefaultEngineGuiPanelManager manager(guiManager, context);
        return manager.CreatePanels();
    }
}
