#pragma once

#include <functional>
#include <vector>

#include "Containers/String.h"
#include "Gui/GuiDocking.h"

namespace BixEngine::Gui
{
    class GuiManager;
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

    protected:
        GuiPanelController() = default;

        [[nodiscard]] GuiPanel& GetPanel() noexcept;
        [[nodiscard]] const GuiPanel& GetPanel() const noexcept;

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

#define BIX_REGISTER_GUI_PANEL(manager, PanelType, nameLiteral, titleLiteral, ...) \
    (manager).RegisterUtilityPanel<PanelType>((nameLiteral), (titleLiteral), ##__VA_ARGS__)
