#include "Engine/Gui/GuiPanelBase.h"
#include "Engine/Gui/Internal/GuiPanel.h"

#include <utility>

namespace BixEngine::Gui
{
    GuiPanelBase::GuiPanelBase(String panelName) : panelName_(std::move(panelName))
    {
    }

    void GuiPanelBase::DrawHeader()
    {
    }

    void GuiPanelBase::DrawBody()
    {
    }

    void GuiPanelBase::HandleShortcuts()
    {
    }

    void GuiPanelBase::OnOpen()
    {
    }

    void GuiPanelBase::OnClose()
    {
    }

    void GuiPanelBase::SetVisible(bool visible) noexcept
    {
        bVisible_ = visible;
        if (panel_)
            panel_->SetVisible(visible);
    }

    void GuiPanelBase::RequestClose()
    {
        if (panel_)
            panel_->SetVisible(false);
    }

    void GuiPanelBase::OnAttach(GuiPanel& panel)
    {
        panel_ = &panel;
        panelName_ = panel.GetTitle();
        bVisible_ = panel.IsVisible();

        panel.OnOpen = [this]
        {
            bVisible_ = true;
            OnOpen();
        };

        panel.OnClose = [this]
        {
            bVisible_ = false;
            OnClose();
        };
    }

    void GuiPanelBase::OnDetach(GuiPanel& panel)
    {
        if (&panel == panel_)
            panel_ = nullptr;
        bVisible_ = false;
    }

    void GuiPanelBase::OnDraw(GuiPanel& panel)
    {
        panelName_ = panel.GetTitle();
        bVisible_ = panel.IsVisible();

        Draw();
        HandleShortcuts();

        bVisible_ = panel.IsVisible();
    }
}

