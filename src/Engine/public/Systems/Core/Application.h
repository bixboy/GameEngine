#pragma once
#include <memory>
#include <type_traits>
#include "Systems/Core/ApplicationConfig.h"
#include "Systems/Core/EngineBootstrap.h"


namespace BixEngine::Game
{
    class Scene;
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

         
        bool HasActiveScene() const;



         
        void Run();


         
        void Shutdown();


         
        template <typename TScene, typename... Args>
        TScene& EmplaceScene(Args&&... args);

    private:
        bool EnsureInitialized();

        Config config_{};

        std::unique_ptr<EngineBootstrap> bootstrap_{};
    };
}

#include "Framework/Scene.h"

namespace BixEngine::Core
{
    template <typename TScene, typename... Args>
    TScene& Application::EmplaceScene(Args&&... args)
    {
        static_assert(std::is_base_of_v<Game::Scene, TScene>, "TScene must derive from BixEngine::Game::Scene");

        if (!EnsureInitialized())
            throw std::runtime_error("Engine initialization failed before EmplaceScene");

        return bootstrap_->EmplaceScene<TScene>(std::forward<Args>(args)...);
    }
}
