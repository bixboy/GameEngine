#include "Systems/Core/SubsystemManager.h"
#include <SDL3/SDL_keycode.h>
#include "Debug/Logger.h"
#include "Time/Timer.h"
#include "Framework/Scene.h"
#include "Framework/SceneManager.h"
#include "Renderer.h"
#include "Input.h"
#include "InputManager.h"
#include "Gui/Core/GuiManager.h"


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

        // 1. Stockage des références
        rendererRef_ = &renderer;
        windowRef_ = &window;
        guiManagerRef_ = guiManager;

        // 2. Création des sous-systèmes internes
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

        sceneManager_->SetContext({
            .renderer = &renderer,
            .inputManager = inputManager_.get(),
            .window = &window,
            .timer = timer_.get(),
            .guiManager = guiManager
        });

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

        rendererRef_ = nullptr;
        windowRef_ = nullptr;
        guiManagerRef_ = nullptr;

        initialized_ = false;
        LOG_INFO("SubsystemManager shut down.");
    }

    bool SubsystemManager::Restart(Graphics::Renderer& renderer, Window& window, Gui::GuiManager* guiManager)
    {
        LOG_INFO("Restarting SubsystemManager...");
        Shutdown();
        
        return Initialize(renderer, window, guiManager);
    }

    // --- Event Handlers ---

    void SubsystemManager::OnWindowResize(int width, int height)
    {
        LOG_INFO("Window resized to " + std::to_string(width) + "x" + std::to_string(height));

        if (rendererRef_)
        {
            rendererRef_->OnResize(width, height); 
        }

        if (guiManagerRef_)
        {
            guiManagerRef_->OnResize(width, height);
        }

        if (sceneManager_)
        {
            if (Game::Scene* scene = sceneManager_->GetScene())
            {
                scene->OnWindowResize(width, height);
            }
        }
    }

    void SubsystemManager::OnWindowMinimized()
    {
        LOG_INFO("Window minimized (Pausing engine).");
        isPaused_ = true;
        
        if (timer_) 
            timer_->SetTimeScale(0.0f);
    }

    void SubsystemManager::OnWindowRestored()
    {
        LOG_INFO("Window restored (Resuming engine).");
        isPaused_ = false;

        if (timer_)
            timer_->SetTimeScale(1.0f);
    }

    void SubsystemManager::OnFileDrop(const char* filePath)
    {
        LOG_INFO("File dropped: " + String(filePath));
        
        // TODO: Envoyer ça à un AssetManager ou charger la scène si c'est un .scene
    }

    void SubsystemManager::ResetInput() noexcept
    {
        if (input_)
            input_->ResetState();
        
        if (inputManager_)
            inputManager_->ResetState();
    }

    void SubsystemManager::NotifyMouseEventDropped() noexcept
    {
        if (input_)
            input_->NotifyMouseEventDropped();
    }

    void SubsystemManager::ProcessEvent(const SDL_Event& event)
    {
        if (input_)
            input_->ProcessEvent(event);
        
        if (inputManager_)
            inputManager_->ProcessEvent(event);

        if (sceneManager_)
        {
            if (auto* scene = sceneManager_->GetScene())
                scene->HandleEvent(event);
        }
    }

    // --- Updates ---

    void SubsystemManager::UpdateAll(float deltaTime)
    {
        if (isPaused_)
        {
            UpdatePaused(deltaTime);
            return;
        }

        UpdateRuntime(deltaTime);
    }

    void SubsystemManager::UpdateRuntime(float deltaTime)
    {
        if (input_)
            input_->UpdateStatistics(deltaTime);
        
        if (inputManager_)
            inputManager_->Update();

        if (sceneManager_)
        {
            if (auto* scene = sceneManager_->GetScene())
                scene->OnRuntimeUpdate(deltaTime);
        }

        if (input_)
            input_->PostUpdate();
    }

    void SubsystemManager::UpdateEditor(float deltaTime)
    {
        if (input_)
            input_->UpdateStatistics(deltaTime);
        
        if (inputManager_)
            inputManager_->Update();

        if (sceneManager_)
        {
            if (auto* scene = sceneManager_->GetScene())
                scene->OnEditorUpdate(deltaTime);
        }

        if (input_)
            input_->PostUpdate();
    }

    void SubsystemManager::UpdatePaused(float deltaTime)
    {
        if (input_)
            input_->UpdateStatistics(deltaTime);
        
        if (inputManager_)
            inputManager_->Update();
        
        if (input_)
            input_->PostUpdate();
    }

    bool SubsystemManager::ShouldQuit() const noexcept
    {
        return input_ && (input_->IsQuitRequested() || input_->IsKeyDown(SDLK_ESCAPE));
    }

    Game::Scene* SubsystemManager::GetActiveScene() noexcept
    {
        return sceneManager_ ? sceneManager_->GetScene() : nullptr;
    }

    const Game::Scene* SubsystemManager::GetActiveScene() const noexcept
    {
        return sceneManager_ ? sceneManager_->GetScene() : nullptr;
    }
}
