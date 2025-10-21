#include "Bix/Engine/Gui/DefaultEngineGui.h"

#include "Bix/Engine/Gui/Panels/ActorInspectorPanel.h"
#include "Bix/Engine/Gui/Panels/ContentBrowserPanel.h"
#include "Bix/Engine/Gui/Panels/SceneOutlinerPanel.h"
#include "Bix/Engine/Gui/Panels/SceneViewportPanel.h"
#include "Bix/Engine/Gui/Panels/StatsPanel.h"

namespace BixEngine::Gui
{
    namespace
    {
        class DefaultEngineGuiPanelManager
        {
        public:
            DefaultEngineGuiPanelManager(GuiManager& guiManager, const DefaultEngineGuiContext& context)
            : guiManager_(guiManager), context_(context){}

            DefaultEngineGuiPanels CreatePanels()
            {
                DefaultEngineGuiPanels panels{};
                panels.sceneViewportPanel = &CreateSceneViewportPanel(guiManager_, context_);
                panels.statsPanel = &CreateStatsPanel(guiManager_, context_);
                panels.sceneOutlinerPanel = &CreateSceneOutlinerPanel(guiManager_, context_);
                panels.actorInspectorPanel = &CreateActorInspectorPanel(guiManager_, context_);
                panels.contentBrowserPanel = &CreateContentBrowserPanel(guiManager_, context_);
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
