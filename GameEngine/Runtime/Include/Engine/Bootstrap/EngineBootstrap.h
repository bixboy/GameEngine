#pragma once

#include <memory>
#include <utility>

#include "Engine/Systems/ApplicationConfig.h"
#include "Engine/Systems/SdlSystem.h"
#include "Engine/Gui/Internal/GuiModule.h"
#include "Engine/Systems/SubsystemManager.h"
#include "Core/EventDispatcher.h"
#include "Engine/Render/RenderLoop.h"

namespace BixEngine
{
    namespace Graphics { class Renderer; }
    namespace Core { class Window; }
}

namespace BixEngine::Core
{
    class EngineBootstrap
    {
    public:
        explicit EngineBootstrap(ApplicationConfig config = {});
        ~EngineBootstrap();

        // Copie et déplacement interdits (un seul moteur actif à la fois).
        EngineBootstrap(const EngineBootstrap&) = delete;
        EngineBootstrap& operator=(const EngineBootstrap&) = delete;
        EngineBootstrap(EngineBootstrap&&) noexcept = delete;
        EngineBootstrap& operator=(EngineBootstrap&&) noexcept = delete;

        
        // Initialise tous les sous-systèmes (SDL, fenêtre, renderer, GUI, etc.).
        bool InitializeAll();

        
        // Indique si le moteur est prêt à tourner.
        bool IsReady() const noexcept;

        
        // Indique si la boucle principale est en cours d’exécution.
        bool IsRunning() const noexcept;

        
        // Exécute une frame complète (events → update → render).
        void Tick();


        // Ferme proprement tous les systèmes du moteur.
        void ShutdownAll() noexcept;


        // Redémarre entièrement le moteur (shutdown + init).
        bool Restart();


        // Crée et active une scène de type TScene.
        template <typename TScene, typename... Args>
        TScene& EmplaceScene(Args&&... args)
        {
            return subsystems_.EmplaceScene<TScene>(std::forward<Args>(args)...);
        }

        [[nodiscard]] Graphics::Renderer* GetRenderer() const noexcept { return renderer_.get(); }

    private:
        // Crée la fenêtre principale SDL.
        bool CreateWindow();


        // Crée le renderer associé à la fenêtre.
        bool CreateRenderer();

        
        ApplicationConfig config_{};
        bool initialized_{false};
        bool running_{false};

        SdlSystem sdlSystem_{};
        
        std::unique_ptr<Window> window_{};
        
        std::unique_ptr<Graphics::Renderer> renderer_{};
        
        GuiModule guiModule_{};
        
        SubsystemManager subsystems_{};
        
        EventDispatcher eventDispatcher_{};
        
        RenderLoop renderLoop_{};
    };
}
