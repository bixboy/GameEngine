#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <sstream>
#include <SDL3/SDL_events.h>

#include "Gui/Internal/NavBar/GuiAssetEditorManager.h"
#include "Gui/Internal/NavBar/GuiNavigationBar.h"


namespace BixEngine
{
    namespace Core
    {
        class Window;
        class Timer;
        class SubsystemManager;
    }

    namespace Graphics
    {
        class Renderer;
    }

    namespace Gui
    {
        class GuiSystem;
        class GuiManager;
        class GuiPanel;
        class ActorEditorController;
        class GuiLayoutManager;
        enum class EditorLayoutType;
    }

    namespace Game
    {
        class Actor;
    }
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
        enum class EngineState { Edit, Play, Pause, Step };

    public:
        GuiModule();
        ~GuiModule() noexcept;

        /** Initialise le système ImGui et le gestionnaire de panneaux. */
        bool Initialize(Window& window, Graphics::Renderer& renderer);

        /** Libère toutes les ressources GUI et ImGui. */
        void Shutdown() noexcept;

        /** Vérifie si le système GUI est actuellement actif. */
        bool IsInitialized() const noexcept;

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
        

        /**
         * @brief Crée et configure tous les panneaux standards du moteur.
         */
        void SetupDefaultGuiPanels(SubsystemManager& subsystems, const float* lastDeltaTimePointer);

        /** Indique si la souris est actuellement au-dessus du panneau Viewport. */
        bool IsMouseOverViewport() const noexcept;
        

        /**
         * @brief S’assure que la texture SDL utilisée pour le viewport existe
         */
        bool EnsureSceneViewportTexture(Graphics::Renderer& renderer);

        /** Détruit la texture du viewport si elle existe. */
        void DestroySceneViewportTexture() noexcept;

        /** Retourne la texture SDL utilisée pour afficher la scène. */
        SDL_Texture* GetSceneViewportTexture() const noexcept { return sceneViewportTexture_; }

        /** Retourne la taille actuelle de la texture de viewport. */
        std::pair<int, int> GetSceneViewportSize() const noexcept
        {
            return {sceneViewportWidth_, sceneViewportHeight_};
        }

        /** Retourne un pointeur vers le gestionnaire GUI (GuiManager). */
        GuiManager* GetGuiManager() noexcept { return guiManager_.get(); }

        /** Retourne un pointeur vers le gestionnaire des éditeurs d'assets. */
        GuiAssetEditorManager* GetAssetEditorManager() noexcept { return assetEditorManager_.get(); }
        const GuiAssetEditorManager* GetAssetEditorManager() const noexcept { return assetEditorManager_.get(); }
        
        // State Management
        void OnPlay();
        void OnStop();
        void OnPause();
        
        [[nodiscard]] EngineState GetEngineState() const noexcept { return m_EngineState; }


        
        // ────────────────────────────────────────────────
        // Données internes
        // ────────────────────────────────────────────────

        std::unique_ptr<GuiSystem> guiSystem_;
        std::unique_ptr<GuiManager> guiManager_;
        std::unique_ptr<GuiLayoutManager> layoutManager_;
        std::unique_ptr<GuiNavigationBar> navigationBar_;

        SubsystemManager* subsystems_{nullptr};

        // Références directes vers les panneaux standards du moteur
        GuiPanel* statsPanel_{nullptr};
        GuiPanel* outlinerPanel_{nullptr};
        GuiPanel* contentBrowserPanel_{nullptr};
        GuiPanel* inspectorPanel_{nullptr};
        GuiPanel* viewportPanel_{nullptr};

        // Pointeurs vers des éléments du jeu liés à l’interface
        Game::Actor* selectedActor_{nullptr};
        const float* lastDeltaTime_{nullptr};

        // Gestion de la texture SDL contenant le rendu du viewport 3D
        SDL_Texture* sceneViewportTexture_{nullptr};
        int sceneViewportWidth_{0};
        int sceneViewportHeight_{0};
        bool sceneViewportTextureErrorLogged_{false};

        std::unique_ptr<GuiAssetEditorManager> assetEditorManager_{};
        std::vector<std::string> focusRequests_{};
        bool bInitialized_{false};

        void OpenAssetEditor(const std::filesystem::path& path);
        void CloseAssetEditor(const std::string& navigationId);

        void ProcessFocusRequests();
        void FocusSceneViewport();

    private:
        void DispatchPendingFileDrops();



        std::vector<std::filesystem::path> pendingDroppedFiles_{};

        EngineState m_EngineState{EngineState::Edit};
        std::stringstream m_SceneBackup;
    };
}
