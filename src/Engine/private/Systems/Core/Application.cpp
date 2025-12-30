#include "Systems/Core/Application.h"
#include "Debug/Logger.h"
#include <utility>

namespace BixEngine::Core
{
    Application::Application(Config config) : config_(std::move(config)), bootstrap_(std::make_unique<EngineBootstrap>(config_))
    {
    }

    Application::~Application()
    {
        Shutdown();
    }

    bool Application::Initialize()
    {
        if (bootstrap_->IsReady())
            return true;

        return bootstrap_->InitializeAll();
    }

    bool Application::HasActiveScene() const
    {
        return bootstrap_ && bootstrap_->HasActiveScene();
    }

    void Application::Run()
    {
        if (!Initialize())
        {
            LOG_ERROR("Application failed to initialize. Aborting Run.");
            return;
        }

        while (bootstrap_->IsRunning())
        {
            bootstrap_->Tick();
        }

        Shutdown();
    }

    void Application::Shutdown()
    {
        if (bootstrap_)
            bootstrap_->ShutdownAll();
    }
}