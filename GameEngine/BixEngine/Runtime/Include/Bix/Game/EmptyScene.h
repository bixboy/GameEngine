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
            BIX_GENERATED_BODY(EmptyScene);
            GENERATED_BODY();
            BIX_DECLARE_SCRIPT_CLASS(EmptyScene, Scene);

            EmptyScene();

            void HandleEvent(const SDL_Event& event) override;
            void Update(float deltaTime) override;
            void Render(Graphics::Renderer& renderer) override;

             void OnEnter() override;

        private:
            Player* player_{nullptr};
    };
}
