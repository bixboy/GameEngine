#pragma once
#include "Containers/String.h"
#include "imgui.h"
#include "Gui/Core/DefaultEngineGui.h"


namespace BixEngine::Gui
{
    class GuiManager;
    class GuiPanel;
    
    class GuiPanelWindow
    {
    public:
        virtual ~GuiPanelWindow() = default;

        GuiPanelWindow(const GuiPanelWindow&) = delete;
        GuiPanelWindow& operator=(const GuiPanelWindow&) = delete;
        GuiPanelWindow(GuiPanelWindow&&) noexcept = delete;
        GuiPanelWindow& operator=(GuiPanelWindow&&) noexcept = delete;

        // --- Cycle de vie ---
        void AttachToPanel(GuiPanel& panel);
        void DetachFromPanel();
        
        // --- Liaison Manager ---
        void BindManager(GuiManager& manager) noexcept { guiManager_ = &manager; }
        void UnbindManager() noexcept { guiManager_ = nullptr; }

        // --- Rendu ---
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
        GuiPanelWindow() = default;

        [[nodiscard]] GuiManager* GetGuiManager() noexcept { return guiManager_; }
        [[nodiscard]] const GuiManager* GetGuiManager() const noexcept { return guiManager_; }

        GuiPanel& OpenChildWindow(const ChildPanelConfig& config); 
        void CloseChildWindows();

        bool NavigateBack();
        bool NavigateForward();
        void NavigateHome();
        void NavigateToPanel(const String& name);

        virtual void OnAttach(GuiPanel& panel) { (void)panel; }
        virtual void OnDetach(GuiPanel& panel) { (void)panel; }
        
        virtual void OnDraw(GuiPanel& panel) = 0;

    private:
        GuiPanel* panel_{nullptr};
        GuiManager* guiManager_{nullptr};
    };
}
