#pragma once
#include <memory>
#include <utility>
#include "Systems/Core/ApplicationConfig.h"
#include "Systems/Core/SdlSystem.h"
#include "Gui/Internal/GuiModule.h"
#include "Systems/Core/SubsystemManager.h"
#include "Editor/EditorSettings.h"
#include "Events/EventDispatcher.h"
#include "Render/Core/RenderLoop.h"
#include "Systems/Audio/AudioSystem.h"


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


        
        bool Restart();

        
        bool HasActiveScene() const;

        
        template <typename TScene, typename... Args>
        TScene& EmplaceScene(Args&&... args)
        {
            return subsystems_.EmplaceScene<TScene>(std::forward<Args>(args)...);
        }

        [[nodiscard]] Graphics::Renderer* GetRenderer() const noexcept { return renderer_.get(); }
        [[nodiscard]] EditorSettings& GetEditorSettings() noexcept { return editorSettings_; }
        [[nodiscard]] const EditorSettings& GetEditorSettings() const noexcept { return editorSettings_; }

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

        EditorSettings editorSettings_{};

        EventDispatcher eventDispatcher_{};

        RenderLoop renderLoop_{};

        Systems::AudioSystem& audioSystem_ = Systems::AudioSystem::Get();
    };
}
