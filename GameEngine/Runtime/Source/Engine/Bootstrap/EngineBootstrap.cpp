#include "Engine/Bootstrap/EngineBootstrap.h"

#include "Core/Logger.h"
#include "Core/Containers/String.h"
#include "Engine/Systems/Window.h"
#include "Game/Components/ComponentRegistry.h"
#include "Graphics/Renderer.h"
#include "Engine/Ressources/ResourceManager.h"
#include "Engine/Ressources/SpriteAtlas.h"
#include "Engine/Ressources/Texture.h"

namespace
{
    constexpr const char* kDefaultAppName = "BixEngine";
    constexpr const char* kDefaultAppId = "com.Bixboy.CustomEngine";
    constexpr const char* kDefaultAppVersion = "1.0";
}

namespace BixEngine::Core
{
    EngineBootstrap::EngineBootstrap(ApplicationConfig config): config_(std::move(config))
    {
    }

    EngineBootstrap::~EngineBootstrap()
    {
        if (initialized_)
            ShutdownAll();
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
        auto& resourceManager = resources::ResourceManager::Get();

        resourceManager.RegisterLoader<resources::Texture>([](const String& path) -> std::shared_ptr<resources::Texture>
        {
            auto texture = std::make_shared<resources::Texture>();
            if (!texture->LoadFromFile(path))
                return nullptr;
            
            return texture;
        });

        resourceManager.RegisterLoader<resources::SpriteAtlas>([](const String& path) -> std::shared_ptr<resources::SpriteAtlas>
        {
            auto atlas = std::make_shared<resources::SpriteAtlas>();
            if (!atlas->LoadFromFile(path))
                return nullptr;
            
            return atlas;
        });

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

        // --- Event dispatcher + render loop ---
        LOG_INFO("Configuring event dispatcher and render loop...");
        eventDispatcher_.Configure(&guiModule_, &subsystems_);
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
