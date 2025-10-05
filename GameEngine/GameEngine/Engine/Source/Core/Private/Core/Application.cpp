#include "Core/Application.h"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <memory>
#include <utility>

#include "Core/Logger.h"
#include "Core/Timer.h"
#include "Core/Window.h"
#include "Game/Actor.h"
#include "Game/Scene.h"
#include "Game/SceneManager.h"
#include "Graphics/Renderer.h"
#include "Input/Input.h"
#include "Input/InputManager.h"

#include "Gui/GuiManager.h"
#include "Gui/GuiPanel.h"
#include "Gui/GuiSystem.h"
#include "Gui/DefaultEngineGui.h"

namespace Engine::Core
{
    namespace
    {
        constexpr const char* kDefaultAppName = "Bixboy Custom Engine";
        constexpr const char* kDefaultAppId = "com.Bixboy.CustomEngine";
        constexpr const char* kDefaultAppVersion = "1.0";
    }

    Application::Application(Config config) : config_(std::move(config)) {}

    Application::~Application()
    {
        Shutdown();
    }

    bool Application::Initialize()
    {
        if (running_)
            return true;

        SDL_SetAppMetadata(kDefaultAppName, kDefaultAppVersion, kDefaultAppId);

        if (!InitializeSDL())
            return false;

        if (!CreateWindow() || !CreateRenderer() || !InitializeGui())
        {
            Shutdown();
            return false;
        }

        InitializeSubsystems();

        running_ = true;
        return true;
    }

    void Application::Run()
    {
        if (!running_)
            return;

        while (running_)
        {
            if (timer_)
            {
                timer_->Tick();
                lastDeltaTime_ = timer_->GetDeltaTime();
            }
            else
            {
                lastDeltaTime_ = 0.0f;
            }

            ProcessEvents();
            BeginFrame();
            Update(lastDeltaTime_);
            Render();
        }
    }

    void Application::ProcessEvents()
    {
        SDL_Event event{};
        while (SDL_PollEvent(&event))
        {
            if (guiSystem_ && guiSystem_->IsInitialized())
                guiSystem_->ProcessEvent(event);

            input_->ProcessEvent(event);

            if (inputManager_)
                inputManager_->ProcessEvent(event);

            if (sceneManager_)
            {
                if (Game::Scene* scene = sceneManager_->GetScene())
                    scene->HandleEvent(event);
            }

            if (event.type == SDL_EVENT_QUIT)
                running_ = false;
        }

        if (input_->IsQuitRequested() || input_->IsKeyDown(SDLK_ESCAPE))
            running_ = false;
    }

    void Application::BeginFrame()
    {
        if (!guiSystem_ || !guiSystem_->IsInitialized())
            return;

        guiSystem_->BeginFrame();
    }

    void Application::Update(float deltaTime)
    {
        if (inputManager_)
            inputManager_->Update();

        if (sceneManager_)
        {
            if (auto* scene = sceneManager_->GetScene())
                scene->Update(deltaTime);
        }
    }

    void Application::Render()
    {
        if (!renderer_)
            return;

        renderer_->Clear(config_.clearColor);

        Game::Scene* activeScene = sceneManager_ ? sceneManager_->GetScene() : nullptr;
        if (activeScene)
            activeScene->Render(*renderer_);

        if (timer_)
            SDL_RenderDebugTextFormat(renderer_->GetSDLRenderer(), 10, 10, "FPS: %.0f", timer_->GetFPS());

        if (activeScene)
        {
            const String& sceneName = activeScene->Name();
            SDL_RenderDebugTextFormat(
                renderer_->GetSDLRenderer(),
                10,
                30,
                "Scene: %.*s",
                static_cast<int>(sceneName.size()),
                sceneName.c_str());
        }

        RenderGui(activeScene);
        renderer_->Present();
    }

    void Application::RenderGui(Game::Scene* activeScene)
    {
        (void)activeScene;

        if (!guiSystem_ || !guiSystem_->IsInitialized())
            return;

        if (guiManager_)
            guiManager_->DrawAll();

        guiSystem_->EndFrame();
        guiSystem_->Render();
    }

// === SDL Init ====    
#pragma region SDL Init
    
    bool Application::InitializeSDL()
    {
        if (sdlInitialized_)
            return true;

        SDL_SetAppMetadata(
            config_.windowTitle.c_str(),
            kDefaultAppVersion,
            kDefaultAppId
        );

        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            LOG_ERROR(String{"Couldn't initialize SDL: "} + SDL_GetError());
            return false;
        }

        sdlInitialized_ = true;
        return true;
    }

    bool Application::CreateWindow()
    {
        window_ = std::make_unique<Window>(config_.windowTitle, config_.width, config_.height, config_.resizable);
        if (!window_ || !window_->IsValid())
        {
            LOG_ERROR(String{"Couldn't create window: "} + SDL_GetError());
            window_.reset();
            return false;
        }
        
        return true;
    }

    bool Application::CreateRenderer()
    {
        renderer_ = std::make_unique<Graphics::Renderer>(window_->GetSDLWindow());
        
        if (!renderer_ || !renderer_->IsValid())
        {
            LOG_ERROR(String{"Couldn't create renderer: "} + SDL_GetError());
            renderer_.reset();
            window_.reset();
            return false;
        }

        return true;
    }

#pragma endregion   

    bool Application::InitializeGui()
    {
        if (guiSystem_ && guiSystem_->IsInitialized())
            return true;

        if (!window_ || !renderer_)
            return false;

        if (!guiSystem_)
            guiSystem_ = std::make_unique<Gui::GuiSystem>();

        if (!guiSystem_->Initialize(window_->GetSDLWindow(), renderer_->GetSDLRenderer()))
        {
            guiSystem_.reset();
            return false;
        }

        return true;
    }

    void Application::InitializeSubsystems()
    {
        timer_ = std::make_unique<Timer>();
        input_ = std::make_unique<Input::Input>();
        inputManager_ = std::make_unique<Input::InputManager>();

        if (inputManager_)
            inputManager_->SetInputDevice(input_.get());

        if (guiSystem_ && guiSystem_->IsInitialized())
        {
            guiManager_ = std::make_unique<Gui::GuiManager>(*guiSystem_);
            SetupDefaultGuiPanels();
        }
        else
        {
            guiManager_.reset();
        }

        sceneManager_ = std::make_unique<Game::SceneManager>();

        if (sceneManager_)
        {
            sceneManager_->SetContext({
                renderer_.get(),
                inputManager_.get(),
                window_.get(),
                timer_.get(),
                guiManager_.get()
            });
        }
    }

    void Application::SetupDefaultGuiPanels()
    {
        if (!guiManager_)
        {
            statsPanel_ = nullptr;
            outlinerPanel_ = nullptr;
            contentBrowserPanel_ = nullptr;
            inspectorPanel_ = nullptr;
            selectedActor_ = nullptr;
            return;
        }

        selectedActor_ = nullptr;

        const Gui::DefaultEngineGuiContext context{
            timer_.get(),
            [this]() -> Game::SceneManager*
            {
                return sceneManager_.get();
            },
            &lastDeltaTime_,
            [this]() -> Game::Actor*
            {
                return selectedActor_;
            },
            [this](Game::Actor* actor)
            {
                selectedActor_ = actor;
            }
        };

        const Gui::DefaultEngineGuiPanels panels = Gui::CreateDefaultEngineGui(*guiManager_, context);
        statsPanel_ = panels.statsPanel;
        outlinerPanel_ = panels.sceneOutlinerPanel;
        contentBrowserPanel_ = panels.contentBrowserPanel;
        inspectorPanel_ = panels.actorInspectorPanel;
    }

// === Shutdown ===
#pragma region Shutdown

    void Application::Shutdown()
    {
        running_ = false;

        ShutdownSubsystems();
        ShutdownSDL();
    }
    
    void Application::ShutdownSubsystems() noexcept
    {
        if (sceneManager_)
            sceneManager_->SetScene(nullptr);

        sceneManager_.reset();
        inputManager_.reset();
        input_.reset();
        timer_.reset();
        ShutdownGui();
        renderer_.reset();
        window_.reset();
    }

    void Application::ShutdownSDL() noexcept
    {
        if (sdlInitialized_)
        {
            SDL_Quit();
            sdlInitialized_ = false;
        }
    }

    void Application::ShutdownGui() noexcept
    {
        statsPanel_ = nullptr;
        outlinerPanel_ = nullptr;
        contentBrowserPanel_ = nullptr;
        inspectorPanel_ = nullptr;
        selectedActor_ = nullptr;

        if (guiManager_)
            guiManager_.reset();

        if (guiSystem_)
        {
            guiSystem_->Shutdown();
            guiSystem_.reset();
        }
    }

#pragma endregion
    
}
