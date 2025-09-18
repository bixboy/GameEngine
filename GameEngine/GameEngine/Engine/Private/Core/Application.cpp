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

namespace Engine::Core {

namespace {

constexpr const char* kDefaultAppName = "Example Custom Engine";
constexpr const char* kDefaultAppId = "com.example.CustomEngine";
constexpr const char* kDefaultAppVersion = "1.0";

}  // namespace

Application::Application(Config config) : config_(std::move(config)) {}

Application::~Application() {
    Shutdown();
}

bool Application::Initialize() {
    if (running_) {
        return true;
    }

    SDL_SetAppMetadata(kDefaultAppName, kDefaultAppVersion, kDefaultAppId);

    if (!InitializeSDL()) {
        return false;
    }

    if (!CreateWindow()) {
        ShutdownSDL();
        return false;
    }

    if (!CreateRenderer()) {
        ShutdownSDL();
        return false;
    }

    InitializeSubsystems();

    running_ = true;
    return true;
}

void Application::Run() {
    if (!running_) {
        return;
    }

    while (running_) {
        timer_->Tick();
        const float dt = timer_->GetDeltaTime();

        ProcessEvents();
        Update(dt);
        Render();
    }
}

void Application::Shutdown() {
    running_ = false;

    ShutdownSubsystems();
    ShutdownSDL();
}

void Application::ProcessEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        input_->ProcessEvent(event);
        if (sceneManager_) {
            if (auto* scene = sceneManager_->GetScene()) {
                scene->HandleEvent(event);
            }
        }
    }

    if (input_->IsQuitRequested() || input_->IsKeyDown(SDLK_ESCAPE)) {
        running_ = false;
    }
}

void Application::Update(float deltaTime) {
    if (sceneManager_) {
        if (auto* scene = sceneManager_->GetScene()) {
            scene->Update(deltaTime);
        }
    }
}

void Application::Render() {
    if (!renderer_) {
        return;
    }

    renderer_->Clear(config_.clearColor);

    const Game::Scene* activeScene = nullptr;
    if (sceneManager_) {
        activeScene = sceneManager_->GetScene();
        if (activeScene) {
            activeScene->Render(*renderer_);
        }
    }

    if (timer_) {
        SDL_RenderDebugTextFormat(renderer_->GetSDLRenderer(), 10, 10, "FPS: %.0f", timer_->GetFPS());
    }

    if (activeScene) {
        const std::string_view sceneName = activeScene->Name();
        SDL_RenderDebugTextFormat(renderer_->GetSDLRenderer(), 10, 30, "Scene: %.*s",
                                  static_cast<int>(sceneName.size()), sceneName.data());
    }

    renderer_->Present();
}

bool Application::InitializeSDL() {
    if (sdlInitialized_) {
        return true;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        LOG_ERROR(std::string{"Couldn't initialize SDL: "} + SDL_GetError());
        return false;
    }

    sdlInitialized_ = true;
    return true;
}

bool Application::CreateWindow() {
    window_ = std::make_unique<Window>(config_.windowTitle, config_.width, config_.height, config_.resizable);
    if (!window_ || !window_->IsValid()) {
        LOG_ERROR(std::string{"Couldn't create window: "} + SDL_GetError());
        window_.reset();
        return false;
    }

    return true;
}

bool Application::CreateRenderer() {
    renderer_ = std::make_unique<Graphics::Renderer>(window_->GetSDLWindow());
    if (!renderer_ || !renderer_->IsValid()) {
        LOG_ERROR(std::string{"Couldn't create renderer: "} + SDL_GetError());
        renderer_.reset();
        window_.reset();
        return false;
    }

    return true;
}

void Application::InitializeSubsystems() {
    timer_ = std::make_unique<Timer>();
    input_ = std::make_unique<Input::Input>();
    sceneManager_ = std::make_unique<Game::SceneManager>();

    if (sceneManager_) {
        sceneManager_->SetContext({renderer_.get(), input_.get(), window_.get(), timer_.get()});
    }
}

void Application::ShutdownSubsystems() noexcept {
    if (sceneManager_) {
        sceneManager_->SetScene(nullptr);
    }

    sceneManager_.reset();
    input_.reset();
    timer_.reset();
    renderer_.reset();
    window_.reset();
}

void Application::ShutdownSDL() noexcept {
    if (sdlInitialized_) {
        SDL_Quit();
        sdlInitialized_ = false;
    }
}

}  // namespace Engine::Core
