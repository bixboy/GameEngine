#pragma once
#include "Containers/String.h"
#include "Gui/Core/GuiCommon.h"
#include "imgui.h"


namespace BixEngine::Gui
{
    class GuiManager;
    class GuiPanel;

    /**
     * Base class for high level panel controllers.
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
         * @brief Attache le contrôleur au GuiManager actif.
         */
        void BindManager(GuiManager& manager) noexcept { guiManager_ = &manager; }

        /**
         * @brief Détache le contrôleur de tout GuiManager.
         */
        void UnbindManager() noexcept { guiManager_ = nullptr; }

        /**
         * Executes the draw routine of the controller for the attached panel.
         */
        void DrawPanel();

        enum class ChildPanelKind
        {
            FloatingWindow,
            SecondaryDocked,
            PersistentPopup
        };

        struct ChildPanelConfig
        {
            String name{};                ///< Identifiant interne stable
            String title{};               ///< Titre visible ImGui
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

        /**
         * @brief Crée ou récupère un panneau enfant géré par le GuiManager.
         */
        GuiPanel& OpenChildPanel(const ChildPanelConfig& config);

        /**
         * @brief Ferme tous les panneaux enfants appartenant à ce contrôleur.
         */
        void CloseChildPanels();

        /**
         * @brief Navigation dans l'historique global.
         */
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

