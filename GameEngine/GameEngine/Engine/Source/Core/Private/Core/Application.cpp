#include "Core/Application.h"

#include <SDL3/SDL.h>

#include <string>
#include <string_view>
#include <utility>

#include "Core/Logger.h"
#include "Core/Timer.h"
#include "Core/Window.h"
#include "Game/Scene.h"
#include "Game/SceneManager.h"
#include "Graphics/Renderer.h"
#include "Input/Input.h"
#include "Input/InputManager.h"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"

namespace Engine::Core
{
    namespace
    {
        constexpr const char* kDefaultAppName = "Example Custom Engine";
        constexpr const char* kDefaultAppId = "com.example.CustomEngine";
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

        if (!CreateWindow() || !CreateRenderer() || !InitializeImGui())
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

    void Application::Shutdown()
    {
        running_ = false;

        ShutdownSubsystems();
        ShutdownSDL();
    }

    void Application::ProcessEvents()
    {
        SDL_Event event{};
        while (SDL_PollEvent(&event))
        {
            if (imguiInitialized_)
                ImGui_ImplSDL3_ProcessEvent(&event);

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
        if (!imguiInitialized_)
            return;

        if (!ImGui::GetCurrentContext())
        {
            LOG_WARNING("ImGui context not available; disabling ImGui frame generation.");
            imguiInitialized_ = false;
            return;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
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
            const std::string_view sceneName = activeScene->Name();
            SDL_RenderDebugTextFormat(
                renderer_->GetSDLRenderer(),
                10,
                30,
                "Scene: %.*s",
                static_cast<int>(sceneName.size()),
                sceneName.data());
        }

        RenderGui(activeScene);
        renderer_->Present();
    }

    void Application::RenderGui(Game::Scene* activeScene)
    {
        if (!imguiInitialized_)
            return;

        if (!ImGui::GetCurrentContext())
        {
            LOG_WARNING("ImGui context not available; skipping ImGui rendering.");
            imguiInitialized_ = false;
            return;
        }

        ImGui::Begin("Engine Stats");
        ImGui::Text("FPS: %.1f", timer_ ? timer_->GetFPS() : 0.0f);
        ImGui::Text("Delta Time: %.3f ms", lastDeltaTime_ * 1000.0f);

        if (activeScene)
        {
            const std::string_view sceneName = activeScene->Name();
            ImGui::Separator();
            ImGui::Text("Scene: %.*s", static_cast<int>(sceneName.size()), sceneName.data());
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer_->GetSDLRenderer());
    }

    bool Application::InitializeSDL()
    {
        if (sdlInitialized_)
            return true;

        SDL_SetAppMetadata(
            config_.windowTitle.c_str(),
            kDefaultAppVersion,
            kDefaultAppId
        );

        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
            LOG_ERROR(std::string{"Couldn't initialize SDL: "} + SDL_GetError());
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
            LOG_ERROR(std::string{"Couldn't create window: "} + SDL_GetError());
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
            LOG_ERROR(std::string{"Couldn't create renderer: "} + SDL_GetError());
            renderer_.reset();
            window_.reset();
            return false;
        }

        return true;
    }

    bool Application::InitializeImGui()
    {
        if (imguiInitialized_)
            return true;

        if (!window_ || !renderer_)
            return false;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();

        if (!ImGui_ImplSDL3_InitForSDLRenderer(window_->GetSDLWindow(), renderer_->GetSDLRenderer()))
        {
            const char* error = SDL_GetError();
            if (error && *error)
                LOG_ERROR(std::string{"Failed to initialize ImGui SDL3 backend: "} + error);
            else
                LOG_ERROR("Failed to initialize ImGui SDL3 backend.");
            ImGui::DestroyContext();
            return false;
        }

        if (!ImGui_ImplSDLRenderer3_Init(renderer_->GetSDLRenderer()))
        {
            const char* error = SDL_GetError();
            if (error && *error)
                LOG_ERROR(std::string{"Failed to initialize ImGui SDL renderer backend: "} + error);
            else
                LOG_ERROR("Failed to initialize ImGui SDL renderer backend.");
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();
            return false;
        }

        imguiInitialized_ = true;
        return true;
    }

    void Application::InitializeSubsystems()
    {
        timer_ = std::make_unique<Timer>();
        input_ = std::make_unique<Input::Input>();
        inputManager_ = std::make_unique<Input::InputManager>();

        if (inputManager_)
            inputManager_->SetInputDevice(input_.get());

        sceneManager_ = std::make_unique<Game::SceneManager>();

        if (sceneManager_)
        {
            sceneManager_->SetContext({
                renderer_.get(),
                inputManager_.get(),
                window_.get(),
                timer_.get()
            });
        }
    }

    void Application::ShutdownSubsystems() noexcept
    {
        if (sceneManager_)
            sceneManager_->SetScene(nullptr);

        sceneManager_.reset();
        inputManager_.reset();
        input_.reset();
        timer_.reset();
        ShutdownImGui();
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

    void Application::ShutdownImGui() noexcept
    {
        if (!imguiInitialized_)
            return;

        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();

        if (ImGuiContext* context = ImGui::GetCurrentContext())
            ImGui::DestroyContext(context);

        imguiInitialized_ = false;
    }
}
