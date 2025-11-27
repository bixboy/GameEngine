
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
        return EnsureInitialized() && bootstrap_->IsReady();
    }

    void Application::Run()
    {
        if (!EnsureInitialized())
            return;

        while (bootstrap_->IsRunning())
        {
            // LOG_INFO("Application: Tick start"); // Commented out to avoid spam, uncomment if needed
            bootstrap_->Tick();
            // LOG_INFO("Application: Tick end");
        }

        bootstrap_->ShutdownAll();
    }

    void Application::Shutdown()
    {
        if (bootstrap_)
            bootstrap_->ShutdownAll();
    }

    bool Application::EnsureInitialized()
    {
        if (!bootstrap_)
            bootstrap_ = std::make_unique<EngineBootstrap>(config_);

        if (!bootstrap_->IsReady())
            return bootstrap_->InitializeAll();

        return true;
    }
}
