#include "Bix/Core/SubsystemManager.h"

#include <SDL3/SDL_keycode.h>

#include "Bix/Core/Timer.h"
#include "Bix/Core/Window.h"
#include "Bix/Game/Scene.h"
#include "Bix/Game/SceneManager.h"
#include "Bix/Graphics/Renderer.h"
#include "Bix/Input/Input.h"
#include "Bix/Input/InputManager.h"

namespace BixEngine::Core
{
    bool SubsystemManager::Initialize(Graphics::Renderer& renderer, Window& window, Gui::GuiManager* guiManager)
    {
        timer_ = std::make_unique<Timer>();
        input_ = std::make_unique<Input::Input>();
        inputManager_ = std::make_unique<Input::InputManager>();

        if (inputManager_)
            inputManager_->SetInputDevice(input_.get());

        if (!sceneManager_)
            sceneManager_ = std::make_unique<Game::SceneManager>();

        if (sceneManager_)
        {
            sceneManager_->SetContext({
                &renderer,
                inputManager_.get(),
                &window,
                timer_.get(),
                guiManager
            });
        }

        return true;
    }

    void SubsystemManager::Shutdown() noexcept
    {
        if (sceneManager_)
            sceneManager_->SetScene(nullptr);

        sceneManager_.reset();

        if (inputManager_)
            inputManager_->SetInputDevice(nullptr);

        inputManager_.reset();
        input_.reset();
        timer_.reset();
    }

    void SubsystemManager::ProcessEvent(const SDL_Event& event)
    {
        if (input_)
            input_->ProcessEvent(event);

        if (inputManager_)
            inputManager_->ProcessEvent(event);

        if (sceneManager_)
        {
            if (Game::Scene* scene = sceneManager_->GetScene())
                scene->HandleEvent(event);
        }
    }

    void SubsystemManager::UpdateAll(float deltaTime)
    {
        if (inputManager_)
            inputManager_->Update();

        if (sceneManager_)
        {
            if (Game::Scene* scene = sceneManager_->GetScene())
                scene->Update(deltaTime);
        }
    }

    bool SubsystemManager::ShouldQuit() const noexcept
    {
        return input_ && (input_->IsQuitRequested() || input_->IsKeyDown(SDLK_ESCAPE));
    }

    Timer* SubsystemManager::GetTimer() noexcept
    {
        return timer_.get();
    }

    const Timer* SubsystemManager::GetTimer() const noexcept
    {
        return timer_.get();
    }

    Input::InputManager* SubsystemManager::GetInputManager() noexcept
    {
        return inputManager_.get();
    }

    Game::SceneManager* SubsystemManager::GetSceneManager() noexcept
    {
        return sceneManager_.get();
    }

    Game::Scene* SubsystemManager::GetActiveScene() noexcept
    {
        return sceneManager_ ? sceneManager_->GetScene() : nullptr;
    }
}
