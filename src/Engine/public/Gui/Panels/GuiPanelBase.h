#pragma once
#include "Containers/String.h"
#include "Gui/Controllers/GuiPanelWindow.h"


namespace BixEngine::Gui
{
    class GuiPanel;
    enum class DockSpaceRegion;
    
    class GuiPanelBase : public GuiPanelWindow
    {
    public:
        explicit GuiPanelBase(String panelName);
        ~GuiPanelBase() override = default;

        GuiPanelBase(const GuiPanelBase&) = delete;
        GuiPanelBase& operator=(const GuiPanelBase&) = delete;
        
        GuiPanelBase(GuiPanelBase&&) noexcept = delete;
        GuiPanelBase& operator=(GuiPanelBase&&) noexcept = delete;

        virtual void Draw();
        
        virtual void DrawHeader();
        
        virtual void DrawBody() = 0;

        // --- Events ---
        virtual void HandleShortcuts();
        virtual void OnOpen();
        virtual void OnClose();

        // --- Gestion État ---
        [[nodiscard]] bool IsVisible() const noexcept { return bVisible_; }
        void SetVisible(bool visible) noexcept;

        [[nodiscard]] const String& GetPanelName() const noexcept { return panelName_; }

        [[nodiscard]] GuiPanel* GetPanel() noexcept override { return panel_; }

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