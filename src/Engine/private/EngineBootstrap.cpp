#include "EngineBootstrap.h"
#include "Logger.h"
#include "Player.h"
#include "Containers/String.h"
#include "Systems/Window.h"
#include "Components/ComponentRegistry.h"
#include "Renderer.h"
#include "Ressources/ResourceManager.h"
#include "Ressources/Loaders/ResourceLoaders.h"

namespace
{
    constexpr auto kDefaultAppName = "BixEngine";
    constexpr auto kDefaultAppId = "com.Bixboy.CustomEngine";
    constexpr auto kDefaultAppVersion = "1.0";
}

namespace BixEngine::Core
{
    EngineBootstrap::EngineBootstrap(ApplicationConfig config) : config_(std::move(config))
    {
    }

    EngineBootstrap::~EngineBootstrap()
    {
        if (initialized_)
            ShutdownAll();
    }

    void ForceLinkPlayer() {
        // On force le linker à voir la classe
        const auto& info = Game::Player::StaticClass(); 
        LOG_INFO("ForceLinkPlayer: Linked class " + info.Name);
    }

    bool EngineBootstrap::InitializeAll()
    {
        if (initialized_)
        {
            LOG_WARNING("EngineBootstrap already initialized.");
            return true;
        }

        LOG_INFO("=== Initializing EngineBootstrap ===");

        Game::RegisterBuiltinComponents();

        ForceLinkPlayer();

        // --- SDL ---
        LOG_INFO("Initializing SDL system...");
        if (!sdlSystem_.Initialize(kDefaultAppName, kDefaultAppId, kDefaultAppVersion))
        {
            LOG_ERROR("SDL system initialization failed.");
            return false;
        }

        // --- Window ---
        LOG_INFO("Creating main window...");
        if (!CreateWindow())
        {
            LOG_ERROR("Failed to create SDL window.");
            ShutdownAll();
            return false;
        }

        // --- Renderer ---
        LOG_INFO("Creating renderer...");
        if (!CreateRenderer())
        {
            LOG_ERROR("Failed to create renderer.");
            ShutdownAll();
            return false;
        }

        // --- Resource Manager ---
        LOG_INFO("Configuring resource loaders...");
        resources::RegisterAllResourceLoaders(renderer_->GetSDLRenderer());


        // --- GUI ---
        LOG_INFO("Initializing GUI module...");
        if (!guiModule_.Initialize(*window_, *renderer_))
        {
            LOG_ERROR("GUI module initialization failed.");
            ShutdownAll();
            return false;
        }

        // --- Subsystems ---
        LOG_INFO("Initializing SubsystemManager...");
        if (!subsystems_.Initialize(*renderer_, *window_, guiModule_.GetGuiManager()))
        {
            LOG_ERROR("SubsystemManager initialization failed.");
            ShutdownAll();
            return false;
        }

        // --- Audio ---
        LOG_INFO("Initializing AudioSystem...");
        if (!audioSystem_.Initialize())
        {
            LOG_ERROR("AudioSystem initialization failed.");
            ShutdownAll();
            return false;
        }

        // --- Event dispatcher + render loop ---
        LOG_INFO("Configuring event dispatcher and render loop...");
        eventDispatcher_.Configure(&guiModule_, &subsystems_);
        eventDispatcher_.SetMouseEventRateLimit(editorSettings_.MouseEventRateLimit);
        renderLoop_.Configure(&subsystems_, &guiModule_, renderer_.get(), config_.clearColor);

        // --- Default GUI panels (debug overlay, FPS counter, etc.) ---
        guiModule_.SetupDefaultGuiPanels(subsystems_, renderLoop_.GetLastDeltaTimePointer());

        initialized_ = true;
        running_ = true;

        LOG_INFO("EngineBootstrap initialized successfully.");
        LOG_INFO("=======================================");
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

        // Récupère les événements SDL (fermeture, clavier, etc.)
        eventDispatcher_.SetMouseEventRateLimit(editorSettings_.MouseEventRateLimit);
        eventDispatcher_.PumpEvents(running_);
        if (!running_)
        {
            LOG_INFO("Quit event received. Stopping engine loop.");
            return;
        }

        // --- Frame sequence ---
        renderLoop_.BeginFrame();
        renderLoop_.Update(deltaTime);
        renderLoop_.Render();
    }

    void EngineBootstrap::ShutdownAll() noexcept
    {
        if (!initialized_)
            return;

        LOG_INFO("Shutting down EngineBootstrap...");

        running_ = false;

        renderLoop_.Reset();
        eventDispatcher_.Reset();
        audioSystem_.Shutdown();
        subsystems_.Shutdown();
        guiModule_.Shutdown();

        if (renderer_)
        {
            LOG_INFO("Destroying renderer...");
            renderer_.reset();
        }

        if (window_)
        {
            LOG_INFO("Destroying window...");
            window_.reset();
        }

        sdlSystem_.Shutdown();
        initialized_ = false;

        LOG_INFO("EngineBootstrap shut down successfully.");
    }

    bool EngineBootstrap::Restart()
    {
        LOG_INFO("Restarting EngineBootstrap...");
        ShutdownAll();
        return InitializeAll();
    }

    bool EngineBootstrap::CreateWindow()
    {
        window_ = std::make_unique<Window>(
            config_.windowTitle,
            config_.width,
            config_.height,
            config_.resizable
        );

        if (!window_ || !window_->IsValid())
        {
            LOG_ERROR(String{"Couldn't create window: "} + SDL_GetError());
            window_.reset();

            return false;
        }

        LOG_INFO("Main window created successfully.");
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

        LOG_INFO("Renderer created successfully.");
        return true;
    }
}
