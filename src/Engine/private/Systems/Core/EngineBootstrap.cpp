#include "Systems/Core/EngineBootstrap.h"
#include "Debug/Logger.h"
#include "Entities/Player.h"
#include "Containers/String.h"
#include "Systems/Core/Window.h"
#include "Components/Core/ComponentRegistry.h"
#include "Renderer.h"
#include "Ressources/Core/ResourceManager.h"
#include "Ressources/Loaders/ResourceLoaders.h"
#include "Gui/Core/EditorPreferences.h"
#include "Serializer/SceneSerializer.h"
#include "Framework/SceneManager.h"

namespace
{
    constexpr auto kDefaultAppName = "BixEngine";
    constexpr auto kDefaultAppId = "com.Bixboy.BixEngine";
    constexpr auto kDefaultAppVersion = "1.0";
}

namespace BixEngine::Core
{
    EngineBootstrap::EngineBootstrap(ApplicationConfig config) : config_(std::move(config)), eventDispatcher_(nullptr, nullptr) 
    {
    }

    EngineBootstrap::~EngineBootstrap()
    {
        if (initialized_)
            ShutdownAll();
    }

    void ForceLinkPlayer()
    {
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

        // 1. SDL System
        LOG_INFO("Initializing SDL system...");
        if (!sdlSystem_.Initialize(kDefaultAppName, kDefaultAppId, kDefaultAppVersion))
        {
            LOG_ERROR("SDL system initialization failed.");
            return false;
        }

        // 2. Window
        LOG_INFO("Creating main window...");
        if (!CreateWindow())
        {
            LOG_ERROR("Failed to create SDL window.");
            ShutdownAll();
            return false;
        }

        // 3. Renderer (Dépend de Window)
        LOG_INFO("Creating renderer...");
        if (!CreateRenderer())
        {
            LOG_ERROR("Failed to create renderer.");
            ShutdownAll();
            return false;
        }

        // 4. Resources
        LOG_INFO("Configuring resource loaders...");
        Resources::RegisterAllResourceLoaders(renderer_->GetSDLRenderer());

        // 5. GUI (Dépend de Window/Renderer)
        LOG_INFO("Initializing GUI module...");
        if (!guiModule_.Initialize(*window_, *renderer_))
        {
            LOG_ERROR("GUI module initialization failed.");
            ShutdownAll();
            return false;
        }

        // 6. Subsystems (Dépend de Renderer/Window/Gui)
        LOG_INFO("Initializing SubsystemManager...");
        if (!subsystems_.Initialize(*renderer_, *window_, guiModule_.GetGuiManager()))
        {
            LOG_ERROR("SubsystemManager initialization failed.");
            ShutdownAll();
            return false;
        }

        // 7. Audio
        LOG_INFO("Initializing AudioSystem...");
        if (!audioSystem_.Initialize())
        {
            LOG_ERROR("AudioSystem initialization failed.");
            ShutdownAll();
            return false;
        }

        // 8. Event Dispatcher & Loop
        LOG_INFO("Configuring event dispatcher and render loop...");
        
        eventDispatcher_.SetDependencies(&guiModule_, &subsystems_);
        eventDispatcher_.SetMouseUpdateRate(editorSettings_.MouseEventRateLimit);
        
        renderLoop_.Configure(&subsystems_, &guiModule_, renderer_.get(), config_.clearColor);

        // 9. Chargement Map par défaut
        const auto& defaultMap = Gui::EditorSettings::Get().DefaultMapPath;
        LOG_INFO("Bootstrap: Checking default map path: " + (defaultMap.empty() ? "EMPTY" : defaultMap));

        if (!defaultMap.empty() && std::filesystem::exists(defaultMap))
        {
            LOG_INFO("Loading default map: " + defaultMap);
            
            auto newScene = std::make_unique<Game::Scene>("DefaultScene");
            
            if (Serialization::SceneSerializer::LoadBinary(*newScene, defaultMap))
            {
                subsystems_.GetSceneManager()->SetScene(std::move(newScene));
                LOG_INFO("Default map loaded successfully.");
            }
            else
            {
                LOG_ERROR("Failed to load default map: " + defaultMap);
            }
        }
        else 
        {
            // Optionnel : Créer une scène vide par défaut si aucune map n'est chargée
            // subsystems_.EmplaceScene<Game::Scene>("Empty Scene");
        }

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
        
        eventDispatcher_.PumpEvents(running_);
        
        if (!running_)
        {
            LOG_INFO("Quit event received. Stopping engine loop.");
            return;
        }

        renderLoop_.BeginFrame();
        renderLoop_.Update(deltaTime);
        renderLoop_.Render();

        if (enableFrameLimiter_ && targetFrameRate_ > 0.0f)
        {
            if (Timer* timer = subsystems_.GetTimer())
            {
                const float targetDelta = 1.0f / targetFrameRate_;
                timer->SleepUntilNextFrame(targetDelta);
            }
        }
    }

    void EngineBootstrap::ShutdownAll() noexcept
    {
        if (!initialized_)
            return;

        LOG_INFO("Shutting down EngineBootstrap...");

        running_ = false;

        renderLoop_.Reset();
        
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

    bool EngineBootstrap::HasActiveScene() const
    {
        return subsystems_.GetActiveScene() != nullptr;
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
        renderer_ = std::make_unique<Graphics::Renderer>(window_->GetSDLWindow(), nullptr, config_.useVSync);

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