#pragma once
#include <memory>
#include "Bix/Game/Scene.h"


namespace BixEngine::Game
{
    class Player;

    BCLASS()
    class EmptyScene : public Scene
    {
        public:
            GENERATED_BODY();

            EmptyScene();

            void HandleEvent(const SDL_Event& event) override;
            void Update(float deltaTime) override;
            void Render(Graphics::Renderer& renderer) override;

             void OnEnter() override;

        private:
            Player* player_{nullptr};
    };
}
