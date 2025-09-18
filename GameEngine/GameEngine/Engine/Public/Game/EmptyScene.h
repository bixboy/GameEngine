#pragma once
#include <memory>
#include "Game/Scene.h"

namespace Engine::Game
{
    class Player;

    class EmptyScene : public Scene
    {
        public:
            EmptyScene();

            void HandleEvent(const SDL_Event& event) override;
            void Update(float deltaTime) override;
            void Render(Graphics::Renderer& renderer) override;

             void OnEnter() override;

        private:
            Player* player_{nullptr};
    };
}
