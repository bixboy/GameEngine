#include "Gui/DefaultEngineGui.h"

#include "Gui/Panels/ContentBrowserPanel.h"
#include "Gui/Panels/SceneOutlinerPanel.h"
#include "Gui/Panels/StatsPanel.h"

namespace Engine::Gui
{
    namespace
    {
        class DefaultEngineGuiPanelManager
        {
        public:
            DefaultEngineGuiPanelManager(GuiManager& guiManager, const DefaultEngineGuiContext& context)
                : guiManager_(guiManager)
                , context_(context)
            {
            }

            DefaultEngineGuiPanels CreatePanels()
            {
                DefaultEngineGuiPanels panels{};
                panels.statsPanel = &CreateStatsPanel(guiManager_, context_);
                panels.sceneOutlinerPanel = &CreateSceneOutlinerPanel(guiManager_, context_);
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
