#pragma once
#include "Gui/GuiPanelBase.h"
#include "Gui/DefaultEngineGui.h"

namespace BixEngine::Gui
{
    class SceneViewportPanel : public GuiPanelBase
    {
    public:
        explicit SceneViewportPanel(const DefaultEngineGuiContext& context);
        ~SceneViewportPanel() override = default;

        void Draw() override;

    private:
        const DefaultEngineGuiContext context_;
    };

    GuiPanel& CreateSceneViewportPanel(GuiManager& guiManager, const DefaultEngineGuiContext& context);
}
