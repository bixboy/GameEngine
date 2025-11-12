#pragma once
#include <memory>
#include <type_traits>
#include "Systems/ApplicationConfig.h"
#include "Bootstrap/EngineBootstrap.h"


namespace BixEngine::Game
{
    class Scene;
}

namespace BixEngine::Core
{
    class Application
    {
    public:
        // config de l'application
        using Config = ApplicationConfig;

        explicit Application(Config config = {});
        ~Application();

        Application(const Application&) = delete; // constructeur de copie
        Application& operator=(const Application&) = delete; // opérateur d'affectation par copie

        Application(Application&&) noexcept = delete; // constructeur de move
        Application& operator=(Application&&) noexcept = delete; // opérateur d'affectation par move


        /**
         * Appelle l'initialisation des sous-systèmes via EngineBootstrap si nécessaire.
         * Idempotent si EngineBootstrap::InitializeAll() est idempotent.
         * @return true si l'engine est prêt (IsReady()).
         */
        bool Initialize();


        /**
         * Exécute la boucle principale (Tick) jusqu'à ce que IsRunning() devienne false.
         * Assure un ShutdownAll() à la sortie.
         */
        void Run();


        /**
         * Arrêt explicite.
         * Idempotent si ShutdownAll() est idempotent.
         */
        void Shutdown();


        /**
         * Construit/active une scène TScene via le bootstrap.
         * Crée et initialise le bootstrap en lazy si besoin.
         * @tparam TScene Doit dériver de Game::Scene.
         * @return Référence sur la scène créée.
         */
        template <typename TScene, typename... Args>
        TScene& EmplaceScene(Args&&... args);

    private:
        bool EnsureInitialized();

        Config config_{};

        std::unique_ptr<EngineBootstrap> bootstrap_{};
    };
}

#include "Game/Scene.h"

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
