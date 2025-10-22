#pragma once

#include "Bix/Game/Scene.h"

namespace BixEngine
{
    namespace Graphics { class Renderer; }

    namespace Game
    {
        class Player;

        class TestScene : public Scene
        {
            public:
                BIX_GENERATED_BODY(TestScene);
                BIX_DECLARE_SCRIPT_CLASS(TestScene, Scene);

                TestScene();
                ~TestScene() override;

                void OnEnter() override;
                void OnExit() override;
                void HandleEvent(const SDL_Event& event) override;
                void Update(float deltaTime) override;
                void Render(Graphics::Renderer& renderer) override;

            private:
                Player* player_{nullptr};
        };
    }
}

