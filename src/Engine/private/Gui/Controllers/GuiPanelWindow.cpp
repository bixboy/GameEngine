#include "Gui/Controllers/GuiPanelWindow.h"

#include <stdexcept>

#include "Gui/Core/GuiManager.h"
#include "Gui/Panels/GuiPanel.h"

namespace BixEngine::Gui
{
    void GuiPanelWindow::AttachToPanel(GuiPanel& panel)
    {
        panel_ = &panel;
        OnAttach(panel);
    }

    void GuiPanelWindow::DetachFromPanel()
    {
        if (!panel_)
            return;

        OnDetach(*panel_);
        panel_ = nullptr;
    }

    void GuiPanelWindow::DrawPanel()
    {
        if (!panel_)
            return;

        OnDraw(*panel_);
    }

    GuiPanel* GuiPanelWindow::GetPanel() noexcept
    {
        return panel_;
    }

    const GuiPanel* GuiPanelWindow::GetPanel() const noexcept
    {
        return panel_;
    }

    GuiPanel& GuiPanelWindow::OpenChildWindow(const ChildPanelConfig& config)
    {
        if (!guiManager_)
        {
            throw std::runtime_error("GuiPanelWindow::OpenChildWindow — aucun GuiManager associé.");
        }

        return guiManager_->OpenChildPanel(*this, config);
    }

    void GuiPanelWindow::CloseChildWindows()
    {
        if (!guiManager_)
            return;

        guiManager_->CloseChildPanels(*this);
    }

    bool GuiPanelWindow::NavigateBack()
    {
        if (!guiManager_)
            return false;

        return guiManager_->NavigateBack();
    }

    bool GuiPanelWindow::NavigateForward()
    {
        if (!guiManager_)
            return false;

        return guiManager_->NavigateForward();
    }

    void GuiPanelWindow::NavigateHome()
    {
        if (!guiManager_)
            return;

        guiManager_->NavigateHome();
    }

    void GuiPanelWindow::NavigateToPanel(const String& name)
    {
        if (!guiManager_)
            return;

        guiManager_->FocusPanel(name);
    }
}
