#pragma once
#include <memory>
#include <type_traits>
#include "Math/Color.h"
#include "Core/String.h"

#define SDL_MAIN_HANDLED

namespace Engine
{

    namespace Graphics { class Renderer; }
    namespace Gui      { class GuiSystem; class GuiManager; class GuiPanel; }
    namespace Game     { class Scene; class SceneManager; class Actor; }
    namespace Input    { class InputManager; class Input; }
    namespace Core     { class Window; class Timer; }

    namespace Core
    {

        class Application
        {
            public:
                struct Config
                {
                    String windowTitle{"Custom Engine"};
                    int width{1280};
                    int height{720};
                    bool resizable{true};
                    Math::Color clearColor{0, 0, 0, 255};
                };

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
                bool InitializeSDL();
                bool CreateWindow();
                bool CreateRenderer();
                bool InitializeGui();
                void InitializeSubsystems();
                void ShutdownSubsystems() noexcept;
                void ShutdownSDL() noexcept;
                void ShutdownGui() noexcept;
                void ProcessEvents();
                void BeginFrame();
                void Update(float deltaTime);
                void Render();
                void RenderGui(Game::Scene* activeScene);
                void SetupDefaultGuiPanels();

                Config config_{};
                bool running_{false};
                bool sdlInitialized_{false};
                float lastDeltaTime_{0.0f};

                std::unique_ptr<Window> window_;
                std::unique_ptr<Graphics::Renderer> renderer_;
                std::unique_ptr<Game::SceneManager> sceneManager_;
                std::unique_ptr<Input::Input> input_;
                std::unique_ptr<Input::InputManager> inputManager_;
                std::unique_ptr<Gui::GuiSystem> guiSystem_;
                std::unique_ptr<Gui::GuiManager> guiManager_;
                Gui::GuiPanel* statsPanel_{nullptr};
                Gui::GuiPanel* outlinerPanel_{nullptr};
                Gui::GuiPanel* contentBrowserPanel_{nullptr};
                Gui::GuiPanel* inspectorPanel_{nullptr};
                Game::Actor* selectedActor_{nullptr};
                std::unique_ptr<Timer> timer_;
        };
    }
}

#include "Game/SceneManager.h"
#include "Game/Scene.h"
#include "Input/Input.h"

namespace Engine::Core
{
    template <typename TScene, typename... Args>
    TScene& Application::EmplaceScene(Args&&... args)
    {
        static_assert(std::is_base_of_v<Game::Scene, TScene>, "TScene must derive from Engine::Game::Scene");
        
        if (!sceneManager_)
            sceneManager_ = std::make_unique<Game::SceneManager>();
        
        return sceneManager_->EmplaceScene<TScene>(std::forward<Args>(args)...);
    }
}
