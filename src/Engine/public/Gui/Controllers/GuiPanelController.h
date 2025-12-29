#pragma once
#include "Containers/String.h"
#include "Gui/Core/GuiCommon.h"
#include "imgui.h"


namespace BixEngine::Gui
{
    class GuiManager;
    class GuiPanel;

     
    class GuiPanelController
    {
    public:
        virtual ~GuiPanelController() = default;

        GuiPanelController(const GuiPanelController&) = delete;
        GuiPanelController& operator=(const GuiPanelController&) = delete;
        
        GuiPanelController(GuiPanelController&&) noexcept = delete;
        GuiPanelController& operator=(GuiPanelController&&) noexcept = delete;

         
        void AttachToPanel(GuiPanel& panel);

         
        void DetachFromPanel();

         
        void BindManager(GuiManager& manager) noexcept { guiManager_ = &manager; }

         
        void UnbindManager() noexcept { guiManager_ = nullptr; }

         
        void DrawPanel();

        enum class ChildPanelKind
        {
            FloatingWindow,
            SecondaryDocked,
            PersistentPopup
        };

        struct ChildPanelConfig
        {
            String name{};                
            String title{};               
            ChildPanelKind kind{ChildPanelKind::FloatingWindow};
            DockSpaceRegion dockRegion{DockSpaceRegion::Center};
            ImGuiCond dockCondition{ImGuiCond_Appearing};
            ImGuiWindowFlags windowFlags{ImGuiWindowFlags_None};
            bool closeWithParent{true};
            bool requestFocus{true};
        };

        [[nodiscard]] virtual GuiPanel* GetPanel() noexcept;
        [[nodiscard]] virtual const GuiPanel* GetPanel() const noexcept;

    protected:
        GuiPanelController() = default;
        

        [[nodiscard]] GuiManager* GetGuiManager() noexcept { return guiManager_; }
        [[nodiscard]] const GuiManager* GetGuiManager() const noexcept { return guiManager_; }

         
        GuiPanel& OpenChildPanel(const ChildPanelConfig& config);

         
        void CloseChildPanels();

         
        bool NavigateBack();
        bool NavigateForward();
        void NavigateHome();
        void NavigateToPanel(const String& name);

        virtual void OnAttach(GuiPanel& panel)
        {
            (void)panel;
        }

        virtual void OnDetach(GuiPanel& panel)
        {
            (void)panel;
        }

        virtual void OnDraw(GuiPanel& panel) = 0;

    private:
        GuiPanel* panel_{nullptr};
        GuiManager* guiManager_{nullptr};
    };
}

