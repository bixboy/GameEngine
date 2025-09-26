#pragma once

#include <memory>

#include "Game/Scene.h"

namespace Engine
{
    namespace Graphics { class Renderer; }

    namespace Game
    {
        class Player;

        class TestScene : public Scene
        {
            public:
                TestScene();
                ~TestScene() override;

                void OnEnter() override;
                void OnExit() override;
                void HandleEvent(const SDL_Event& event) override;
                void Update(float deltaTime) override;
                void Render(Graphics::Renderer& renderer) override;

            private:
                std::unique_ptr<Player> player_{};
        };
    }
}

