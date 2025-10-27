#include "Engine/Systems/SubsystemManager.h"

#include <SDL3/SDL_keycode.h>

#include "Core/Logger.h"
#include "Core/Timer.h"
#include "Engine/Systems/Window.h"
#include "Game/Scene.h"
#include "Game/SceneManager.h"
#include "Graphics/Renderer.h"
#include "Input/Input.h"
#include "Input/InputManager.h"

namespace BixEngine::Core
{
    SubsystemManager::~SubsystemManager()
    {
        if (initialized_)
            Shutdown();
    }

    bool SubsystemManager::Initialize(Graphics::Renderer& renderer, Window& window, Gui::GuiManager* guiManager)
    {
        if (initialized_)
        {
            LOG_WARNING("SubsystemManager already initialized.");
            return true;
        }

        LOG_INFO("Initializing SubsystemManager...");
        
        timer_ = std::make_unique<Timer>();
        input_ = std::make_unique<Input::Input>();
        inputManager_ = std::make_unique<Input::InputManager>();

        if (inputManager_)
            inputManager_->SetInputDevice(input_.get());

        if (!sceneManager_)
            sceneManager_ = std::make_unique<Game::SceneManager>();

        if (!sceneManager_)
        {
            LOG_ERROR("Failed to create SceneManager.");
            return false;
        }

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

        initialized_ = true;
        LOG_INFO("SubsystemManager initialized successfully.");
        return true;
    }

    void SubsystemManager::Shutdown() noexcept
    {
        if (!initialized_)
            return;

        LOG_INFO("Shutting down SubsystemManager...");
        
        if (sceneManager_)
            sceneManager_->SetScene(nullptr);

        sceneManager_.reset();

        if (inputManager_)
            inputManager_->SetInputDevice(nullptr);

        inputManager_.reset();
        input_.reset();
        timer_.reset();

        initialized_ = false;
        LOG_INFO("SubsystemManager shut down.");
    }

    bool SubsystemManager::Restart(Graphics::Renderer& renderer, Window& window, Gui::GuiManager* guiManager)
    {
        LOG_INFO("Restarting SubsystemManager...");
        Shutdown();
        return Initialize(renderer, window, guiManager);
    }

    void SubsystemManager::ResetInput() noexcept
    {
        if (input_)
            input_->ResetState();

        if (inputManager_)
            inputManager_->Reset();
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
