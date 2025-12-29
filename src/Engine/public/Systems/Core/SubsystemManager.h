#pragma once
#include <memory>
#include <SDL3/SDL_events.h>
#include "Framework/SceneManager.h"
#include "Input.h"
#include "InputManager.h"
#include "Time/Timer.h"


namespace BixEngine
{
    namespace Graphics
    {
        class Renderer;
    }

    namespace Gui
    {
        class GuiManager;
    }

    namespace Game
    {
        class SceneManager;
        class Scene;
        class Actor;
    }

    namespace Input
    {
        class Input;
        class InputManager;
    }

    namespace Core
    {
        class Window;
        class Timer;
    }
}

namespace BixEngine::Core
{
    class SubsystemManager
    {
    public:
        SubsystemManager() = default;
        ~SubsystemManager();


        
        bool Initialize(Graphics::Renderer& renderer, Window& window, Gui::GuiManager* guiManager);


        
        void Shutdown() noexcept;


        
        bool Restart(Graphics::Renderer& renderer, Window& window, Gui::GuiManager* guiManager);


        
        void ResetInput() noexcept;


        
        void ProcessEvent(const SDL_Event& event);


        
        void UpdateAll(float deltaTime);

        void UpdateRuntime(float deltaTime);
        void UpdateEditor(float deltaTime);
        void UpdatePaused(float deltaTime);

        void NotifyMouseEventDropped() noexcept;


        
        bool ShouldQuit() const noexcept;


        
        Timer* GetTimer() noexcept;
        const Timer* GetTimer() const noexcept;
        Input::InputManager* GetInputManager() noexcept;
        Input::Input* GetInputDevice() noexcept;
        const Input::Input* GetInputDevice() const noexcept;
        Game::SceneManager* GetSceneManager() noexcept;
        Game::Scene* GetActiveScene() noexcept;
        Game::Scene* GetScene() noexcept;
        const Game::Scene* GetScene() const noexcept;


        
        template <typename TScene, typename... Args>
        TScene& EmplaceScene(Args&&... args);


        
        bool IsInitialized() const noexcept { return initialized_; }

    private:
        bool initialized_{false};

        std::unique_ptr<Timer> timer_{};
        std::unique_ptr<Input::Input> input_{};
        std::unique_ptr<Input::InputManager> inputManager_{};
        std::unique_ptr<Game::SceneManager> sceneManager_{};
    };
}

namespace BixEngine::Core
{
    template <typename TScene, typename... Args>
    TScene& SubsystemManager::EmplaceScene(Args&&... args)
    {
        if (!sceneManager_)
            sceneManager_ = std::make_unique<Game::SceneManager>();

        return sceneManager_->EmplaceScene<TScene>(std::forward<Args>(args)...);
    }
}
