#pragma once
#include <memory>
#include <type_traits>
#include <stdexcept>
#include "Systems/Core/ApplicationConfig.h"
#include "Systems/Core/EngineBootstrap.h"
#include "Framework/Scene.h"


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
         
        // Charger/Creer une scène au démarrage
        template <typename TScene, typename... Args>
        TScene& EmplaceScene(Args&&... args)
        {
            static_assert(std::is_base_of_v<Game::Scene, TScene>, "TScene must derive from BixEngine::Game::Scene");

            if (!Initialize())
            {
                throw std::runtime_error("Engine initialization failed inside EmplaceScene");
            }

            return bootstrap_->EmplaceScene<TScene>(std::forward<Args>(args)...);
        }

    private:
        Config config_{};
        
        std::unique_ptr<EngineBootstrap> bootstrap_{};
    };
}