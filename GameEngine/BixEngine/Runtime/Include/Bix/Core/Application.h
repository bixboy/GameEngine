#pragma once

#include <memory>
#include <type_traits>

#include "Bix/Core/ApplicationConfig.h"
#include "Bix/Core/EngineBootstrap.h"

namespace BixEngine
{
    namespace Game { class Scene; }
}

namespace BixEngine::Core
{
    class Application
    {
    public:
        using Config = ApplicationConfig;

        explicit Application(Config config = {});
        ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        Application(Application&&) noexcept = delete;
        Application& operator=(Application&&) noexcept = delete;

        bool Initialize();
        void Run();
        void Shutdown();

        template <typename TScene, typename... Args>
        TScene& EmplaceScene(Args&&... args);

    private:
        Config config_{};
        std::unique_ptr<EngineBootstrap> bootstrap_{};
    };
}

#include "Bix/Game/Scene.h"

namespace BixEngine::Core
{
    template <typename TScene, typename... Args>
    TScene& Application::EmplaceScene(Args&&... args)
    {
        static_assert(std::is_base_of_v<Game::Scene, TScene>, "TScene must derive from BixEngine::Game::Scene");
        if (!bootstrap_)
        {
            bootstrap_ = std::make_unique<EngineBootstrap>(config_);
            bootstrap_->InitializeAll();
        }
        return bootstrap_->EmplaceScene<TScene>(std::forward<Args>(args)...);
    }
}
