#pragma once

#include <memory>

namespace BixEngine::Gui
{
    class GuiPanel;

    /**
     * Base class for high level panel controllers.
     *
     * Controllers encapsulate the behaviour and rendering logic for a GuiPanel.
     * They can be attached to any panel through GuiManager and reused across
     * different contexts which keeps GuiPanel itself agnostic of editor logic.
     */
    class GuiPanelController
    {
    public:
        virtual ~GuiPanelController() = default;

        GuiPanelController(const GuiPanelController&) = delete;
        GuiPanelController& operator=(const GuiPanelController&) = delete;
        GuiPanelController(GuiPanelController&&) noexcept = delete;
        GuiPanelController& operator=(GuiPanelController&&) noexcept = delete;

        /**
         * Called by GuiManager when the controller is attached to a panel.
         */
        void AttachToPanel(GuiPanel& panel);

        /**
         * Called by GuiManager before the controller is destroyed or detached.
         */
        void DetachFromPanel();

        /**
         * Executes the draw routine of the controller for the attached panel.
         */
        void DrawPanel();

    protected:
        GuiPanelController() = default;

        [[nodiscard]] GuiPanel& GetPanel() noexcept;
        [[nodiscard]] const GuiPanel& GetPanel() const noexcept;

        virtual void OnAttach(GuiPanel& panel) {}
        virtual void OnDetach(GuiPanel& panel) {}
        virtual void OnDraw(GuiPanel& panel) = 0;

    private:
        GuiPanel* panel_{nullptr};
    };
}

#define BIX_REGISTER_GUI_PANEL(manager, PanelType, nameLiteral, titleLiteral, ...) \
    (manager).RegisterUtilityPanel<PanelType>((nameLiteral), (titleLiteral), ##__VA_ARGS__)

