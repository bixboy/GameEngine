#pragma once
#include <memory>
#include <SDL3/SDL_events.h>
#include "Framework/SceneManager.h"
#include "Input.h"
#include "InputManager.h"
#include "Time/Timer.h"

namespace BixEngine
{
    namespace Graphics { class Renderer; }
    
    namespace Gui { class GuiManager; }
    
    namespace Game
    {
        class SceneManager;
        class Scene;
    }
    
    namespace Core
    {
        class Window;
    }
    
}

namespace BixEngine::Core
{
    class SubsystemManager
    {
    public:
        SubsystemManager() = default;
        ~SubsystemManager();

        SubsystemManager(const SubsystemManager&) = delete;
        SubsystemManager& operator=(const SubsystemManager&) = delete;

        // --- Cycle de Vie ---
        
        bool Initialize(Graphics::Renderer& renderer, Window& window, Gui::GuiManager* guiManager);
        void Shutdown() noexcept;
        bool Restart(Graphics::Renderer& renderer, Window& window, Gui::GuiManager* guiManager);

        // --- Gestion des Événements Système ---

        void ProcessEvent(const SDL_Event& event);
        
        // Appelé quand la fenêtre change de taille
        void OnWindowResize(int width, int height);
        
        // Appelé quand la fenêtre est réduite
        void OnWindowMinimized();
        
        // Appelé quand la fenêtre revient au premier plan
        void OnWindowRestored();
        
        // Appelé quand un fichier est glissé-déposé sur la fenêtre
        void OnFileDrop(const char* filePath);

        // Réinitialise les entrées
        void ResetInput() noexcept;
        
        // Notifie l'input qu'un mouvement souris a été ignoré
        void NotifyMouseEventDropped() noexcept;

        // --- Mise à jour (Update Loop) ---

        void UpdateAll(float deltaTime);
        void UpdateRuntime(float deltaTime);
        void UpdateEditor(float deltaTime);
        void UpdatePaused(float deltaTime);

        [[nodiscard]] bool ShouldQuit() const noexcept;

        // --- Accesseurs ---

        [[nodiscard]] bool IsInitialized() const noexcept { return initialized_; }
        
        Timer* GetTimer() noexcept { return timer_.get(); }
        const Timer* GetTimer() const noexcept { return timer_.get(); }
        
        Input::InputManager* GetInputManager() noexcept { return inputManager_.get(); }
        
        Input::Input* GetInputDevice() noexcept { return input_.get(); }
        const Input::Input* GetInputDevice() const noexcept { return input_.get(); }
        
        Game::SceneManager* GetSceneManager() noexcept { return sceneManager_.get(); }
        
        Game::Scene* GetActiveScene() noexcept;

        [[nodiscard]] const Game::Scene* GetActiveScene() const noexcept;
        
        template <typename TScene, typename... Args>
        TScene& EmplaceScene(Args&&... args);

    private:
        bool initialized_{false};
        bool isPaused_{false};

        // Réff vers les systèmes externes (Non possédés)
        Graphics::Renderer* rendererRef_{nullptr};
        Window* windowRef_{nullptr};
        Gui::GuiManager* guiManagerRef_{nullptr};

        // Systèmes possédés (Owned)
        std::unique_ptr<Timer> timer_{};
        std::unique_ptr<Input::Input> input_{};
        std::unique_ptr<Input::InputManager> inputManager_{};
        std::unique_ptr<Game::SceneManager> sceneManager_{};
    };

    template <typename TScene, typename... Args>
    TScene& SubsystemManager::EmplaceScene(Args&&... args)
    {
        if (!sceneManager_)
            sceneManager_ = std::make_unique<Game::SceneManager>();

        return sceneManager_->EmplaceScene<TScene>(std::forward<Args>(args)...);
    }
}