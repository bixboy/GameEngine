#pragma once
#include <functional>
#include <memory>
#include <span>
#include <utility>
#include "Core/Containers/String.h"
#include "Engine/Gui/Core/GuiDocking.h"
#include "Engine/Gui/Core/GuiPanelRegistry.h"
#include "imgui.h"


namespace BixEngine::Gui
{
    class GuiSystem;
    class GuiPanel;
    class GuiPanelController;
    
    template <typename ControllerT>
    struct PanelRegistration
    {
        GuiPanel& panel;
        ControllerT& controller;
    };

    /**
     * @brief Gestionnaire central des panneaux ImGui du moteur.
     */
    class GuiManager
    {
    public:
        explicit GuiManager(GuiSystem& guiSystem);
        ~GuiManager();

        // ────────────────────────────────────────────────
        // 🧩 Création / Suppression
        // ────────────────────────────────────────────────

        /** Crée un panneau. */
        GuiPanel& CreatePanel(String name, String title);

        template <typename PanelT, typename... Args>
        PanelT& CreatePanelOfType(String name, String title, Args&&... args)
        {
            return registry_.AddPanelOfType<PanelT>(std::move(name), std::move(title), std::forward<Args>(args)...);
        }

        /** Supprime un panneau existant. */
        void RemovePanel(const String& name);

        /** Supprime plusieurs panneaux à partir de leurs pointeurs. */
        void RemovePanels(std::span<GuiPanel*> panels);

        // ────────────────────────────────────────────────
        // 🔍 Accès / Recherche
        // ────────────────────────────────────────────────

        /** Renvoie un pointeur vers le panneau demandé. */
        [[nodiscard]] GuiPanel* FindPanel(const String& name) noexcept;

        /** Version const de FindPanel(). */
        [[nodiscard]] const GuiPanel* FindPanel(const String& name) const noexcept;

        // ────────────────────────────────────────────────
        // 🧱 Docking
        // ────────────────────────────────────────────────

        /** Définit la zone de docking d’un panneau via son nom. */
        void SetPanelDockingArea(const String& name, DockSpaceRegion area, ImGuiCond condition = ImGuiCond_FirstUseEver);

        /** Définit la zone de docking d’un panneau via référence. */
        void SetPanelDockingArea(GuiPanel& panel, DockSpaceRegion area, ImGuiCond condition = ImGuiCond_FirstUseEver);

        // ────────────────────────────────────────────────
        // 🖼️ Rendu
        // ────────────────────────────────────────────────

        /** Dessine tous les panneaux enregistrés. */
        void DrawAll();

        /** Retourne la liste des panneaux actifs (modifiable). */
        [[nodiscard]] std::vector<GuiPanel*> GetPanels();

        /** Retourne la liste des panneaux actifs (lecture seule). */
        [[nodiscard]] std::vector<const GuiPanel*> GetPanels() const;

        // ────────────────────────────────────────────────
        // 🎮 Contrôleurs
        // ────────────────────────────────────────────────

        /** Attache un contrôleur à un panneau (via nom). */
        GuiPanelController& AttachController(const String& name, std::unique_ptr<GuiPanelController> controller);

        /** Attache un contrôleur à un panneau (via référence). */
        GuiPanelController& AttachController(GuiPanel& panel, std::unique_ptr<GuiPanelController> controller);

        /** Détache le contrôleur du panneau indiqué. */
        void DetachController(const String& name);

        /** Détache le contrôleur d’un panneau donné. */
        void DetachController(GuiPanel& panel);

        /** Récupère le contrôleur d’un panneau (modifiable). */
        GuiPanelController* GetController(const String& name) noexcept;

        /** Récupère le contrôleur d’un panneau (lecture seule). */
        const GuiPanelController* GetController(const String& name) const noexcept;

        /** 
         * @brief Renvoie le contrôleur s’il correspond au type demandé.
         */
        template <typename T>
        T* GetControllerAs(const String& name) noexcept
        {
            if (auto* base = GetController(name))
                return dynamic_cast<T*>(base);
            return nullptr;
        }

        /**
         * @brief Crée un panneau utilitaire et attache directement son contrôleur.
         * Utile pour initialiser des outils internes rapidement.
         */
        template <typename ControllerT, typename... Args>
        PanelRegistration<ControllerT> RegisterUtilityPanel(String name, String title, Args&&... args)
        {
            GuiPanel& panel = CreatePanel(std::move(name), std::move(title));
            auto controller = std::make_unique<ControllerT>(std::forward<Args>(args)...);
            ControllerT& controllerRef = static_cast<ControllerT&>(AttachController(panel, std::move(controller)));
            return {panel, controllerRef};
        }

        // ────────────────────────────────────────────────
        // 🔔 Callbacks
        // ────────────────────────────────────────────────

        std::function<void(GuiPanel&)> OnPanelCreated; ///< Notifié après création d’un panneau
        std::function<void(GuiPanel&)> OnPanelRemoved; ///< Notifié après suppression d’un panneau

    private:
        GuiSystem* guiSystem_{nullptr};
        GuiPanelRegistry registry_;

        void AttachDrawFunction_(GuiPanelRegistry::PanelEntry& entry);
    };
}
