#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <SDL3/SDL_events.h>

#include "Engine/Gui/Controllers/ActorEditorController.h"

namespace BixEngine
{
    namespace Core { class GuiNavigationBar; class Window; class Timer; class SubsystemManager; }
    namespace Graphics { class Renderer; }
    namespace Gui { class GuiSystem; class GuiManager; class GuiPanel; class ActorEditorController; class GuiLayoutManager; enum class EditorLayoutType; }
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
        ~GuiModule() noexcept;

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
        // 🪟 Gestion de la texture du viewpor
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

    public:
        // ────────────────────────────────────────────────
        // 🧠 Données internes
        // ────────────────────────────────────────────────

        std::unique_ptr<Gui::GuiSystem> guiSystem_;
        std::unique_ptr<Gui::GuiManager> guiManager_;
        std::unique_ptr<Gui::GuiLayoutManager> layoutManager_;
        std::unique_ptr<GuiNavigationBar> navigationBar_;

        SubsystemManager* subsystems_{nullptr};

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

        struct ActorEditorPanels
        {
            Gui::GuiPanel* toolbar{nullptr};
            Gui::GuiPanel* viewport{nullptr};
            Gui::GuiPanel* outline{nullptr};
            Gui::GuiPanel* inspector{nullptr};

            [[nodiscard]] std::size_t Count() const noexcept
            {
                std::size_t count = 0;
                if (toolbar) ++count;
                if (viewport) ++count;
                if (outline) ++count;
                if (inspector) ++count;
                return count;
            }

            template <typename Fn>
            void ForEachPanel(Fn&& fn) const
            {
                if (toolbar) std::forward<Fn>(fn)(toolbar);
                if (viewport) std::forward<Fn>(fn)(viewport);
                if (outline) std::forward<Fn>(fn)(outline);
                if (inspector) std::forward<Fn>(fn)(inspector);
            }

            [[nodiscard]] std::span<Gui::GuiPanel*> CopyTo(std::span<Gui::GuiPanel*> buffer) const noexcept
            {
                std::size_t index = 0;
                auto push = [&](Gui::GuiPanel* panel) noexcept
                {
                    if (!panel || index >= buffer.size())
                        return;
                    buffer[index++] = panel;
                };

                push(toolbar);
                push(viewport);
                push(outline);
                push(inspector);

                return buffer.first(index);
            }
        };

        struct ActorEditorEntry
        {
            std::filesystem::path assetPath;
            std::string navigationId;
            std::string buttonLabel;
            ActorEditorPanels panels{};
            std::shared_ptr<Gui::ActorEditorController::SharedState> sharedState{};
        };

        static constexpr std::size_t kActorEditorPanelCapacity = 4;

        struct PathHash
        {
            std::size_t operator()(const std::filesystem::path& value) const noexcept
            {
                return std::hash<std::string>{}(value.generic_string());
            }
        };

        using PanelBuffer = std::array<Gui::GuiPanel*, kActorEditorPanelCapacity>;

        std::unordered_map<std::string, ActorEditorEntry> actorEditors_{};
        std::unordered_map<std::filesystem::path, std::string, PathHash> actorEditorsByPath_{};
        std::vector<std::string> actorEditorOrder_{};
        std::vector<std::string> focusRequests_{};
        std::string activeNavigationId_{"scene"};
        Gui::EditorLayoutType activeLayout_{static_cast<Gui::EditorLayoutType>(0)};
        int nextActorEditorId_{};
        bool bInitialized_{false};

        void OpenActorEditor(const std::filesystem::path& path);
        void CloseActorEditor(const std::string& navigationId);
        
        void ProcessFocusRequests();
        void FocusSceneViewport();
        void RefreshActorPanelsVisibility();
        void ApplyActorEditorPanels(ActorEditorEntry& entry);
        [[nodiscard]] std::span<Gui::GuiPanel*> CollectPanels(const ActorEditorPanels& panels, PanelBuffer& buffer) const noexcept;
    };
}
