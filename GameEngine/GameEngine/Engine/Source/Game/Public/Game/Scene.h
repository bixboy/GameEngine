#pragma once
#include <memory>
#include <vector>
#include <SDL3/SDL_events.h>
#include <string>
#include <string_view>
#include "Game/Actor.h"
#include "Input/InputManager.h"

namespace Engine
{
    namespace Graphics { class Renderer; }
    namespace Core     { class Timer; class Window; }
    namespace Input    { class Input; }

    namespace Game
    {
        struct SceneContext
        {
            Graphics::Renderer* renderer{nullptr};
            Input::InputManager* inputManager{nullptr};
            Core::Window* window{nullptr};
            Core::Timer* timer{nullptr};
        };

        class Scene
        {
            public:
                explicit Scene(std::string name = "Unnamed Scene");
                virtual ~Scene() = default;

                virtual void HandleEvent(const SDL_Event& event) { (void)event; }
                virtual void Update(float deltaTime) { (void)deltaTime; }
                virtual void Render(Graphics::Renderer& renderer) { (void)renderer; }

                virtual void OnEnter() {}
                virtual void OnExit() {}

                void SetContext(SceneContext context) noexcept;

                void AddActor(std::unique_ptr<Actor> actor);

                [[nodiscard]] std::string_view Name() const noexcept { return name_; }

            protected:
                void SetName(std::string name);
            
                [[nodiscard]] Input::InputManager& GetInputManager() const;
                [[nodiscard]] bool HasInputManager() const noexcept { return context_.inputManager != nullptr; }
            
                [[nodiscard]] Graphics::Renderer& GetRenderer() const;
                [[nodiscard]] bool HasRenderer() const noexcept { return context_.renderer != nullptr; }
            
                [[nodiscard]] Core::Window& GetWindow() const;
                [[nodiscard]] bool HasWindow() const noexcept { return context_.window != nullptr; }
            
                [[nodiscard]] Core::Timer& GetTimer() const;
                [[nodiscard]] bool HasTimer() const noexcept { return context_.timer != nullptr; }

            private:
                std::string name_;
                SceneContext context_{};

                std::vector<std::unique_ptr<Actor>> actors_;
        };
    }
}
