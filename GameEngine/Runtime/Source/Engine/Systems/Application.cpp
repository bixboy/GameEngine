#include "Engine/Systems/Application.h"

#include <utility>

namespace BixEngine::Core
{
    Application::Application(Config config)
        : config_(std::move(config))
        , bootstrap_(std::make_unique<EngineBootstrap>(config_))
    {
    }

    Application::~Application() = default;

    bool Application::Initialize()
    {
        if (!bootstrap_)
            bootstrap_ = std::make_unique<EngineBootstrap>(config_);

        if (!bootstrap_->InitializeAll())
            return false;

        return bootstrap_->IsReady();
    }

    void Application::Run()
    {
        if (!bootstrap_)
            bootstrap_ = std::make_unique<EngineBootstrap>(config_);

        if (!bootstrap_->IsReady() && !bootstrap_->InitializeAll())
            return;

        while (bootstrap_->IsRunning())
            bootstrap_->Tick();

        bootstrap_->ShutdownAll();
    }

    void Application::Shutdown()
    {
        if (bootstrap_)
            bootstrap_->ShutdownAll();
    }
}
