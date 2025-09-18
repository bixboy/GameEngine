#pragma once

#include <memory>
#include <string>
#include <type_traits>

#include <SDL3/SDL_stdinc.h>

#define SDL_MAIN_HANDLED

namespace Engine {

namespace Graphics { class Renderer; }
namespace Game { class SceneManager; }
namespace Input { class Input; }
namespace Core { class Window; class Timer; }

namespace Core {

class Application {
public:
    struct Config {
        std::string windowTitle{"Custom Engine"};
        int width{1280};
        int height{720};
        bool resizable{true};
        SDL_Color clearColor{0, 0, 0, 255};
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
    void InitializeSubsystems();
    void ShutdownSubsystems() noexcept;
    void ShutdownSDL() noexcept;
    void ProcessEvents();
    void Update(float deltaTime);
    void Render();

    Config config_{};
    bool running_{false};
    bool sdlInitialized_{false};

    std::unique_ptr<Window> window_;
    std::unique_ptr<Graphics::Renderer> renderer_;
    std::unique_ptr<Game::SceneManager> sceneManager_;
    std::unique_ptr<Input::Input> input_;
    std::unique_ptr<Timer> timer_;
};

}  // namespace Core

}  // namespace Engine

#include "Game/SceneManager.h"
#include "Game/Scene.h"
#include "Input/Input.h"

namespace Engine::Core {

template <typename TScene, typename... Args>
TScene& Application::EmplaceScene(Args&&... args) {
    static_assert(std::is_base_of_v<Game::Scene, TScene>, "TScene must derive from Engine::Game::Scene");
    if (!sceneManager_) {
        sceneManager_ = std::make_unique<Game::SceneManager>();
    }
    return sceneManager_->EmplaceScene<TScene>(std::forward<Args>(args)...);
}

}  // namespace Engine::Core
