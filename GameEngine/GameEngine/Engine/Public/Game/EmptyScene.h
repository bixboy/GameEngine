#pragma once
#include "Game/Actor.h"
#include "Game/Scene.h"

namespace Engine::Game
{
    class EmptyScene : public Scene
    {
        public:
            EmptyScene();

            void HandleEvent(const SDL_Event& event) override;
            void Update(float deltaTime) override;
            void Render(Graphics::Renderer& renderer) override;

             void OnEnter() override;

        private:
            std::unique_ptr<Actor> player_;
    };
}
