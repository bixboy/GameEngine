#pragma once
#include "Gui/Base/GuiPanelBase.h"
#include "Gui/Core/DefaultEngineGui.h"

namespace BixEngine::Gui
{
    class SceneViewportPanel : public GuiPanelBase
    {
    public:
        explicit SceneViewportPanel(const DefaultEngineGuiContext& context);
        ~SceneViewportPanel() override = default;

        void Draw() override;

    private:
        DefaultEngineGuiContext context_;
    };
}
