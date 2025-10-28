#pragma once

#include <memory>
#include <utility>
#include <SDL3/SDL_events.h>

namespace BixEngine
{
    namespace Core { class Window; class Timer; class SubsystemManager; }
    namespace Graphics { class Renderer; }
    namespace Gui { class GuiSystem; class GuiManager; class GuiPanel; }
    namespace Game { class Actor; }
}

struct SDL_Texture;

namespace BixEngine::Core
{
    /**
     * @brief Module principal gérant le système d'interface utilisateur (ImGui).
     */
    class GuiModule
    {
    public:
        GuiModule();
        ~GuiModule();

        // ────────────────────────────────────────────────
        // ⚙️ Cycle de vie
        // ────────────────────────────────────────────────

        /** Initialise le système ImGui et le gestionnaire de panneaux. */
        bool Initialize(Window& window, Graphics::Renderer& renderer);

        /** Libère toutes les ressources GUI et ImGui. */
        void Shutdown() noexcept;

        /** Vérifie si le système GUI est actuellement actif. */
        bool IsInitialized() const noexcept;

        // ────────────────────────────────────────────────
        // 🎮 Gestion des événements et du frame
        // ────────────────────────────────────────────────

        /**
         * @brief Transmet un événement SDL à ImGui.
         */
        bool ProcessEvent(const SDL_Event& event);

        /** Démarre un nouveau frame ImGui. */
        void BeginFrame();

        /**
         * @brief Termine le frame ImGui et effectue le rendu.
         */
        void Render(SubsystemManager& subsystems);

        // ────────────────────────────────────────────────
        // 🧩 Construction des panneaux par défaut
        // ────────────────────────────────────────────────

        /**
         * @brief Crée et configure tous les panneaux standards du moteur.
         * 
         * Exemple : Viewport, Outliner, Inspector, ContentBrowser, Stats.
         */
        void SetupDefaultGuiPanels(SubsystemManager& subsystems, const float* lastDeltaTimePointer);

        /** Indique si la souris est actuellement au-dessus du panneau Viewport. */
        bool IsMouseOverViewport() const noexcept;

        // ────────────────────────────────────────────────
        // 🪟 Gestion de la texture du viewport 3D
        // ────────────────────────────────────────────────

        /**
         * @brief S’assure que la texture SDL utilisée pour le viewport existe
         * 
         * Si la taille change ou la texture est invalide, elle est recréée.
         */
        bool EnsureSceneViewportTexture(Graphics::Renderer& renderer);

        /** Détruit la texture du viewport si elle existe. */
        void DestroySceneViewportTexture() noexcept;

        // ────────────────────────────────────────────────
        // 🔍 Accès aux composants internes
        // ────────────────────────────────────────────────

        /** Retourne la texture SDL utilisée pour afficher la scène. */
        SDL_Texture* GetSceneViewportTexture() const noexcept { return sceneViewportTexture_; }

        /** Retourne la taille actuelle de la texture de viewport. */
        std::pair<int, int> GetSceneViewportSize() const noexcept { return {sceneViewportWidth_, sceneViewportHeight_}; }

        /** Retourne un pointeur vers le gestionnaire GUI (GuiManager). */
        Gui::GuiManager* GetGuiManager() noexcept { return guiManager_.get(); }

    private:
        // ────────────────────────────────────────────────
        // 🧠 Données internes
        // ────────────────────────────────────────────────

        std::unique_ptr<Gui::GuiSystem> guiSystem_;
        std::unique_ptr<Gui::GuiManager> guiManager_;

        // Références directes vers les panneaux standards du moteur
        Gui::GuiPanel* statsPanel_{nullptr};
        Gui::GuiPanel* outlinerPanel_{nullptr};
        Gui::GuiPanel* contentBrowserPanel_{nullptr};
        Gui::GuiPanel* inspectorPanel_{nullptr};
        Gui::GuiPanel* viewportPanel_{nullptr};

        // Pointeurs vers des éléments du jeu liés à l’interface
        Game::Actor* selectedActor_{nullptr};
        const float* lastDeltaTime_{nullptr};

        // Gestion de la texture SDL contenant le rendu du viewport 3D
        SDL_Texture* sceneViewportTexture_{nullptr};
        int sceneViewportWidth_{0};
        int sceneViewportHeight_{0};
        bool sceneViewportTextureErrorLogged_{false};
    };
}
