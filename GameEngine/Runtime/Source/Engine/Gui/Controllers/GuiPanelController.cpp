#include "Engine/Gui/Controllers/GuiPanelController.h"
#include "Engine/Gui/Core/GuiPanel.h"


namespace BixEngine::Gui
{
    void GuiPanelController::AttachToPanel(GuiPanel& panel)
    {
        panel_ = &panel;
        OnAttach(panel);
    }

    void GuiPanelController::DetachFromPanel()
    {
        if (!panel_)
            return;

        OnDetach(*panel_);
        panel_ = nullptr;
    }

    void GuiPanelController::DrawPanel()
    {
        if (!panel_)
            return;

        OnDraw(*panel_);
    }

    GuiPanel& GuiPanelController::GetPanel() noexcept
    {
        return *panel_;
    }

    const GuiPanel& GuiPanelController::GetPanel() const noexcept
    {
        return *panel_;
    }
}

