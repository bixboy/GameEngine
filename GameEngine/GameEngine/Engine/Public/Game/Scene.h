#pragma once

#include <SDL3/SDL_events.h>

#include <string>
#include <string_view>

namespace Engine {

namespace Graphics { class Renderer; }
namespace Core { class Timer; class Window; }
namespace Input { class Input; }

namespace Game {

struct SceneContext {
    Graphics::Renderer* renderer{nullptr};
    Input::Input* input{nullptr};
    Core::Window* window{nullptr};
    Core::Timer* timer{nullptr};
};

class Scene {
public:
    explicit Scene(std::string name = "Unnamed Scene");
    virtual ~Scene() = default;

    virtual void HandleEvent(const SDL_Event& event) = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render(Graphics::Renderer& renderer) = 0;

    virtual void OnEnter() {}
    virtual void OnExit() {}

    void SetContext(SceneContext context) noexcept;

    [[nodiscard]] std::string_view Name() const noexcept { return name_; }

protected:
    void SetName(std::string name);
    [[nodiscard]] Input::Input& GetInput() const;
    [[nodiscard]] bool HasInput() const noexcept { return context_.input != nullptr; }
    [[nodiscard]] Graphics::Renderer& GetRenderer() const;
    [[nodiscard]] bool HasRenderer() const noexcept { return context_.renderer != nullptr; }
    [[nodiscard]] Core::Window& GetWindow() const;
    [[nodiscard]] bool HasWindow() const noexcept { return context_.window != nullptr; }
    [[nodiscard]] Core::Timer& GetTimer() const;
    [[nodiscard]] bool HasTimer() const noexcept { return context_.timer != nullptr; }

private:
    std::string name_;
    SceneContext context_{};
};

}  // namespace Game

}  // namespace Engine
