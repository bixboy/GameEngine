#pragma once

#include <memory>
#include <utility>

#include "Bix/Core/ApplicationConfig.h"
#include "Bix/Core/SdlSystem.h"
#include "Bix/Core/GuiModule.h"
#include "Bix/Core/SubsystemManager.h"
#include "Bix/Core/EventDispatcher.h"
#include "Bix/Core/RenderLoop.h"

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

        EngineBootstrap(const EngineBootstrap&) = delete;
        EngineBootstrap& operator=(const EngineBootstrap&) = delete;
        EngineBootstrap(EngineBootstrap&&) noexcept = delete;
        EngineBootstrap& operator=(EngineBootstrap&&) noexcept = delete;

        bool InitializeAll();
        bool IsReady() const noexcept;
        bool IsRunning() const noexcept;

        void Tick();
        void ShutdownAll() noexcept;

        template <typename TScene, typename... Args>
        TScene& EmplaceScene(Args&&... args)
        {
            return subsystems_.EmplaceScene<TScene>(std::forward<Args>(args)...);
        }

    private:
        bool CreateWindow();
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
