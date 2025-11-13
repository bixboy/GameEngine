#pragma once
#include "Containers/String.h"
#include "Gui/Controllers/GuiPanelController.h"


namespace BixEngine::Gui
{
    class GuiPanel;

    /**
     * @brief Base class encapsulating the life-cycle of an ImGui panel.
     *
     * GuiPanelBase exposes a high level API that mirrors the requirements of the
     * editor architecture: a single Draw() entry point complemented by overridable
     * hooks for the header, body and shortcuts handling. The implementation bridges
     * the low level GuiPanel window with the high level controller model that was
     * already present in the engine, effectively removing the need for bespoke
     * boilerplate inside each panel.
     */
    class GuiPanelBase : public GuiPanelController
    {
    public:
        explicit GuiPanelBase(String panelName);
        ~GuiPanelBase() override = default;

        GuiPanelBase(const GuiPanelBase&) = delete;
        GuiPanelBase& operator=(const GuiPanelBase&) = delete;
        GuiPanelBase(GuiPanelBase&&) noexcept = delete;
        GuiPanelBase& operator=(GuiPanelBase&&) noexcept = delete;

        /** Top level draw method to be implemented by derived classes. */
        virtual void Draw() = 0;

        /** Optional header section. */
        virtual void DrawHeader();

        /** Optional body section. */
        virtual void DrawBody();

        /** Shortcut processing executed after Draw(). */
        virtual void HandleShortcuts();

        /** Called when the panel is opened (visibility toggled to true). */
        virtual void OnOpen();

        /** Called when the panel is closed (visibility toggled to false). */
        virtual void OnClose();

        [[nodiscard]] bool IsVisible() const noexcept { return bVisible_; }
        void SetVisible(bool visible) noexcept;

        [[nodiscard]] const String& GetPanelName() const noexcept { return panelName_; }

        /** Provides access to the underlying GuiPanel window object. */
        [[nodiscard]] GuiPanel* GetPanel() noexcept { return panel_; }
        [[nodiscard]] const GuiPanel* GetPanel() const noexcept { return panel_; }

    protected:
        void RequestClose();

    private:
        void OnAttach(GuiPanel& panel) override;
        void OnDetach(GuiPanel& panel) override;
        void OnDraw(GuiPanel& panel) override;

        String panelName_;
        GuiPanel* panel_{nullptr};
        bool bVisible_{true};
    };
}

