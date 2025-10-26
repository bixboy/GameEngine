#include "Engine/Bootstrap/EngineBootstrap.h"

#include <SDL3/SDL.h>

#include "Core/Logger.h"
#include "Core/Containers/String.h"
#include "Engine/Systems/Window.h"
#include "Graphics/Renderer.h"

namespace
{
    constexpr const char* kDefaultAppName = "BixEngine";
    constexpr const char* kDefaultAppId = "com.Bixboy.CustomEngine";
    constexpr const char* kDefaultAppVersion = "1.0";
}

namespace BixEngine::Core
{
    EngineBootstrap::EngineBootstrap(ApplicationConfig config) : config_(std::move(config))
    {
    }

    EngineBootstrap::~EngineBootstrap()
    {
        ShutdownAll();
    }

    bool EngineBootstrap::InitializeAll()
    {
        if (initialized_)
            return true;

        if (!sdlSystem_.Initialize(kDefaultAppName, kDefaultAppId, kDefaultAppVersion))
            return false;

        if (!CreateWindow())
        {
            ShutdownAll();
            return false;
        }

        if (!CreateRenderer())
        {
            ShutdownAll();
            return false;
        }

        if (!guiModule_.Initialize(*window_, *renderer_))
        {
            ShutdownAll();
            return false;
        }

        if (!subsystems_.Initialize(*renderer_, *window_, guiModule_.GetGuiManager()))
        {
            ShutdownAll();
            return false;
        }

        eventDispatcher_.Configure(&guiModule_, &subsystems_);
        renderLoop_.Configure(&subsystems_, &guiModule_, renderer_.get(), config_.clearColor);
        guiModule_.SetupDefaultGuiPanels(subsystems_, renderLoop_.GetLastDeltaTimePointer());

        initialized_ = true;
        running_ = true;
        return true;
    }

    bool EngineBootstrap::IsReady() const noexcept
    {
        return initialized_;
    }

    bool EngineBootstrap::IsRunning() const noexcept
    {
        return running_;
    }

    void EngineBootstrap::Tick()
    {
        if (!initialized_ || !running_)
            return;

        const float deltaTime = renderLoop_.CalculateDeltaTime();
        eventDispatcher_.PumpEvents(running_);

        if (!running_)
            return;

        renderLoop_.BeginFrame();
        renderLoop_.Update(deltaTime);
        renderLoop_.Render();
    }

    void EngineBootstrap::ShutdownAll() noexcept
    {
        running_ = false;
        renderLoop_.Reset();
        eventDispatcher_.Reset();
        subsystems_.Shutdown();
        guiModule_.Shutdown();
        renderer_.reset();
        window_.reset();
        sdlSystem_.Shutdown();
        initialized_ = false;
    }

    bool EngineBootstrap::CreateWindow()
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

    bool EngineBootstrap::CreateRenderer()
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

}
