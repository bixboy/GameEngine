#pragma once

#include <memory>
#include <SDL3/SDL_events.h>

namespace BixEngine
{
    namespace Graphics { class Renderer; }
    namespace Gui { class GuiManager; }
    namespace Game { class SceneManager; class Scene; class Actor; }
    namespace Input { class Input; class InputManager; }
    namespace Core { class Window; class Timer; }
}

#include "Input/Input.h"
#include "Input/InputManager.h"
#include "Core/Timer.h"

namespace BixEngine::Core
{
    class SubsystemManager
    {
    public:
        SubsystemManager() = default;
        ~SubsystemManager();

        bool Initialize(Graphics::Renderer& renderer, Window& window, Gui::GuiManager* guiManager);
        void Shutdown() noexcept;

        void ProcessEvent(const SDL_Event& event);
        void UpdateAll(float deltaTime);
        bool ShouldQuit() const noexcept;

        Timer* GetTimer() noexcept;
        const Timer* GetTimer() const noexcept;
        Input::InputManager* GetInputManager() noexcept;
        Game::SceneManager* GetSceneManager() noexcept;
        Game::Scene* GetActiveScene() noexcept;

        template <typename TScene, typename... Args>
        TScene& EmplaceScene(Args&&... args);

    private:
        std::unique_ptr<Timer> timer_{};
        std::unique_ptr<Input::Input> input_{};
        std::unique_ptr<Input::InputManager> inputManager_{};
        std::unique_ptr<Game::SceneManager> sceneManager_{};
    };
}

#include "Game/SceneManager.h"
#include "Game/Scene.h"

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
