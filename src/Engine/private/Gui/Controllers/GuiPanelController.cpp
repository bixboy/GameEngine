#include "Gui/Controllers/GuiPanelController.h"

#include <stdexcept>

#include "Gui/GuiManager.h"
#include "Gui/Internal/GuiPanel.h"

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

    GuiPanel* GuiPanelController::GetPanel() noexcept
    {
        return panel_;
    }

    const GuiPanel* GuiPanelController::GetPanel() const noexcept
    {
        return panel_;
    }

    GuiPanel& GuiPanelController::OpenChildPanel(const ChildPanelConfig& config)
    {
        if (!guiManager_)
            throw std::runtime_error("GuiPanelController::OpenChildPanel — aucun GuiManager associé.");

        return guiManager_->OpenChildPanel(*this, config);
    }

    void GuiPanelController::CloseChildPanels()
    {
        if (!guiManager_)
            return;

        guiManager_->CloseChildPanels(*this);
    }

    bool GuiPanelController::NavigateBack()
    {
        if (!guiManager_)
            return false;

        return guiManager_->NavigateBack();
    }

    bool GuiPanelController::NavigateForward()
    {
        if (!guiManager_)
            return false;

        return guiManager_->NavigateForward();
    }

    void GuiPanelController::NavigateHome()
    {
        if (!guiManager_)
            return;

        guiManager_->NavigateHome();
    }

    void GuiPanelController::NavigateToPanel(const String& name)
    {
        if (!guiManager_)
            return;

        guiManager_->FocusPanel(name);
    }
}
